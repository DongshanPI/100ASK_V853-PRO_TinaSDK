#include "eq_sw.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "aactd/common.h"
#include "aactd/communicate.h"
#include "local.h"

#define UNIX_DOMAIN_SOCKET_PATH "/tmp/aactd.eq_sw.sock"

#define CLIENT_NUM_MAX 32

static pthread_t server_thread_id;

static struct client_connect {
    int fd;
    int is_used;
} client_connects[CLIENT_NUM_MAX];
static pthread_mutex_t mutex_client_connects = PTHREAD_MUTEX_INITIALIZER;

static uint8_t *local_com_buf = NULL;
static struct aactd_com local_com;
static pthread_mutex_t mutex_local_com = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_local_com_updated = PTHREAD_COND_INITIALIZER;

static void *client_connect_thread(void *arg)
{
    struct client_connect *connect = (struct client_connect *)arg;
    ssize_t write_bytes;
    unsigned int com_buf_actual_len;

    aactd_info("Connection from EQ_SW client established (fd: %d)\n", connect->fd);

    pthread_detach(pthread_self());

    pthread_mutex_lock(&mutex_local_com);
    /*
     * If local_com.header.data_len is not 0, meaning that local_com data has
     * been updated before. So write the previous data to the client.
     */
    if (local_com.header.data_len != 0) {
        AACTD_DEBUG(1, "Write current internal EQ_SW parameters to client\n");
        com_buf_actual_len =
            sizeof(struct aactd_com_header) + local_com.header.data_len + 1;
        write_bytes = aactd_writen(connect->fd, local_com_buf, com_buf_actual_len);
        if (write_bytes < 0) {
            aactd_error("Write error\n");
            pthread_mutex_unlock(&mutex_local_com);
            goto close_connect_fd;
        } else if (write_bytes < com_buf_actual_len) {
            aactd_error("Wrtie is incomplete\n");
            pthread_mutex_unlock(&mutex_local_com);
            goto  close_connect_fd;
        }
    }
    pthread_mutex_unlock(&mutex_local_com);

    while (1) {
        pthread_mutex_lock(&mutex_local_com);
        pthread_cond_wait(&cond_local_com_updated, &mutex_local_com);

        AACTD_DEBUG(1, "EQ_SW parameters updated, write to client\n");
        AACTD_DEBUG(2, "Print EQ_SW local parameters:\n");
        if (verbose_level >= 2) {
            aactd_com_print_content(&local_com);
        }

        com_buf_actual_len =
            sizeof(struct aactd_com_header) + local_com.header.data_len + 1;
        write_bytes = aactd_writen(connect->fd, local_com_buf, com_buf_actual_len);
        if (write_bytes < 0) {
            aactd_error("Write error\n");
            pthread_mutex_unlock(&mutex_local_com);
            goto close_connect_fd;
        } else if (write_bytes < com_buf_actual_len) {
            aactd_error("Wrtie is incomplete\n");
            pthread_mutex_unlock(&mutex_local_com);
            goto  close_connect_fd;
        }
        pthread_mutex_unlock(&mutex_local_com);
    }

close_connect_fd:
    close(connect->fd);
    aactd_info("Connection with EQ_SW client (fd: %d) closed\n", connect->fd);

    pthread_mutex_lock(&mutex_client_connects);
    connect->is_used = 0;
    pthread_mutex_unlock(&mutex_client_connects);

    pthread_exit(NULL);
}

static void *server_thread(void *arg)
{
    int ret;
    int i;
    int listen_fd, connect_fd;
    socklen_t client_addr_len;
    struct sockaddr_un server_addr, client_addr;
    pthread_t client_connect_thread_id;

    /* Init client_connects */
    pthread_mutex_lock(&mutex_client_connects);
    for (i = 0; i < CLIENT_NUM_MAX; ++i) {
        client_connects[i].is_used = 0;
    }
    pthread_mutex_unlock(&mutex_client_connects);

    listen_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        aactd_error("Failed to create socket (%s)\n", strerror(errno));
        goto out;
    }

    unlink(UNIX_DOMAIN_SOCKET_PATH);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strcpy(server_addr.sun_path, UNIX_DOMAIN_SOCKET_PATH);

    ret = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        aactd_error("Failed to bind path %s to socket (fd: %d) (%s)\n",
                server_addr.sun_path, listen_fd, strerror(errno));
        goto close_listen_fd;
    }
    AACTD_DEBUG(0, "Bind path %s to socket (fd: %d)\n", server_addr.sun_path, listen_fd);

    ret = listen(listen_fd, LISTEN_BACKLOG);
    if (ret < 0) {
        aactd_error("Failed to listen socket (fd: %d) (%s)\n", listen_fd, strerror(errno));
        goto close_listen_fd;
    }

    bzero(&client_addr, sizeof(client_addr));
    client_addr_len = sizeof(client_addr);
    while (1) {
        connect_fd = accept(listen_fd,
                (struct sockaddr *)&client_addr, &client_addr_len);
        if (connect_fd < 0) {
            aactd_error("Failed to accept connection from socket (fd: %d) (%s)\n",
                    listen_fd, strerror(errno));
            continue;
        }

        pthread_mutex_lock(&mutex_client_connects);
        for (i = 0; i < CLIENT_NUM_MAX; ++i) {
            if (!client_connects[i].is_used) {
                client_connects[i].fd = connect_fd;
                client_connects[i].is_used = 1;
                break;
            }
        }
        if (i >= CLIENT_NUM_MAX) {
            aactd_error("Not support more client connections (max: %d)\n",
                    CLIENT_NUM_MAX);
            close(connect_fd);
            pthread_mutex_unlock(&mutex_client_connects);
            continue;
        }
        ret = pthread_create(&client_connect_thread_id, NULL,
                client_connect_thread, (void *)&client_connects[i]);
        pthread_mutex_unlock(&mutex_client_connects);
    }

close_listen_fd:
    close(listen_fd);
out:
    pthread_exit(NULL);
}

int eq_sw_local_init(void)
{
    int ret;

    local_com_buf = malloc(com_buf_len_max);
    if (!local_com_buf) {
        aactd_error("No memory\n");
        ret = -ENOMEM;
        goto err_out;
    }
    local_com.data = local_com_buf + sizeof(struct aactd_com_header);
    local_com.header.data_len = 0;

    ret = pthread_create(&server_thread_id, NULL, server_thread, NULL);
    if (ret != 0) {
        aactd_error("Failed to create thread: server_thread\n");
        ret = -1;
        goto free_local_data;
    }

    return 0;

free_local_data:
    free(local_com_buf);
    local_com_buf = NULL;
err_out:
    return ret;
}

int eq_sw_local_release(void)
{
    pthread_join(server_thread_id, NULL);

    if (local_com_buf) {
        free(local_com_buf);
        local_com_buf = NULL;
    }
    return 0;
}

int eq_sw_write_com_to_local(const struct aactd_com *com)
{
    unsigned int com_buf_actual_len;

    pthread_mutex_lock(&mutex_local_com);
    aactd_com_copy(com, &local_com);
    aactd_com_header_to_buf(&local_com.header, local_com_buf);
    com_buf_actual_len =
        sizeof(struct aactd_com_header) + local_com.header.data_len + 1;
    local_com.checksum = aactd_calculate_checksum(local_com_buf, com_buf_actual_len - 1);
    *(local_com_buf + com_buf_actual_len - 1) = local_com.checksum;
    pthread_cond_broadcast(&cond_local_com_updated);
    pthread_mutex_unlock(&mutex_local_com);

    return 0;
}
