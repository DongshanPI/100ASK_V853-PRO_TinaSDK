#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <signal.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/kernel.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "include/g2d_driver_enh.h"
#include "include/ion_head.h"

#define RGB_IMAGE_NAME "/mnt/UDISK/c1080_good.rgb"
#define Y8_IMAGE_NAME "/mnt/UDISK/en_dmabuf_bike_1280x720_220_Y8.bin"
#define NV12_IMAGE_NAME "/mnt/UDISK/bike_1280x720_220.bin"

#define FRAME_TO_BE_PROCESS 16
/*4 rgb convert 6 Y8 convert 6 yuv420 convert*/
unsigned int out_width[FRAME_TO_BE_PROCESS] = {
	192,  154,  108,  321,  447,  960, 241, 320,
	1920, 1439, 1280, 1920, 2048, 720, 800, 480};
unsigned int out_height[FRAME_TO_BE_PROCESS] = {
	108,  87,  70,   217, 213, 640,
	840,  240, 1080, 777, 800, 1080,
	2048, 480, 480,  240};

struct test_info_t
{
	struct mixer_para info[FRAME_TO_BE_PROCESS];
	int fd;
	int ion_fd;
	char dst_image_name[FRAME_TO_BE_PROCESS][100];
	char src_image_name[FRAME_TO_BE_PROCESS][100];
	FILE *src_fp;
	FILE *dst_fp[FRAME_TO_BE_PROCESS];
	struct ion_info src_ion[FRAME_TO_BE_PROCESS];
	struct ion_info dst_ion[FRAME_TO_BE_PROCESS];
	int out_size[FRAME_TO_BE_PROCESS];
	int in_size[FRAME_TO_BE_PROCESS];
};


struct test_info_t test_info;

/* Signal handler */
static void terminate(int sig_no)
{
	int val[6];
	memset(val,0,sizeof(val));
	printf("Got signal %d, exiting ...\n", sig_no);
	if(test_info.fd == -1)
	{
			printf("EXIT:");
			exit(1);
	}

	close(test_info.fd);
	printf("EXIT:");
	exit(1);
}

int parse_cmdline(int argc, char **argv, struct test_info_t *p)
{
	int err = 0;
	int i = 0;
	int b = 0;

	while (i < argc) {
		printf("%s ", argv[i]);
		i++;
	}
	printf("\n");

	i = 0;
	while(i < argc) {
		if ( ! strcmp(argv[i], "-src_rgb_file")) {
			if (argc > i+1) {
				i++;
				for (b = 0; b < 4; b++) {
					p->src_image_name[b][0] = '\0';
					sprintf(p->src_image_name[b], "%s", argv[i]);
				}
				printf("src_rgb_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_rgb_file")) {
			if (argc > i+1) {
				i++;
				for (b = 0; b < 4; b++) {
				p->dst_image_name[b][0] = '\0';
				sprintf(p->dst_image_name[b], "%s", argv[i]);
				}
				printf("dst_rgb_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-src_y8_file")) {
			if (argc > i+1) {
				i++;
				for (b = 4; b < 10; b++) {
					p->src_image_name[b][0] = '\0';
					sprintf(p->src_image_name[b], "%s", argv[i]);
				}
				printf("src_y8_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_y8_file")) {
			if (argc > i+1) {
				i++;
				for (b = 4; b < 10; b++) {
				p->dst_image_name[b][0] = '\0';
				sprintf(p->dst_image_name[b], "%s", argv[i]);
				}
				printf("dst_y8_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-src_nv12_file")) {
			if (argc > i+1) {
				i++;
				for (b = 10; b < 16; b++) {
					p->src_image_name[b][0] = '\0';
					sprintf(p->src_image_name[b], "%s", argv[i]);
				}
				printf("src_nv12_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_nv12_file")) {
			if (argc > i+1) {
				i++;
				for (b = 10; b < 16; b++) {
				p->dst_image_name[b][0] = '\0';
				sprintf(p->dst_image_name[b], "%s", argv[i]);
				}
				printf("dst_nv12_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		i++;
	}

	if(err > 0) {
		printf("example : ./g2d_mixer_task -src_rgb_file /mnt/UDISK/c1080_good.rgb -dst_rgb_file /mnt/UDISK \n");
		return -1;
	} else
		 return 0;
}

static void install_sig_handler(void)
{
	signal(SIGBUS, terminate);
	signal(SIGFPE, terminate);
	signal(SIGHUP, terminate);
	signal(SIGILL, terminate);
	signal(SIGINT, terminate);
	signal(SIGIOT, terminate);
	signal(SIGPIPE, terminate);
	signal(SIGQUIT, terminate);
	signal(SIGSEGV, terminate);
	signal(SIGSYS, terminate);
	signal(SIGTERM, terminate);
	signal(SIGTRAP, terminate);
	signal(SIGUSR1, terminate);
	signal(SIGUSR2, terminate);
}


#define DISPALIGN(value, align) ((align==0)?(unsigned long)value:((((unsigned long)value) + ((align) - 1)) & ~((align) - 1)))

/**
 * @name       :disp_layer_is_iommu_enabled
 * @brief      :judge whether iommu is enabled
 * @return     :1 if iommu enabled or 0 if iommu disable
 */
static int is_iommu_enabled(void)
{
	struct stat iommu_sysfs;
	if (stat("/sys/class/iommu", &iommu_sysfs) == 0 &&
	    S_ISDIR(iommu_sysfs.st_mode))
		return 1;
	else
		return 0;
}

void ion_flush_cache(int fd)
{
	printf("IonFlushCache!\n");
	struct dma_buf_sync sync;
	int ret;

	/* START may need to use fix me */
	sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
	/* clean and invalid user cache */

	ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (ret) {
		printf("DMA_BUF_IOCTL_SYNC failed \n");
	}

	return;
}

int ion_open(void)
{
	int fd = open("/dev/ion", O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		printf("open ion device failed!%s\n", strerror(errno));
	printf("[%s]: success fd = %d\n", __func__, fd);
	return fd;
}

int ion_memory_request(struct ion_info *ion, unsigned int mem_size)
{
	struct ion_allocation_data data;
	int ret = -1;
	int ion_fd = 0;;

	if (test_info.ion_fd <= 0) {
		test_info.ion_fd = ion_open();
		if (test_info.ion_fd < 0) {
			return -1;
		}
	}
	ion_fd = test_info.ion_fd;

	printf("%s(%d): ion_fd %d\n", __func__, __LINE__, ion_fd);

	/* alloc buffer */
	if (is_iommu_enabled())
		data.heap_id_mask = ION_SYSTEM_HEAP_MASK;
	else
		data.heap_id_mask = ION_DMA_HEAP_MASK;

	data.len = mem_size;
	data.flags = ION_FLAG_CACHED;

	printf("%s(%d): ION_HEAP_TYPE 0x%x\n", __func__, __LINE__, data.heap_id_mask);

	ret = ioctl(ion_fd, ION_IOC_ALLOC, &data);
	if(ret < 0) {
		printf("%s(%d): ION_IOC_ALLOC err, ret = %d\n", __func__, __LINE__, ret);
		data.fd = -1;
		goto out;
	}
	printf("%s(%d): ION_IOC_ALLOC succes, dmabuf-fd = %d, size = %d\n",
				__func__, __LINE__, ret, data.len);
	printf("\n");

	ion->virt_addr = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, data.fd, 0);

	if (ion->virt_addr == MAP_FAILED) {
		printf("%s(%d): mmap err, ret %p\n", __func__, __LINE__, ion->virt_addr);
		data.fd = -1;
	}

	ion->alloc_data = data;
out:
	return data.fd;
}

void ion_memory_release(int fd)
{
	close(fd);
	return;
}


int main(int argc, char **argv)
{
	unsigned long arg[6];
	int i, n, rv;
	int ret = -1;
	int src_fd = 0, dst_fd = 0;
	char dst_file_name[100];

	install_sig_handler();
	memset(&test_info, 0, sizeof(struct test_info_t));

	rv = parse_cmdline(argc, argv, &test_info);
	if(rv < 0) {
		printf("parse_command fail");
		return -1;
	}

	if((test_info.fd = open("/dev/g2d",O_RDWR)) == -1) {
		printf("open g2d device fail!\n");
		close(test_info.fd);
		return -1;
	}

	/* we will scaler 1 image to 5 different resolution image
	 * source image is same image*/
	test_info.info[0].flag_h = G2D_BLT_NONE_H;
	test_info.info[0].op_flag = OP_BITBLT;
	test_info.info[0].src_image_h.format = G2D_FORMAT_RGB888;
	test_info.info[0].src_image_h.width = 1920;
	test_info.info[0].src_image_h.height = 1080;
	test_info.info[0].src_image_h.clip_rect.x = 0;
	test_info.info[0].src_image_h.clip_rect.y = 0;
	test_info.info[0].src_image_h.clip_rect.w = 1920;
	test_info.info[0].src_image_h.clip_rect.h = 1080;
	test_info.info[0].src_image_h.color = 0xee8899;
	test_info.info[0].src_image_h.mode = G2D_PIXEL_ALPHA;
	test_info.info[0].src_image_h.alpha = 0xaa;
	test_info.info[0].src_image_h.align[0] = 0;
	test_info.info[0].src_image_h.align[1] = 0;
	test_info.info[0].src_image_h.align[2] = 0;

	test_info.info[0].dst_image_h.format = G2D_FORMAT_RGB888;
	test_info.info[0].dst_image_h.width = 800;
	test_info.info[0].dst_image_h.height = 480;
	test_info.info[0].dst_image_h.clip_rect.x = 0;
	test_info.info[0].dst_image_h.clip_rect.y = 0;
	test_info.info[0].dst_image_h.clip_rect.w = 1920;
	test_info.info[0].dst_image_h.clip_rect.h = 1080;
	test_info.info[0].dst_image_h.color = 0xee8899;
	test_info.info[0].dst_image_h.mode = G2D_PIXEL_ALPHA;
	test_info.info[0].dst_image_h.alpha = 255;
	test_info.info[0].dst_image_h.align[0] = 0;
	test_info.info[0].dst_image_h.align[1] = 0;
	test_info.info[0].dst_image_h.align[2] = 0;

	for (i = 0; i < FRAME_TO_BE_PROCESS; ++i) {
		memcpy(&test_info.info[i], &test_info.info[0],
		       sizeof(struct mixer_para));
		test_info.info[i].dst_image_h.width = out_width[i];
		test_info.info[i].dst_image_h.height = out_height[i];
		test_info.info[i].dst_image_h.clip_rect.w = out_width[i];
		test_info.info[i].dst_image_h.clip_rect.h = out_height[i];
		if (i < 4) {
			test_info.out_size[i] = test_info.info[i].dst_image_h.width * test_info.info[i].dst_image_h.height * 3;
			test_info.info[i].src_image_h.format = G2D_FORMAT_BGR888;
			test_info.info[i].src_image_h.width = 1920;
			test_info.info[i].src_image_h.height = 1080;
			test_info.info[i].src_image_h.clip_rect.w = 1920;
			test_info.info[i].src_image_h.clip_rect.h = 1080;
			test_info.in_size[i] = 1920*1080*3;
			//snprintf(test_info.src_image_name[i], 100,"%s",RGB_IMAGE_NAME);
		} else if (i < 10) {
			test_info.out_size[i] = test_info.info[i].dst_image_h.width * test_info.info[i].dst_image_h.height;
			test_info.info[i].src_image_h.format = G2D_FORMAT_Y8;
			test_info.info[i].src_image_h.width = 1280;
			test_info.info[i].src_image_h.height = 720;
			test_info.info[i].src_image_h.clip_rect.w = 1280;
			test_info.info[i].src_image_h.clip_rect.h = 720;
			test_info.in_size[i] = 1280*720;
			//snprintf(test_info.src_image_name[i], 100,"%s",Y8_IMAGE_NAME);
		} else {
			test_info.out_size[i] = test_info.info[i].dst_image_h.width * test_info.info[i].dst_image_h.height * 2;
			test_info.info[i].src_image_h.format = G2D_FORMAT_YUV420UVC_U1V1U0V0;
			test_info.info[i].src_image_h.width = 1280;
			test_info.info[i].src_image_h.height = 720;
			test_info.info[i].src_image_h.clip_rect.w = 1280;
			test_info.info[i].src_image_h.clip_rect.h = 720;
			test_info.in_size[i] = 1280*720*2;
			//snprintf(test_info.src_image_name[i], 100,"%s",NV12_IMAGE_NAME);
		}
		dst_fd = ion_memory_request(&test_info.dst_ion[i], test_info.out_size[i]);
		test_info.info[i].dst_image_h.fd = dst_fd;
		test_info.info[i].dst_image_h.format = test_info.info[i].src_image_h.format;

		src_fd = ion_memory_request(&test_info.src_ion[i], test_info.in_size[i]);
		if((test_info.src_fp = fopen(test_info.src_image_name[i], "r")) == NULL) {
			printf("open file %s fail. \n", test_info.src_image_name[i]);
			return -1;
		} else
			printf("open file %s ok. \n", test_info.src_image_name[i]);
		fread(test_info.src_ion[i].virt_addr, test_info.in_size[i], 1, test_info.src_fp);
		fclose(test_info.src_fp);

		ion_flush_cache(src_fd);
		test_info.info[i].src_image_h.fd = src_fd;
	}

	arg[0] = (unsigned long)test_info.info;
	arg[1] = FRAME_TO_BE_PROCESS;
	if (ioctl(test_info.fd, G2D_CMD_MIXER_TASK, (arg)) < 0) {
		printf("[%d][%s][%s]G2D_CMD_MIXER_TASK failure!\n", __LINE__,
		       __FILE__, __FUNCTION__);
		goto FREE_SRC;
	}
	printf("[%d][%s][%s]G2D_CMD_MIXER_TASK SUCCESSFULL!\n", __LINE__,
	       __FILE__, __FUNCTION__);


	printf("save result data to file\n");
	char sufix[40] = {0};
	for (i = 0; i < FRAME_TO_BE_PROCESS; ++i) {
		if (i < 4) {
			snprintf(sufix, 40, "rgb888");
		} else if (i < 10)
			snprintf(sufix, 40, "y8");
		else
			snprintf(sufix, 40, "nv12");

		snprintf(dst_file_name, 100,
			 "%s/frame%d_%dx%d_to_%dx%d.%s", test_info.dst_image_name[i], i,
			 test_info.info[i].src_image_h.width,
			 test_info.info[i].src_image_h.height,
			 test_info.info[i].dst_image_h.width,
			 test_info.info[i].dst_image_h.height, sufix);
		if((test_info.dst_fp[i] = fopen(dst_file_name, "wb+")) == NULL) {
			printf("open file %s fail.\n", dst_file_name);
			break;
		} else {
			ret = fwrite(test_info.dst_ion[i].virt_addr,
				     test_info.out_size[i], 1, test_info.dst_fp[i]);
			ion_flush_cache(test_info.info[i].dst_image_h.fd);
			printf("Frame %d saved\n", i);
		}

	}

FREE_DST:
	for (i = 0; i < FRAME_TO_BE_PROCESS; ++i) {
		ion_memory_release(test_info.info[i].dst_image_h.fd);
		if (test_info.dst_fp[i])
			fclose(test_info.dst_fp[i]);
	}
FREE_SRC:
	for (i = 0; i < FRAME_TO_BE_PROCESS; ++i) {
		ion_memory_release(test_info.info[i].src_image_h.fd);
	}
OUT:
	close(test_info.ion_fd);
	close(test_info.fd);

	return 0;
}
