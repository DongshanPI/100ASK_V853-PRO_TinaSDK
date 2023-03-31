/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Char <yanjianbo@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include "sst_log.h"
#include "zlib.h"
#include "secure_storage.h"
#include "secure_storage_ioctl.h"

static pthread_mutex_t sec_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 *  commond parameters
 */
#define FLASH_TYPE_NAND 0
#define FLASH_TYPE_SD1 1
#define FLASH_TYPE_SD2 2
#define FLASH_TYPE_NOR          3
#define FLASH_TYPE_EMMC3        4
#define FLASH_TYPE_SPI_NAND     5
#define FLASH_TYPE_UNKNOW -1

struct secblc_op_t {
    int item;
    unsigned char *buf;
    unsigned int len;
};

#define SUNXI_SECSTORE_VERSION 1
/* Store element struct in nand/emmc */
#define MAX_STORE_LEN 0xc00 /*3K payload*/
#define STORE_OBJECT_MAGIC 0x17253948
#define STORE_REENCRYPT_MAGIC 0x86734716
#define STORE_WRITE_PROTECT_MAGIC 0x8ad3820f
#pragma pack(1)
typedef struct {
    uint32_t magic; /* store object magic*/
    int32_t id; /*store id, 0x01,0x02.. for user*/
    uint8_t name[64]; /*OEM name*/
    uint32_t re_encrypt; /*flag for OEM object*/
    uint32_t version;
    uint32_t
        write_protect; /*can be placed or not, =0, can be write_protectd*/
    uint32_t reserved[3];
    uint32_t actual_len; /*the actual len in data buffer*/
    uint8_t data[MAX_STORE_LEN]; /*the payload of secure object*/
    uint32_t crc; /*crc to check the sotre_objce valid*/
} store_object_t;
#pragma pack()

#define CMDLINE_FILE_PATH ("/proc/cmdline")

#define SEC_BLK_SIZE (4096)
#define MAX_SECURE_STORAGE_MAX_ITEM (32)
#define MAP_KEY_NAME_SIZE (64)
#define MAP_KEY_DATA_SIZE (32)

#pragma pack(1)
struct map_info {
    uint8_t data[SEC_BLK_SIZE - sizeof(uint32_t) * 2];
    uint32_t magic;
    uint32_t crc;
} secure_storage_map;
#pragma pack()

static unsigned int secure_storage_inited = 0;

static unsigned int map_dirty;
static inline void set_map_dirty(void)
{
    map_dirty = 1;
}
static inline void clear_map_dirty(void)
{
    map_dirty = 0;
}
static inline int try_map_dirty(void)
{
    return map_dirty;
}

#define sst_ioctl_path "/dev/sst_storage"
static int sst_use_ioctl = 0;

static int flash_boot_type = FLASH_TYPE_UNKNOW;
/*
 * EMMC parameter
 */
#define SDMMC_SECTOR_SIZE (512)
#define SDMMC_SECURE_STORAGE_START_ADD (6 * 1024 * 1024 / 512) //6M
#define SDMMC_ITEM_SIZE (4 * 1024 / 512) //4K
static char *sd_oem_path = "/dev/mmcblk0";

/*
 * Nand parameters
 */
#define SECBLK_READ _IO('V', 20)
#define SECBLK_WRITE _IO('V', 21)
#define SECBLK_IOCTL _IO('V', 22)
static char *nand_oem_path = "/dev/nand0";
static char *nand_oem_path_mtd = "/dev/mtd2"; /* for ubi */

static int spliteKeyAndValue(char* str, char** key, char** value)
{
	int elocation = strcspn(str,"=");
	if (elocation < 0){
		return -1;
	}
	str[elocation] = '\0';
	*key = str;
	*value = str + elocation + 1;
	return 0;
}

static int getInfoFromCmdline(const char* key, char* value, int val_len)
{
	FILE* fp;
	char cmdline[1024];

	/* read partition info from /proc/cmdline */
	if ((fp = fopen(CMDLINE_FILE_PATH, "r")) == NULL) {
		SST_DBG_ERR("can't open %s \n", CMDLINE_FILE_PATH);
		return -1;
	}
	fgets(cmdline, 1024, fp);
	fclose(fp);

	/* splite the cmdline by space */
	char* p = NULL;
	char* lkey = NULL;
	char* lvalue = NULL;
	p = strtok(cmdline, " ");
	if (!spliteKeyAndValue(p, &lkey, &lvalue)){
		if (!strcmp(lkey,key)){
			goto done;
		}
	}

	while ((p = strtok(NULL, " "))){
		if (!spliteKeyAndValue(p, &lkey, &lvalue)){
			if (!strcmp(lkey,key)){
				goto done;
			}
		}
	}

	SST_DBG_ERR("No key named %s in cmdline.\n", key);
	return -1;

done:
	strncpy(value, lvalue, val_len);
	return 0;
}

static int get_flash_type(void)
{
	char ctype[20];
	flash_boot_type = FLASH_TYPE_UNKNOW;

	memset(ctype, 0, 16);
	if (getInfoFromCmdline("boot_type", ctype, 10) == 0) {
		/* get type from boot_type */
		SST_DBG_INFO("flash type = %s\n", ctype);
		flash_boot_type = atoi(ctype);
		if (flash_boot_type == 0 && ctype[0] != '0')
			flash_boot_type = FLASH_TYPE_UNKNOW;

		if (flash_boot_type == FLASH_TYPE_SPI_NAND)
			flash_boot_type = FLASH_TYPE_NAND;

	} else if (getInfoFromCmdline("root", ctype, 20) == 0) {
		/* get type from root */
		if (!strncmp(ctype, "/dev/nand", sizeof("/dev/nand")-1))
			flash_boot_type = FLASH_TYPE_NAND;
		else if (!strncmp(ctype, "/dev/ubi", sizeof("/dev/ubi")-1))
			flash_boot_type = FLASH_TYPE_NAND;
		else if (!strncmp(ctype, "/dev/mtdblock", sizeof("/dev/mtdblock")-1))
			flash_boot_type = FLASH_TYPE_NOR;
		else if (!strncmp(ctype, "/dev/mmcblk", sizeof("/dev/mmcblk")-1))
			flash_boot_type = FLASH_TYPE_EMMC3;
	}
	SST_DBG_ERR("flash type = %s: %d\n", ctype, flash_boot_type);
	return flash_boot_type;
}

static int check_secure_storage_key(unsigned char *buffer)
{
    store_object_t *obj = (store_object_t *)buffer;

    if (obj->magic != STORE_OBJECT_MAGIC) {
        SST_DBG_ERR("Input object magic fail [0x%x]\n", obj->magic);
        return -1;
    }

    if (obj->crc != crc32(0, (void *)obj, sizeof(*obj) - 4)) {
        SST_DBG_ERR("Input object crc fail [0x%x]\n", obj->crc);
        return -1;
    }
    return 0;
}

static int check_secure_storage_map(void *buffer)
{
    struct map_info *map_buf = buffer;

    if (map_buf->magic != STORE_OBJECT_MAGIC) {
        SST_DBG_ERR("Item0 (Map) magic is bad\n");
        return 2;
    }
    if (map_buf->crc !=
        crc32(0, (void *)map_buf, sizeof(struct map_info) - 4)) {
        SST_DBG_ERR("Item0 (Map) crc is fail [0x%x]\n", map_buf->crc);
        return -1;
    }
    return 0;
}

/*nand secure storage read/write*/
static int _nand_read(int id, unsigned char *buf, ssize_t len)
{
    int fd;
    int ret;
    char *dev;
    struct secblc_op_t *secblk_op;

    if (!buf) {
        SST_DBG_ERR("-buf NULL\n");
        return -1;
    }
    if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
        SST_DBG_ERR("out of range id %x\n", id);
        return -1;
    }

    if (access(nand_oem_path, F_OK) != -1)
        dev = nand_oem_path;
    else if (access(nand_oem_path_mtd, F_OK) != -1)
        dev = nand_oem_path_mtd;
    else {
        SST_DBG_ERR("nand device not exist\n");
        return -1;
    }

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        SST_DBG_ERR("Failed to open %s: %s", dev, strerror(errno));
        return -1;
    }

    secblk_op = (struct secblc_op_t *)malloc(sizeof(*secblk_op));
    if (!secblk_op) {
        SST_DBG_ERR("Out of memory\n");
        close(fd);
        return -1;
    }

    secblk_op->item = id;
    secblk_op->buf  = (unsigned char *)buf;
    secblk_op->len  = len;

    ret = ioctl(fd, SECBLK_READ, secblk_op);
    if (ret < 0) {
        SST_DBG_ERR("Nand secblk read fail\n");
        free(secblk_op);
        close(fd);
        return -1;
    }

    free(secblk_op);
    close(fd);
    return 0;
}

static int _nand_write(int id, unsigned char *buf, ssize_t len)
{
    int fd;
    char *dev = NULL;
    int ret;
    struct secblc_op_t *secblk_op;

    if (!buf) {
        SST_DBG_ERR("- buf NULL\n");
        return (-1);
    }

    if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
        SST_DBG_ERR("out of range id %x\n", id);
        return (-1);
    }

    if (access(nand_oem_path, F_OK) != -1)
        dev = nand_oem_path;
    else if (access(nand_oem_path_mtd, F_OK) != -1)
        dev = nand_oem_path_mtd;
    else {
        SST_DBG_ERR("nand device not exist\n");
        return -1;
    }

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        SST_DBG_ERR("Failed to open %s: %s", dev, strerror(errno));
        return -1;
    }

    secblk_op = (struct secblc_op_t *)malloc(sizeof(*secblk_op));
    if (!secblk_op) {
        SST_DBG_ERR("Out of memory\n");
        close(fd);
        return -1;
    }

    secblk_op->item = id;
    secblk_op->buf  = (unsigned char *)buf;
    secblk_op->len  = len;

    ret = ioctl(fd, SECBLK_WRITE, secblk_op);
    if (ret < 0) {
        SST_DBG_ERR("Nand secblk write fail\n");
        free(secblk_op);
        close(fd);
        return -1;
    }

    fsync(fd);
    free(secblk_op);
    close(fd);
    return 0;
}

/*emmc secure storage read/write*/
static int _sd_read(int id, unsigned char *buf, ssize_t len)
{
    int offset, ret;
    char *align, *sd_align_buffer;
    int fd;

    if (!buf) {
        SST_DBG_ERR("-buf NULL\n");
        return -1;
    }

    if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
        SST_DBG_ERR("out of range id %x\n", id);
        return (-1);
    }

    sd_align_buffer = malloc(SEC_BLK_SIZE + 64);
    if (!sd_align_buffer) {
        SST_DBG_ERR("out of memory\n");
        return -1;
    }

    align = (char *)(((unsigned long)sd_align_buffer + 0x20) & (~0x1f));

    fd = open(sd_oem_path, O_RDWR);
    if (fd < 0) {
        SST_DBG_ERR("Failed to open %s: %s", sd_oem_path, strerror(errno));
        free(sd_align_buffer);
        return -1;
    }

    /* raw */
    offset = (SDMMC_SECURE_STORAGE_START_ADD + SDMMC_ITEM_SIZE * 2 * id) *
             SDMMC_SECTOR_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        SST_DBG_ERR("Failed to sleek %s, offset = %d\n", (char *)sd_oem_path, offset);
        close(fd);
        free(sd_align_buffer);
        return -1;
    }
    ret = read(fd, align, len);
    if (ret != len) {
        SST_DBG_ERR("_sst_user_read: read request len 0x%x, actual read 0x%x\n", len,
              ret);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }

    memcpy(buf, align, len);
    free(sd_align_buffer);
    close(fd);
    return 0;
}

/*emmc secure storage backup item read*/
static int _sd_read_backup(int id, unsigned char *buf, ssize_t len)
{
    int offset, ret;
    char *align, *sd_align_buffer;
    int fd;

    if (!buf) {
        SST_DBG_ERR("-buf NULL\n");
        return -1;
    }

    if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
        SST_DBG_ERR("out of range id %x\n", id);
        return (-1);
    }

    sd_align_buffer = malloc(SEC_BLK_SIZE + 64);
    if (!sd_align_buffer) {
        SST_DBG_ERR("out of memory\n");
        return -1;
    }

    align = (char *)(((unsigned long)sd_align_buffer + 0x20) & (~0x1f));

    fd = open(sd_oem_path, O_RDWR);
    if (fd < 0) {
        SST_DBG_ERR("Failed to open %s: %s", sd_oem_path, strerror(errno));
        free(sd_align_buffer);
        return -1;
    }

    /*backup */
    offset = (SDMMC_SECURE_STORAGE_START_ADD + SDMMC_ITEM_SIZE * 2 * id +
              SDMMC_ITEM_SIZE) *
             SDMMC_SECTOR_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        SST_DBG_ERR("Failed to sleek %s, offset = %d\n", (char *)sd_oem_path, offset);
        close(fd);
        free(sd_align_buffer);
        return -1;
    }
    ret = read(fd, align, len);
    if (ret != len) {
        SST_DBG_ERR("_sst_user_read: read request len 0x%x, actual read 0x%x\n", len,
              ret);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }

    memcpy(buf, align, len);
    free(sd_align_buffer);
    close(fd);
    return 0;
}

static int _sd_write(int id, unsigned char *buf, ssize_t len)
{
    int offset, ret;
    char *align, *sd_align_buffer;
    int fd;

    if (!buf) {
        SST_DBG_ERR("- buf NULL\n");
        return (-1);
    }

    if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
        SST_DBG_ERR("out of range id %x\n", id);
        return (-1);
    }

    sd_align_buffer = malloc(SEC_BLK_SIZE + 64);
    if (!sd_align_buffer) {
        SST_DBG_ERR("out of memory\n");
        return -1;
    }

    align = (char *)(((unsigned long)sd_align_buffer + 0x20) & (~0x1f));
    memcpy(align, buf, len);

    offset = (SDMMC_SECURE_STORAGE_START_ADD + SDMMC_ITEM_SIZE * 2 * id) *
             SDMMC_SECTOR_SIZE;

    fd = open(sd_oem_path, O_RDWR);
    if (fd < 0) {
        SST_DBG_ERR("Failed to open %s: %s", sd_oem_path, strerror(errno));
        free(sd_align_buffer);
        return -1;
    }

    /*raw */
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        SST_DBG_ERR("Failed to sleek %s, offset = %d\n", sd_oem_path, offset);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }

    ret = write(fd, align, len);
    if (ret != len) {
        SST_DBG_ERR("_sst_user_write: write request len 0x%x, actual write 0x%x\n",
              len, ret);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }
    /*backup */
    offset = (SDMMC_SECURE_STORAGE_START_ADD + SDMMC_ITEM_SIZE * 2 * id +
              SDMMC_ITEM_SIZE) *
             SDMMC_SECTOR_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        SST_DBG_ERR("Failed to sleek %s, offset = %d\n", sd_oem_path, offset);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }

    ret = write(fd, align, len);
    if (ret != len) {
        SST_DBG_ERR("_sst_user_write: write request len 0x%x, actual write 0x%x\n",
              len, ret);
        free(sd_align_buffer);
        close(fd);
        return -1;
    }

    fsync(fd);
    free(sd_align_buffer);
    close(fd);
    return 0;
}

static int sd_read_key(int id, unsigned char *buf, ssize_t len)
{
    int ret;

    SST_DBG_INFO("read item%d copy0\n", id);
    if (sst_use_ioctl)
        ret = _sd_read_ioctl(id, buf, len);
    else
        ret = _sd_read(id, buf, len);
    if (!ret) {
        /*check copy 0 */
        if (!check_secure_storage_key(buf)) {
            SST_DBG_INFO("the secure storage item%d copy0 is good\n", id);
            return 0; /*copy 0 pass*/
        }
        SST_DBG_ERR("the secure storage item%d copy0 is bad\n", id);
    }

    // read backup
    memset(buf, 0x0, len);
    SST_DBG_INFO("read item%d copy1\n", id);
    if (sst_use_ioctl)
        ret = _sd_read_backup_ioctl(id, buf, len);
    else
        ret = _sd_read_backup(id, buf, len);
    if (!ret) {
        /*check copy 1 */
        if (!check_secure_storage_key(buf)) {
            SST_DBG_INFO("the secure storage item%d copy1 is good\n", id);
            return 0; /*copy 1 pass*/
        }
        SST_DBG_ERR("the secure storage item%d copy1 is bad\n", id);
    }
    return -1;
}

static int sd_read_map(int id, unsigned char *buf, ssize_t len)
{
    int ret;
    int have_map_copy0 = 0;

    char *map_copy0_buf;

    map_copy0_buf = (char *)malloc(len);
    if (!map_copy0_buf) {
        SST_DBG_ERR("out of memory\n");
        return -1;
    }

    SST_DBG_INFO("read item%d copy0\n", id);
    if (sst_use_ioctl)
        ret = _sd_read_ioctl(id, buf, len);
    else
        ret = _sd_read(id, buf, len);
    if (!ret) {
        /*read ok*/
        ret = check_secure_storage_map(buf);
        if (ret == 0) {
            SST_DBG_INFO("the secure storage item0 copy0 is good\n");
            free(map_copy0_buf);
            return 0; /*copy 0 pass*/
        } else if (ret == 2) {
            memcpy(map_copy0_buf, buf, len);
            have_map_copy0 = 1;
            SST_DBG_INFO("the secure storage item0 copy0 is bad\n");
        } else
            SST_DBG_INFO(
                "the secure storage item0 copy 0 crc fail, the data is bad\n");
    }

    // read backup
    memset(buf, 0x0, len);
    SST_DBG_INFO("read item%d copy1\n", id);
    if (sst_use_ioctl)
        ret = _sd_read_backup_ioctl(id, buf, len);
    else
        ret = _sd_read_backup(id, buf, len);
    if (!ret) {
        ret = check_secure_storage_map(buf);
        if (ret == 0) {
            SST_DBG_INFO("the secure storage item0 copy1 is good\n");
            free(map_copy0_buf);
            return 0;
        } else if (ret == 2) {
            if (have_map_copy0 && !memcmp(map_copy0_buf, buf, len)) {
                SST_DBG_INFO(
                    "the secure storage item0 copy0 == copy1, the data is good\n");
                free(map_copy0_buf);
                return 0; /*copy have no magic and crc*/
            } else {
                SST_DBG_INFO(
                    "the secure storage item0 copy0 != copy1, try to use copy0\n");

                // when the map is not verify, try to use copy0, maybe is good,  20151125
                memset(buf, 0x0, len);
                memcpy(buf, map_copy0_buf, len);

                free(map_copy0_buf);
                return 1;
            }
        } else {
            SST_DBG_INFO(
                "the secure storage item0 copy 1 crc fail, the data is bad\n");
            free(map_copy0_buf);
            return 1;
        }
    }
    SST_DBG_ERR("unknown error happen in item 0 read\n");
    free(map_copy0_buf);
    return -1;
}

static int nv_read(int id, unsigned char *buf, ssize_t len)
{
    int ret;
    switch (flash_boot_type) {
    case FLASH_TYPE_NAND:
        if (sst_use_ioctl)
            ret = _nand_read_ioctl(id, buf, len);
        else
            ret = _nand_read(id, buf, len);
        break;
    case FLASH_TYPE_SD1:
    case FLASH_TYPE_SD2:
        if (id == 0)
            ret = sd_read_map(0, buf, len);
        else
            ret = sd_read_key(id, buf, len);
        break;
    default:
        SST_DBG_ERR("Unknown no-volatile device: %d\n", flash_boot_type);
        ret = -1;
        break;
    }

    return ret;
}

static int nv_write(int id, unsigned char *buf, ssize_t len)
{
    int ret;
    switch (flash_boot_type) {
    case FLASH_TYPE_NAND:
        if (sst_use_ioctl)
            ret = _nand_write_ioctl(id, buf, len);
        else
            ret = _nand_write(id, buf, len);
        break;
    case FLASH_TYPE_SD1:
    case FLASH_TYPE_SD2:
        if (sst_use_ioctl)
            ret = _sd_write_ioctl(id, buf, len);
        else
            ret = _sd_write(id, buf, len);
        break;
    default:
        SST_DBG_ERR("Unknown no-volatile device: %d\n", flash_boot_type);
        ret = -1;
        break;
    }
    return ret;
}

/*Low-level operation*/
static int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
    return nv_read(item, buf, len);
}

static int sunxi_secstorage_write(int item, unsigned char *buf,
                                  unsigned int len)
{
    return nv_write(item, buf, len);
}

static void __is_use_sst_ctl(void)
{
    int ret;

    ret = access(sst_ioctl_path, F_OK);
    if (ret < 0) {
        sst_use_ioctl = 0;
        return;
    }
    SST_DBG_INFO("use sst ioctl");
    sst_use_ioctl = 1;
}

/*
 * Map format:
 *      name:length\0
 *      name:length\0
 */
static int __probe_name_in_map(unsigned char *buffer, const char *item_name,
                               int *len)
{
    unsigned char *buf_start = buffer;
    int index                = 1;
    char name[MAP_KEY_NAME_SIZE], length[MAP_KEY_DATA_SIZE];
    unsigned int i, j;

    SST_DBG_INFO("__probe_name_in_map\n");

    while (*buf_start != '\0') {
        memset(name, 0, MAP_KEY_NAME_SIZE);
        memset(length, 0, MAP_KEY_DATA_SIZE);
        i = 0;
        while (buf_start[i] != ':' && (buf_start[i] != '\0') &&
               (&buf_start[i] - buffer) < SEC_BLK_SIZE && i < sizeof(name)) {
            name[i] = buf_start[i];
            i++;
        }

        if (i >= sizeof(name))
            return -1;

        i++;
        j = 0;
        while ((buf_start[i] != ' ') && (buf_start[i] != '\0') &&
               (&buf_start[i] - buffer) < SEC_BLK_SIZE && j < sizeof(length)) {
            length[j] = buf_start[i];
            i++;
            j++;
        }

        //deal rubbish data
        if ((&buf_start[i] - buffer) >= SEC_BLK_SIZE || j >= sizeof(length)) {
            return -1;
        }

        if ((strlen(item_name) == strlen(name)) &&
            (memcmp(item_name, name, strlen(item_name)) == 0)) {
            buf_start += strlen(item_name) + 1;
            *len = atol((const char *)length);
            return index;
        }
        index++;
        buf_start += strlen((const char *)buf_start) + 1;
    }

    return -1;
}

static int __fill_name_in_map(unsigned char *buffer, const char *item_name,
                              int length)
{
    unsigned char *buf_start = buffer;
    int index                = 1;
    int name_len;

    while (*buf_start != '\0' && (buf_start - buffer) < SEC_BLK_SIZE) {
        name_len = 0;
        while (buf_start[name_len] != ':' &&
               (&buf_start[name_len] - buffer) < SEC_BLK_SIZE &&
               name_len < MAP_KEY_NAME_SIZE)
            name_len++;

        //deal rubbish data
        if ((&buf_start[name_len] - buffer) >= SEC_BLK_SIZE ||
            name_len >= MAP_KEY_NAME_SIZE) {
            SST_DBG_INFO("__fill_name_in_map: dirty map, memset 0\n");
            memset(buffer, 0x0, SEC_BLK_SIZE);
            buf_start = buffer;
            index     = 1;
            break;
        }

            SST_DBG_ERR("name in map %s\n", buf_start);
		if (!memcmp((const char *)buf_start, item_name, name_len)) {
            SST_DBG_INFO("name in map %s\n", buf_start);
			return index;
		}
        index++;
        buf_start += strlen((const char *)buf_start) + 1;
    }
    if (index >= 32)
        return -1;

    sprintf((char *)buf_start, "%s:%d", item_name, length);

    return index;
}

/*load source data to secure_object struct
 *
 * src      : secure_object
 * len      : secure_object buffer len
 * payload  : taregt payload
 * retLen   : target payload actual length
 * */
static int unwrap_secure_object(void *src, unsigned int len, void *payload,
                                int *retLen)
{
    store_object_t *obj;

    if (len != sizeof(store_object_t)) {
        SST_DBG_ERR("Input length not equal secure object size 0x%x\n", len);
        return -1;
    }

    obj = (store_object_t *)src;

    if (obj->magic != STORE_OBJECT_MAGIC) {
        SST_DBG_ERR("Input object magic fail [0x%x]\n", obj->magic);
        return -1;
    }

    if (obj->re_encrypt == STORE_REENCRYPT_MAGIC) {
        SST_DBG_INFO("secure object is encrypt by chip\n");
    }

    if (obj->crc != crc32(0, (void *)obj, sizeof(*obj) - 4)) {
        SST_DBG_ERR("Input object crc fail [0x%x]\n", obj->crc);
        return -1;
    }

    memcpy(payload, obj->data, obj->actual_len);
    *retLen = obj->actual_len;

    return 0;
}

/*Store source data to secure_object struct
 *
 * src      : payload data to secure_object
 * name     : input payloader data name
 * tagt     : taregt secure_object
 * len      : input payload data length
 * retLen   : target secure_object length
 * */
static int wrap_secure_object(void *src, const char *name, unsigned int len,
                              void *tagt, int *retLen)
{
    store_object_t *obj;

    if (len > MAX_STORE_LEN) {
        SST_DBG_ERR("Input length larger then secure object payload size\n");
        return -1;
    }

    obj     = (store_object_t *)tagt;
    *retLen = sizeof(store_object_t);

    obj->magic = STORE_OBJECT_MAGIC;
    strncpy(obj->name, name, 64);
    obj->re_encrypt    = 0;
    obj->version       = SUNXI_SECSTORE_VERSION;
    obj->id            = 0;
    obj->write_protect = 0;
    memset(obj->reserved, 0, 4);
    obj->actual_len = len;
    memcpy(obj->data, src, len);

    obj->crc = crc32(0, (const unsigned char *)obj, sizeof(*obj) - 4);

    return 0;
}

static int sunxi_secure_storage_read(const char *item_name, char *buffer,
                                     int buffer_len, int *data_len)
{
    int ret, index;
    int len_in_store;
    unsigned char *buffer_to_sec;

    SST_DBG_INFO("secure storage read %s \n", item_name);
    if (!secure_storage_inited) {
        SST_DBG_ERR("%s err: secure storage has not been inited\n", __func__);
        return -1;
    }

    buffer_to_sec = (unsigned char *)malloc(SEC_BLK_SIZE);
    if (!buffer_to_sec) {
        SST_DBG_ERR("%s out of memory", __func__);
        return -1;
    }

    index =
        __probe_name_in_map(secure_storage_map.data, item_name, &len_in_store);
    if (index < 0) {
        SST_DBG_ERR("no item name %s in the map\n", item_name);
        free(buffer_to_sec);
        return -1;
    }
    memset(buffer, 0, buffer_len);
    ret = sunxi_secstorage_read(index, buffer_to_sec, SEC_BLK_SIZE);
    if (ret) {
        SST_DBG_ERR("read secure storage block %d name %s err\n", index, item_name);
        free(buffer_to_sec);
        return -1;
    }
    if (len_in_store > buffer_len) {
        memcpy(buffer, buffer_to_sec, buffer_len);
    } else {
        memcpy(buffer, buffer_to_sec, len_in_store);
    }
    *data_len = len_in_store;

    SST_DBG_INFO("secure storage read %s done\n", item_name);

    free(buffer_to_sec);
    return 0;
}

/*Add new item to secure storage*/
static int sunxi_secure_storage_write(const char *item_name, char *buffer,
                                      int length)
{
    int ret, index;

    if (!secure_storage_inited) {
        SST_DBG_ERR("%s err: secure storage has not been inited\n", __func__);

        return -1;
    }
    index = __fill_name_in_map(secure_storage_map.data, item_name, length);
    if (index < 0) {
        SST_DBG_ERR("write secure storage block %d name %s overrage\n", index,
              item_name);

        return -1;
    }

    ret = sunxi_secstorage_write(index, (unsigned char *)buffer, SEC_BLK_SIZE);
    if (ret) {
        SST_DBG_ERR("write secure storage block %d name %s err\n", index, item_name);

        return -1;
    }
    set_map_dirty();
    SST_DBG_ERR("write secure storage: %d ok\n", index);

    return 0;
}

static int sunxi_secure_storage_probe(const char *item_name)
{
    int ret;
    int len;

    if (!secure_storage_inited) {
        SST_DBG_ERR("%s err: secure storage has not been inited\n", __func__);

        return -1;
    }
    ret = __probe_name_in_map(secure_storage_map.data, item_name, &len);
    if (ret < 0) {
        SST_DBG_ERR("no item name %s in the map\n", item_name);

        return -1;
    }
    return 0;
}

int sunxi_secure_object_erase(void)
{
    if (sunxi_secure_storage_init()) {
        SST_DBG_ERR("%s secure storage init err\n", __func__);
        return -1;
    }

    memset(&secure_storage_map, 0, SEC_BLK_SIZE);
    set_map_dirty();

    if (sunxi_secure_storage_exit(0)) {
        SST_DBG_ERR("%s secure storage exit err\n", __func__);
        return -1;
    }
    return 0;
}

int sunxi_secure_object_list(void)
{
    int ret, index = 1;
    unsigned char *buffer;
    unsigned char *buf_start;
    unsigned char key_data[SEC_BLK_SIZE];
    char name[MAP_KEY_NAME_SIZE], length[MAP_KEY_DATA_SIZE];
    int retLen;
    unsigned int i, j, len;

    if (sunxi_secure_storage_init()) {
        SST_DBG_ERR("%s secure storage init err\n", __func__);
        return -1;
    }
    printf("\n[secure storage]\n");

    buffer    = (unsigned char *)&secure_storage_map;
    buf_start = buffer;

    while (*buf_start != '\0') {
        memset(name, 0, MAP_KEY_NAME_SIZE);
        memset(length, 0, MAP_KEY_DATA_SIZE);
        i = 0;
        while (buf_start[i] != ':' && (buf_start[i] != '\0') &&
               (&buf_start[i] - buffer) < SEC_BLK_SIZE && i < sizeof(name)) {
            name[i] = buf_start[i];
            i++;
        }
        if (i >= sizeof(name))
            return -1;

        i++;
        j = 0;
        while ((buf_start[i] != ' ') && (buf_start[i] != '\0') &&
               (&buf_start[i] - buffer) < SEC_BLK_SIZE && j < sizeof(length)) {
            length[j] = buf_start[i];
            i++;
            j++;
        }
        //deal rubbish data
        if ((&buf_start[i] - buffer) >= SEC_BLK_SIZE || j >= sizeof(length)) {
            return -1;
        }
        len = atoi(length);
        SST_DBG_INFO("name in map %s, len 0x%x\n", name, len);
        memset(key_data, 0, SEC_BLK_SIZE);

        ret = sunxi_secure_object_read(name, (void *)key_data, SEC_BLK_SIZE,
                                       &retLen);
        if (ret < 0) {
            SST_DBG_ERR("get secure storage index %d err\n", index);
            return -1;
        }
        printf("%d: %s = %s\n", index, name, key_data);

        index++;
        buf_start += strlen((const char *)buf_start) + 1;
    }
    if (index == 1) {
        printf("no key!!\n");
    }

    if (sunxi_secure_storage_exit(0)) {
        SST_DBG_ERR("%s secure storage exit err\n", __func__);
        return -1;
    }
    return 0;
}

int sunxi_secure_object_read(const char *item_name, char *buffer,
                             int buffer_len, int *data_len)
{
    char secure_object[SEC_BLK_SIZE];
    int retLen, ret;

    (void)buffer_len;
    memset(secure_object, 0, SEC_BLK_SIZE);

    ret = sunxi_secure_storage_read(item_name, secure_object, SEC_BLK_SIZE,
                                    &retLen);
    if (ret) {
        SST_DBG_ERR("sunxi storage read fail\n");
        return -1;
    }
    return unwrap_secure_object(secure_object, retLen, buffer, data_len);
}

int sunxi_secure_object_write(const char *item_name, char *buffer, int length)
{
    char secure_object[SEC_BLK_SIZE];
    int retLen;
    int ret;

    /*
     * If there is THE same name itme in the secure storage, we need to decide
     * how to deal with it.
     * case 1. The same name in the secure storage is write_protected, Do thing.
     * case 2. Otherwise, write it.
     */
    if (sunxi_secure_storage_probe(item_name) == 0) {
        /*the same name in map*/
        sunxi_secure_object_read(item_name, secure_object, SEC_BLK_SIZE,
                                 &retLen);
        store_object_t *so = (store_object_t *)secure_object;
        if (so->magic == STORE_OBJECT_MAGIC &&
            so->write_protect == STORE_WRITE_PROTECT_MAGIC) {
            SST_DBG_ERR("Can't write a write_protect secure item\n");
            return -1;
        } else {
            SST_DBG_ERR("secure object name[%s] already in device\n", item_name);
        }
    }

    memset(secure_object, 0, SEC_BLK_SIZE);
    retLen = 0;
    ret = wrap_secure_object((void *)buffer, item_name, length, secure_object,
                             &retLen);
    if (ret < 0 || retLen > SEC_BLK_SIZE) {
        SST_DBG_ERR("wrap fail before secure storage write\n");
        return -1;
    }
    return sunxi_secure_storage_write(item_name, secure_object, retLen);
}

int sunxi_secure_storage_init(void)
{
    int ret;

    pthread_mutex_lock(&sec_mutex);

    if (!secure_storage_inited) {
        get_flash_type();
        __is_use_sst_ctl();

        ret =
            sunxi_secstorage_read(0, (void *)&secure_storage_map, SEC_BLK_SIZE);
        if (ret == -1) {
            SST_DBG_ERR("get secure storage map err\n");
            pthread_mutex_unlock(&sec_mutex);
            return -1;
        }
        /* map_copy0 != map_copy1 || map_copy1 crc error
        else if (ret == 1){

        }
        */

        ret = check_secure_storage_map(&secure_storage_map);
        if (ret == -1) {
            memset(&secure_storage_map, 0, SEC_BLK_SIZE);
        } else if (ret == 2) {
            if ((secure_storage_map.data[0] == 0xff) ||
                (secure_storage_map.data[0] == 0x00)) {
                SST_DBG_INFO("the secure storage map is empty\n");
                memset(&secure_storage_map, 0, SEC_BLK_SIZE);
            } else {
                // no things
            }
        }
    }

    secure_storage_inited = 1;
    return 0;
}

int sunxi_secure_storage_exit(int mode)
{
    int ret;
    if (!secure_storage_inited) {
        SST_DBG_ERR("err: secure storage has not been inited\n");
        return -1;
    }
    if (mode || try_map_dirty()) {
        secure_storage_map.magic = STORE_OBJECT_MAGIC;
        secure_storage_map.crc =
            crc32(0, (void *)&secure_storage_map, sizeof(struct map_info) - 4);
        ret = sunxi_secstorage_write(0, (void *)&secure_storage_map,
                                     SEC_BLK_SIZE);
        if (ret < 0) {
            printf("write secure storage map\n");
            pthread_mutex_unlock(&sec_mutex);
            return -1;
        }
    }
    secure_storage_inited = 0;
    clear_map_dirty();
    pthread_mutex_unlock(&sec_mutex);

    return 0;
}

/*
 * Check the platform support secure storage or not
 * If yes return 0, otherwise return -1 ;
 *
 */
int secure_storage_support(void)
{
    /*Check the map read */
    if (sunxi_secure_storage_init() < 0) {
        SST_DBG_ERR("Don't support secure storage");
        return -1;
    }
    sunxi_secure_storage_exit(0);

    return 0;
}
