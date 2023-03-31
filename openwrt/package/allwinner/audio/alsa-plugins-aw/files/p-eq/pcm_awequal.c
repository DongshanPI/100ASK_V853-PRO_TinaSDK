#include "config.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <aactd/common.h>
#include <aactd/communicate.h>

#include "eq.h"

#define LINE_STR_BUF_LEN_MAX 1024

#define TUNING_COM_BUF_LEN_DEFAULT 1024
#define UNIX_DOMAIN_SOCKET_PATH "/tmp/aactd.eq_sw.sock"

#define EPOLL_MAXEVENTS 1

typedef struct {
    snd_pcm_extplug_t ext;

    char *config_file;
    int tuning_support;
    int tuning_com_buf_len;
    int verbose;

    pthread_t try_connecting_thread_id;
    pthread_cond_t cond_try_connecting;
    pthread_cond_t cond_connected;
    pthread_mutex_t mutex_tuning_fd;
    int tuning_fd;

    int should_stop;

    int eq_enabled;
    eq_prms_t eq_prms;
    void *eq_handle;

    pthread_t tuning_thread_id;
    pthread_mutex_t mutex_eq;

} snd_pcm_awequal_t;

static int parse_config_to_eq_prms(const char *config_file, eq_prms_t *prms, int *enabled)
{
    int ret;
    FILE *fp = NULL;
    char line_str[LINE_STR_BUF_LEN_MAX];
    int temp_int;
    int type;
    int frequency;
    int gain;
    float Q;
    int index = 0;

    if (!config_file || !prms) {
        SNDERR("Invalid config_file or prms");
        ret = -1;
        goto out;
    }

    fp = fopen(config_file, "r");
    if (!fp) {
        SNDERR("Failed to open %s (%s)", strerror(errno));
        ret = -1;
        goto out;
    }

    while (fgets(line_str, LINE_STR_BUF_LEN_MAX, fp)) {
        if (sscanf(line_str, "channels=%d", &temp_int) == 1) {
            prms->chan = temp_int;
        } else if (sscanf(line_str, "enabled=%d", &temp_int) == 1) {
            *enabled = temp_int;
        } else if (sscanf(line_str, "bin_num=%d", &temp_int) == 1) {
            prms->biq_num = temp_int;
        } else if (sscanf(line_str, "samplerate=%d", &temp_int) == 1) {
            prms->sampling_rate = temp_int;
        } else if (sscanf(line_str, "params=%d %d %d %f",
                    &type, &frequency, &gain, &Q) == 4) {
            prms->core_prms[index].type = type;
            prms->core_prms[index].fc = frequency;
            prms->core_prms[index].G = gain;
            prms->core_prms[index].Q = Q;
            ++index;
        }
    }
    ret = 0;

    fclose(fp);
out:
    return ret;
}

static void print_eq_prms(const eq_prms_t *prms)
{
    int i;
    for (i = 0; i < prms->biq_num; ++i) {
        const eq_core_prms_t *core_prms = &prms->core_prms[i];
        SNDERR(" [Biquad%02i] type: %i, freq: %d, gain: %d, Q: %.2f",
                i + 1, core_prms->type, core_prms->fc, core_prms->G, core_prms->Q);
    }
}

static void print_eq_prms_pc(const eq_prms_t_pc *prms)
{
    int i;
    for (i = 0; i < MAX_FILTER_N; ++i) {
        const eq_core_prms_t *core_prms = &prms->core_prms[i];
        SNDERR(" [BQ%02i] type: %i, freq: %d, gain: %d, Q: %.2f, enabled: %d",
                i + 1, core_prms->type, core_prms->fc, core_prms->G,
                core_prms->Q, prms->enable_[i]);
    }
}

static int aactd_com_eq_sw_data_to_eq_prms(const struct aactd_com_eq_sw_data *data,
        eq_prms_t_pc *prms)
{
    int ret;
    int i;

    for (i = 0; i < MAX_FILTER_N; ++i) {
        prms->enable_[i] = 0;
    }

    if (data->filter_num > MAX_FILTER_N) {
        SNDERR("Too many filters");
        ret = -1;
        goto out;
    }

    for (i = 0; i < data->filter_num; ++i) {
        prms->core_prms[i].type = data->filter_args[i].type;
        prms->core_prms[i].fc = data->filter_args[i].frequency;
        prms->core_prms[i].G = data->filter_args[i].gain;
        prms->core_prms[i].Q = (float)(data->filter_args[i].quality) / 100;
        prms->enable_[i] = data->filter_args[i].enabled;
    }

    ret = 0;
out:
    return ret;
}

static inline void *area_addr(const snd_pcm_channel_area_t *area,
        snd_pcm_uframes_t offset)
{
    unsigned int bitofs = area->first + area->step * offset;
    return (char *) area->addr + bitofs / 8;
}

static snd_pcm_sframes_t awequal_transfer(snd_pcm_extplug_t *ext,
        const snd_pcm_channel_area_t *dst_areas, snd_pcm_uframes_t dst_offset,
        const snd_pcm_channel_area_t *src_areas, snd_pcm_uframes_t src_offset,
        snd_pcm_uframes_t size)
{
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)ext->private_data;
    short *dst = (short *)area_addr(dst_areas, dst_offset);
    int channels = ext->channels;
    snd_pcm_format_t format = ext->format;

    snd_pcm_areas_copy(dst_areas, dst_offset, src_areas, src_offset,
            channels, size, format);

    pthread_mutex_lock(&awequal->mutex_eq);
    if (!awequal->eq_handle) {
        SNDERR("Invalid eq handle");
        pthread_mutex_unlock(&awequal->mutex_eq);
        return 0;
    }
    if (awequal->eq_enabled) {
        eq_process(awequal->eq_handle, dst, size);
    }
    pthread_mutex_unlock(&awequal->mutex_eq);

    return size;
}

static int connect_to_tuning_server(void)
{
    int ret;
    int socket_fd;
    struct sockaddr_un server_addr;
    int flags;

    socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        SNDERR("Failed to create socket (%s)", strerror(errno));
        ret = -1;
        goto err_out;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strcpy(server_addr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    ret = connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        SNDERR("Failed to connect to server %s", UNIX_DOMAIN_SOCKET_PATH);
        goto close_fd;
    }

    flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0) {
        SNDERR("F_GETFL error (%s)", strerror(errno));
        ret = -1;
        goto close_fd;
    }
    flags |= O_NONBLOCK;
    ret = fcntl(socket_fd, F_SETFL, flags);
    if (ret < 0) {
        SNDERR("F_SETFL error (%s)", strerror(errno));
        ret = -1;
        goto close_fd;
    }

    return socket_fd;

close_fd:
    close(socket_fd);
err_out:
    return ret;
}

static void *try_connecting_thread(void *arg)
{
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)arg;

    while (!awequal->should_stop) {
        pthread_mutex_lock(&awequal->mutex_tuning_fd);
        while (awequal->tuning_fd >= 0) {
            pthread_cond_wait(&awequal->cond_try_connecting, &awequal->mutex_tuning_fd);
            if (awequal->should_stop) {
                pthread_mutex_unlock(&awequal->mutex_tuning_fd);
                goto out;
            }
        }
        awequal->tuning_fd = connect_to_tuning_server();
        if (awequal->tuning_fd < 0) {
            SNDERR("Failed to connect to tuning server, try again");
            pthread_mutex_unlock(&awequal->mutex_tuning_fd);
            sleep(1);
            continue;
        }
        pthread_cond_signal(&awequal->cond_connected);
        pthread_mutex_unlock(&awequal->mutex_tuning_fd);
    }

out:
    pthread_exit(NULL);
}

static void *tuning_thread(void *arg)
{
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)arg;
    int ret;

    int epoll_fd;
    struct epoll_event event;
    struct epoll_event avail_events[EPOLL_MAXEVENTS];
    struct epoll_event *p_avail_event;
    int nfds;

    ssize_t read_bytes;
    uint8_t *remote_com_buf = NULL;
    unsigned int remote_com_actual_len;
    struct aactd_com remote_com = {
        .data = NULL,
    };
    uint8_t checksum_cal;

    struct aactd_com_eq_sw_data remote_data = {
        .filter_args = NULL,
    };
    eq_prms_t_pc remote_prms;

    remote_com_buf = malloc(awequal->tuning_com_buf_len);
    if (!remote_com_buf) {
        SNDERR("Failed to allocate memory for remote_com_buf");
        goto out;
    }
    remote_com.data = remote_com_buf + sizeof(struct aactd_com_header);

    remote_data.filter_args = malloc(awequal->tuning_com_buf_len);
    if (!remote_data.filter_args) {
        SNDERR("Failed to allocate memory for remote_data.filter_args");
        goto free_com_buf;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        SNDERR("Failed to create epoll instance (%s)", strerror(errno));
        goto free_filter_args;
    }

    while (!awequal->should_stop) {
        pthread_mutex_lock(&awequal->mutex_tuning_fd);
        while (awequal->tuning_fd < 0) {
            pthread_cond_wait(&awequal->cond_connected, &awequal->mutex_tuning_fd);
            if (awequal->should_stop) {
                pthread_mutex_unlock(&awequal->mutex_tuning_fd);
                goto close_epoll_fd;
            }
        }

        event.data.fd = awequal->tuning_fd;
        event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
        ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, awequal->tuning_fd, &event);
        if (ret < 0) {
            SNDERR("EPOLL_CTL_ADD error (%s)", strerror(errno));
            pthread_mutex_unlock(&awequal->mutex_tuning_fd);
            break;
        }
        pthread_mutex_unlock(&awequal->mutex_tuning_fd);

        while (1) {
            nfds = epoll_wait(epoll_fd, avail_events, EPOLL_MAXEVENTS, -1);
            if (nfds < 0) {
                SNDERR("epoll_wait error (%s)", strerror(errno));
                continue;
            }

            if (nfds > 1) {
                SNDERR("More than one file descriptor is ready");
                continue;
            }
            p_avail_event = &avail_events[0];

            pthread_mutex_lock(&awequal->mutex_tuning_fd);

            if (p_avail_event->data.fd != awequal->tuning_fd) {
                SNDERR("Unexpected file descriptor");
                goto wait_for_next_event;
            }
            if (p_avail_event->events & EPOLLERR) {
                SNDERR("EPOLLERR. Connection with tuning server may be closed");
                goto rebuild_connection;
            }
            if (p_avail_event->events & EPOLLRDHUP) {
                SNDERR("Connection with tuning server closed");
                goto rebuild_connection;
            }
            if (!(p_avail_event->events & EPOLLIN)) {
                SNDERR("No EPOLLIN event");
                goto wait_for_next_event;
            }

            /* Read header */
            read_bytes = aactd_readn(awequal->tuning_fd, remote_com_buf,
                    sizeof(struct aactd_com_header));
            if (read_bytes < 0) {
                SNDERR("Failed to read from tuning server");
                goto rebuild_connection;
            } else if (read_bytes == 0) {
                SNDERR("Connection with tuning server closed");
                goto rebuild_connection;
            } else if ((size_t)read_bytes < sizeof(struct aactd_com_header)) {
                SNDERR("Failed to read header from tuning server, try again");
                goto wait_for_next_event;
            }

            aactd_com_buf_to_header(remote_com_buf, &remote_com.header);

            if (remote_com.header.flag != AACTD_COM_HEADER_FLAG) {
                SNDERR("Incorrect header flag: 0x%x\n", remote_com.header.flag);
                goto wait_for_next_event;
            }

            remote_com_actual_len =
                sizeof(struct aactd_com_header) + remote_com.header.data_len + 1;

            /* Read data & checksum */
            read_bytes = aactd_readn(awequal->tuning_fd, remote_com.data,
                    remote_com.header.data_len + 1);
            if (read_bytes < 0) {
                SNDERR("Failed to read from tuning server");
                goto rebuild_connection;
            } else if (read_bytes == 0) {
                SNDERR("Connection with tuning server closed");
                goto rebuild_connection;
            } else if ((size_t)read_bytes < remote_com.header.data_len + 1) {
                SNDERR("Failed to read header data and checksum from tuning server, "
                        "try again");
                goto wait_for_next_event;
            }

            /* Verify checksum */
            remote_com.checksum = *(remote_com.data + remote_com.header.data_len);
            checksum_cal = aactd_calculate_checksum(remote_com_buf, remote_com_actual_len - 1);
            if (remote_com.checksum != checksum_cal) {
                SNDERR("Checksum error (got: 0x%x, calculated: 0x%x), discard and try again",
                        remote_com.checksum, checksum_cal);
                goto wait_for_next_event;
            }

            ret = aactd_com_eq_sw_buf_to_data(remote_com.data, &remote_data);
            if (ret < 0) {
                SNDERR("Failed to convert remote data buffer to data");
                goto wait_for_next_event;
            }
            ret = aactd_com_eq_sw_data_to_eq_prms(&remote_data, &remote_prms);
            if (ret < 0) {
                SNDERR("Failed to convert remote data to eq prms");
                goto wait_for_next_event;
            }

            pthread_mutex_lock(&awequal->mutex_eq);
            awequal->eq_enabled = remote_data.global_enabled;
            /*
             * NOTE: Use original "chan" and "sampling_rate", because they
             * won't be set by remote server.
             */
            remote_prms.chan = awequal->eq_prms.chan;
            remote_prms.sampling_rate = awequal->eq_prms.sampling_rate;

            eq_avert_parms(&awequal->eq_prms, &remote_prms);
            awequal->eq_handle = eq_setpara_reset(&awequal->eq_prms, awequal->eq_handle);
            if (!awequal->eq_handle) {
                SNDERR("Failed to reset eq parameters");
                pthread_mutex_unlock(&awequal->mutex_eq);
                goto wait_for_next_event;
            }
            pthread_mutex_unlock(&awequal->mutex_eq);

            if (awequal->verbose) {
                SNDERR("Data received from remote server:");
                aactd_com_print_content(&remote_com);
                SNDERR("Parameters updated:");
                print_eq_prms_pc(&remote_prms);
            }

wait_for_next_event:
            pthread_mutex_unlock(&awequal->mutex_tuning_fd);
            continue;

rebuild_connection:
            pthread_mutex_unlock(&awequal->mutex_tuning_fd);
            break;
        }

        pthread_mutex_lock(&awequal->mutex_tuning_fd);
        ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, awequal->tuning_fd, NULL);
        if (ret < 0) {
            SNDERR("EPOLL_CTL_DEL error (%s)", strerror(errno));
            pthread_mutex_unlock(&awequal->mutex_tuning_fd);
            break;
        }
        close(awequal->tuning_fd);
        awequal->tuning_fd = -1;
        pthread_cond_signal(&awequal->cond_try_connecting);
        pthread_mutex_unlock(&awequal->mutex_tuning_fd);
    }

close_epoll_fd:
    close(epoll_fd);
free_filter_args:
    free(remote_data.filter_args);
free_com_buf:
    free(remote_com_buf);
out:
    pthread_exit(NULL);
}

static int awequal_hw_params(snd_pcm_extplug_t *ext,
        snd_pcm_hw_params_t *params)
{
    int ret;
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)ext->private_data;

    if (awequal->eq_handle) {
        ret = 0;
        goto out;
    }

    ret = parse_config_to_eq_prms(awequal->config_file,
            &awequal->eq_prms, &awequal->eq_enabled);
    if (ret < 0) {
        SNDERR("parse_config_to_eq_prms failed");
        goto out;
    }
    /*
     * Overwrite the "chan" and "sampling_rate" parameters with ALSA parameters.
     * Therefore, these two parameters in config file are useless actually.
     */
    awequal->eq_prms.chan = ext->channels;
    awequal->eq_prms.sampling_rate = ext->rate;

    if (awequal->verbose) {
        SNDERR("Parse from config file %s", awequal->config_file);
        SNDERR("  GlobalEnable: %d", awequal->eq_enabled);
        print_eq_prms(&awequal->eq_prms);
    }

    if (awequal->tuning_support) {
        ret = pthread_create(&awequal->try_connecting_thread_id, NULL,
                try_connecting_thread, (void *)awequal);
        if (ret != 0) {
            SNDERR("Failed to create try connecting thread");
            ret = -1;
            goto out;
        }

        ret = pthread_create(&awequal->tuning_thread_id, NULL,
                tuning_thread, (void *)awequal);
        if (ret != 0) {
            SNDERR("Failed to create tuning thread");
            ret = -1;
            goto out;
        }
    }

    awequal->eq_handle = eq_create(&awequal->eq_prms);
    if (!awequal->eq_handle) {
        SNDERR("eq_create failed");
        ret = -1;
        goto out;
    }

    ret = 0;
out:
    return ret;
}

static int awequal_hw_free(snd_pcm_extplug_t *ext)
{
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)ext->private_data;

    if (!awequal) {
        return 0;
    }

    if (awequal->tuning_support) {
        pthread_mutex_lock(&awequal->mutex_tuning_fd);
        if (awequal->tuning_fd >= 0) {
            /*
             * try_connecting_thread may block on pthread_cond_wait(cond_try_connecting),
             * use signal to wake it up;
             * tuning_thread may block on epoll_wait(), use shutdown() to
             * trigger epoll_wait() to return. After finding the connection
             * has been shut down, the tuning_fd will be closed in tuning_thread.
             */
            pthread_cond_signal(&awequal->cond_try_connecting);
            shutdown(awequal->tuning_fd, SHUT_RDWR);
        } else {
            /*
             * tuning_thread may block on pthread_cond_wait(cond_connected),
             * use signal to wake it up.
             */
            pthread_cond_signal(&awequal->cond_connected);
        }
        awequal->should_stop = 1;
        pthread_mutex_unlock(&awequal->mutex_tuning_fd);

        pthread_join(awequal->try_connecting_thread_id, NULL);
        pthread_join(awequal->tuning_thread_id, NULL);
    }

    if (awequal->eq_handle) {
        eq_destroy(awequal->eq_handle);
        awequal->eq_handle = NULL;
    }
    return 0;
}

static int awequal_close(snd_pcm_extplug_t *ext)
{
    snd_pcm_awequal_t *awequal = (snd_pcm_awequal_t *)ext->private_data;

    if (!awequal) {
        return 0;
    }

    if (awequal->eq_handle) {
        eq_destroy(awequal->eq_handle);
        awequal->eq_handle = NULL;
    }
    pthread_mutex_destroy(&awequal->mutex_eq);
    pthread_mutex_destroy(&awequal->mutex_tuning_fd);
    pthread_cond_destroy(&awequal->cond_connected);
    pthread_cond_destroy(&awequal->cond_try_connecting);
    if (awequal->config_file) {
        free(awequal->config_file);
        awequal->config_file = NULL;
    }
    free(awequal);
    awequal = NULL;
    return 0;
}

static const snd_pcm_extplug_callback_t awequal_callback = {
    .transfer = awequal_transfer,
    .hw_params = awequal_hw_params,
    .hw_free = awequal_hw_free,
    .close = awequal_close,
};

static int get_int_parm(snd_config_t *n, const char *id, const char *str,
        int *value_ret)
{
    long value;
    int err;

    if (strcmp(id, str) != 0) {
        return 1;
    }
    err = snd_config_get_integer(n, &value);
    if (err < 0) {
        SNDERR("Invalid value for %s parameter", id);
        return err;
    }
    *value_ret = value;
    return 0;
}

static int get_string_param(snd_config_t *n, const char *id, const char *str,
        const char **value_ret)
{
    const char *value = NULL;
    int err;

    if (strcmp(id, str) != 0) {
        return 1;
    }
    err = snd_config_get_string(n, &value);
    if (err < 0) {
        SNDERR("Invalid value for %s parameter", id);
        return err;
    }
    *value_ret = value;
    return 0;
}

static int get_bool_parm(snd_config_t *n, const char *id, const char *str,
        int *value_ret)
{
    int value;
    if (strcmp(id, str) != 0) {
        return 1;
    }

    value = snd_config_get_bool(n);
    if (value < 0) {
        SNDERR("Invalid value for %s parameter", id);
        return value;
    }
    *value_ret = value;
    return 0;
}

SND_PCM_PLUGIN_DEFINE_FUNC(awequal)
{
    snd_config_iterator_t i, next;
    snd_config_t *slave_conf = NULL;
    snd_pcm_awequal_t *awequal= NULL;

    int err = 0;
    const char *config_file = NULL;
    int tuning_support = 0;
    int tuning_com_buf_len = TUNING_COM_BUF_LEN_DEFAULT;
    int verbose = 0;

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0) {
            continue;
        }
        if (strcmp(id, "comment") == 0
                || strcmp(id, "type") == 0
                || strcmp(id, "hint") == 0) {
            continue;
        }
        if (strcmp(id, "slave") == 0) {
            slave_conf = n;
            continue;
        }
        err = get_string_param(n, id, "config_file", &config_file);
        if (err <= 0) {
            goto ok;
        }
        err = get_bool_parm(n, id, "tuning_support", &tuning_support);
        if (err <= 0) {
            goto ok;
        }
        err = get_int_parm(n, id, "tuning_com_buf_len", &tuning_com_buf_len);
        if (err <= 0) {
            goto ok;
        }
        err = get_bool_parm(n, id, "verbose", &verbose);
        if (err <= 0) {
            goto ok;
        }
        SNDERR("Unknown field %s", id);
        err = -EINVAL;
    ok:
        if (err < 0) {
            goto error;
        }
    }

    if (!slave_conf) {
        SNDERR("No slave defined for awequal");
        err = -EINVAL;
        goto error;
    }

    awequal = calloc(1, sizeof(*awequal));
    if (!awequal) {
        err = -ENOMEM;
        goto error;
    }

    if (!config_file) {
        config_file = "/etc/awequal.conf";
    }
    awequal->config_file = (char *)malloc(strlen(config_file) + 1);
    if (!awequal->config_file) {
        err = -ENOMEM;
        goto error;
    }
    strcpy(awequal->config_file, config_file);

    awequal->tuning_support = tuning_support;
    awequal->tuning_com_buf_len = tuning_com_buf_len;
    awequal->verbose = verbose;

    err = pthread_cond_init(&awequal->cond_try_connecting, NULL);
    if (err < 0) {
        SNDERR("pthread_cond_init error");
        goto error;
    }
    err = pthread_cond_init(&awequal->cond_connected, NULL);
    if (err < 0) {
        SNDERR("pthread_cond_init error");
        goto destroy_cond_try_connecting;
    }
    err = pthread_mutex_init(&awequal->mutex_tuning_fd, NULL);
    if (err < 0) {
        SNDERR("pthread_mutex_init error");
        goto destroy_cond_connected;
    }
    awequal->tuning_fd = -1;

    awequal->should_stop = 0;

    err = pthread_mutex_init(&awequal->mutex_eq, NULL);
    if (err < 0) {
        SNDERR("pthread_mutex_init error");
        goto destroy_mutex_tuning_fd;
    }
    awequal->eq_handle = NULL;

    awequal->ext.version = SND_PCM_EXTPLUG_VERSION;
    awequal->ext.name = "Allwinner Equalizer Plugin";
    awequal->ext.callback = &awequal_callback;
    awequal->ext.private_data = awequal;

    err = snd_pcm_extplug_create(&awequal->ext, name, root, slave_conf, stream, mode);
    if (err < 0) {
        goto destroy_mutex_eq;
    }

    snd_pcm_extplug_set_param(
            &awequal->ext, SND_PCM_EXTPLUG_HW_FORMAT, SND_PCM_FORMAT_S16_LE);
    snd_pcm_extplug_set_slave_param(
            &awequal->ext, SND_PCM_EXTPLUG_HW_FORMAT, SND_PCM_FORMAT_S16_LE);

    *pcmp = awequal->ext.pcm;
    return 0;

destroy_mutex_eq:
    pthread_mutex_destroy(&awequal->mutex_eq);
destroy_mutex_tuning_fd:
    pthread_mutex_destroy(&awequal->mutex_tuning_fd);
destroy_cond_connected:
    pthread_cond_destroy(&awequal->cond_connected);
destroy_cond_try_connecting:
    pthread_cond_destroy(&awequal->cond_try_connecting);
error:
    if (awequal) {
        if (awequal->config_file) {
            free(awequal->config_file);
        }
        free(awequal);
    }
    return err;
}

SND_PCM_PLUGIN_SYMBOL(awequal)
