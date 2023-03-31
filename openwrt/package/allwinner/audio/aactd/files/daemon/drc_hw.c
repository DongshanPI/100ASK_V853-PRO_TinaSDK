#include "drc_hw.h"

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "aactd/common.h"
#include "aactd/communicate.h"
#include "local.h"

#define SUNXI_REG_DUMP_PATH "/sys/class/sunxi_dump/dump"
#define SUNXI_REG_WRITE_PATH "/sys/class/sunxi_dump/write"

static struct aactd_com_drc_hw_data local_data;

int drc_hw_local_init(void)
{
    int ret;

    local_data.reg_num = 0;
    local_data.reg_args = malloc(com_buf_len_max);
    if (!local_data.reg_args) {
        aactd_error("No memory\n");
        ret = -ENOMEM;
        goto err_out;
    }

    return 0;

free_local_data:
    free(local_data.reg_args);
    local_data.reg_args = NULL;
err_out:
    return ret;
}

int drc_hw_local_release(void)
{
    if (local_data.reg_args) {
        free(local_data.reg_args);
        local_data.reg_args = NULL;
    }
    return 0;
}

static int drc_hw_data_set(const struct aactd_com_drc_hw_data *data)
{
    int ret = 0;
    int fd;
    int i;
    uint32_t reg_addr;
    char buf[22];

    fd = open(SUNXI_REG_WRITE_PATH, O_WRONLY);
    if (fd < 0) {
        aactd_error("Failed to open %s (%s)\n", SUNXI_REG_WRITE_PATH, strerror(errno));
        ret = -1;
        goto out;
    }

    for (i = 0; i < data->reg_num; ++i) {
        reg_addr = DRC_HW_REG_BASE_ADDR + data->reg_args[i].offset;
        snprintf(buf, sizeof(buf), "0x%08x 0x%08x", reg_addr, data->reg_args[i].value);
        AACTD_DEBUG(2, "Write \"%s\" to %s\n", buf, SUNXI_REG_WRITE_PATH);

        ret = write(fd, buf, strlen(buf));
        if (ret < 0) {
            aactd_error("Failed to write \"%s\" to %s (%s)\n",
                    buf, SUNXI_REG_WRITE_PATH, strerror(errno));
            goto close_fd;
        } else if ((unsigned int)ret < strlen(buf)) {
            aactd_error("Writed bytes to %s is fewer than expected\n",
                    SUNXI_REG_WRITE_PATH);
            ret = -1;
            goto close_fd;
        }
    }
    ret = 0;

close_fd:
    close(fd);
out:
    return ret;
}

int drc_hw_write_com_to_local(const struct aactd_com *com)
{
    int ret;

    ret = aactd_com_drc_hw_buf_to_data(com->data, &local_data);
    if (ret < 0) {
        aactd_error("aactd_com_drc_hw_buf_to_data failed\n");
        goto out;
    }

    ret = drc_hw_data_set(&local_data);
    if (ret < 0) {
        aactd_error("Failed to set DRC data\n");
        goto out;
    }

    ret = 0;
out:
    return ret;
}
