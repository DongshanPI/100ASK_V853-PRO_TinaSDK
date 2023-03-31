#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "aactd/common.h"
#include "aactd/communicate.h"

#define PORT_DEFAULT 5005

#define INET_ADDR_STR_LEN INET_ADDRSTRLEN
#define COM_BUF_LEN_DEFAULT 1024

#define EQ_SW_FILTER_ARG_NUM 2
#define DRC_HW_REG_ARG_NUM 7

static void help_msg(void)
{
    printf("\n");
    printf("USAGE:\n");
    printf("\taactd_test_client [ARGUMENTS]\n");
    printf("ARGUMENTS:\n");
    printf("\t-d,--drc  : test drc\n");
    printf("\t-e,--eq   : test eq\n");
}

int main(int argc, char *argv[])
{
    int ret;

    int socket_fd;
    struct sockaddr_in server_addr;
    char inet_addr_str[INET_ADDR_STR_LEN];

    uint8_t com_buf[COM_BUF_LEN_DEFAULT];

    struct aactd_com_eq_sw_filter_arg eq_sw_filter_args[EQ_SW_FILTER_ARG_NUM];
    struct aactd_com_drc_hw_reg_arg drc_hw_reg_args[DRC_HW_REG_ARG_NUM];

    struct aactd_com com = {
        .data = com_buf + sizeof(struct aactd_com_header),
        .checksum = 0,
    };
    struct aactd_com_eq_sw_data eq_sw_data = {
        .filter_args = eq_sw_filter_args,
    };
    struct aactd_com_drc_hw_data drc_hw_data = {
        .reg_args = drc_hw_reg_args,
    };

    enum aactd_com_type type = TYPE_RESERVED;
    unsigned int com_buf_actual_len;
    ssize_t write_bytes;

    const struct option opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "drc", no_argument, NULL, 'd' },
        { "eq", no_argument, NULL, 'e' },
    };
    int opt;

    if (argc <= 1) {
        help_msg();
        ret = -1;
        goto out;
    }

    while ((opt = getopt_long(argc, argv, "hde", opts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            help_msg();
            ret = 0;
            goto out;
        case 'd':
            type = DRC_HW;
            break;
        case 'e':
            type = EQ_SW;
            break;
        default:
            aactd_error("Invalid argument\n");
            help_msg();
            ret = -1;
            goto out;
        }
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        aactd_error("Failed to create socket\n");
        ret = -1;
        goto out;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = htons(PORT_DEFAULT);

    ret = connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        aactd_error("Failed to connect to server %s, port %d\n",
                inet_ntop(AF_INET, &server_addr.sin_addr, inet_addr_str, INET_ADDR_STR_LEN),
                ntohs(server_addr.sin_port));
        goto close_connect_fd;
    }

    switch (type) {
    case EQ_SW:
        com.header.flag = 0xAA;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = EQ_SW;
        com.header.data_len = sizeof(eq_sw_data.global_enabled)
            + sizeof(eq_sw_data.filter_num)
            + sizeof(struct aactd_com_eq_sw_filter_arg) * EQ_SW_FILTER_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        eq_sw_data.global_enabled = 1;
        eq_sw_data.filter_num = EQ_SW_FILTER_ARG_NUM;
        eq_sw_data.filter_args[0].type = 1;
        eq_sw_data.filter_args[0].frequency = 1000;
        eq_sw_data.filter_args[0].gain = -12;
        eq_sw_data.filter_args[0].quality = (int32_t)(1.5 * 100);
        eq_sw_data.filter_args[0].enabled = 1;
        eq_sw_data.filter_args[1].type = 1;
        eq_sw_data.filter_args[1].frequency = 9000;
        eq_sw_data.filter_args[1].gain = -12;
        eq_sw_data.filter_args[1].quality = (int32_t)(2.2 * 100);
        eq_sw_data.filter_args[1].enabled = 0;
        aactd_com_eq_sw_data_to_buf(&eq_sw_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;
        break;

    case DRC_HW:
        com.header.flag = 0xAA;
        com.header.version = 1;
        com.header.command = CMD_WRITE;
        com.header.type = DRC_HW;
        com.header.data_len = sizeof(drc_hw_data.reg_num)
            + sizeof(struct aactd_com_drc_hw_reg_arg) * DRC_HW_REG_ARG_NUM;
        aactd_com_header_to_buf(&com.header, com_buf);

        drc_hw_data.reg_num = DRC_HW_REG_ARG_NUM;
        drc_hw_data.reg_args[0].offset = 0xF0;
        drc_hw_data.reg_args[0].value = 0xA0000000;
        drc_hw_data.reg_args[1].offset = 0x108;
        drc_hw_data.reg_args[1].value = 0x00FB;
        drc_hw_data.reg_args[2].offset = 0x10C;
        drc_hw_data.reg_args[2].value = 0x000B;
        drc_hw_data.reg_args[3].offset = 0x110;
        drc_hw_data.reg_args[3].value = 0x77F0;
        drc_hw_data.reg_args[4].offset = 0x114;
        drc_hw_data.reg_args[4].value = 0x000B;
        drc_hw_data.reg_args[5].offset = 0x118;
        drc_hw_data.reg_args[5].value = 0x77F0;
        drc_hw_data.reg_args[6].offset = 0x11C;
        drc_hw_data.reg_args[6].value = 0x00FF;
        aactd_com_drc_hw_data_to_buf(&drc_hw_data, com_buf + sizeof(struct aactd_com_header));

        com_buf_actual_len = sizeof(struct aactd_com_header) + com.header.data_len + 1;
        com.checksum = aactd_calculate_checksum(com_buf, com_buf_actual_len - 1);
        *(com_buf + com_buf_actual_len - 1) = com.checksum;
        break;

    default:
        aactd_error("Unknown type\n");
        ret = -1;
        goto close_connect_fd;
    }

    write_bytes = aactd_writen(socket_fd, com_buf, com_buf_actual_len);
    if (write_bytes < 0) {
        aactd_error("Write error\n");
        ret = -1;
        goto close_connect_fd;
    } else if (write_bytes < com_buf_actual_len) {
        aactd_error("Write is incomplete\n");
        ret = -1;
        goto close_connect_fd;
    }

close_connect_fd:
    close(socket_fd);
out:
    return ret;
}
