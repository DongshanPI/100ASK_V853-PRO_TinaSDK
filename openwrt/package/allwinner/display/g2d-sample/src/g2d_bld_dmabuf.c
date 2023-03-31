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
#include <sys/ioctl.h>
#include <errno.h>
#include "include/g2d_driver_enh.h"
#include "include/ion_head.h"

struct test_info_t
{
	int g2d_fd;
	int ion_fd;
	g2d_bld info;
	char filename[3][64];
	FILE *fp[3];
	struct ion_info src_ion;
	struct ion_info src_ion2;
	struct ion_info dst_ion;
};

struct test_info_t test_info;

/* Signal handler */
static void terminate(int sig_no)
{
	int val[6];
	memset(val,0,sizeof(val));
	printf("Got signal %d, exiting ...\n", sig_no);
	if(test_info.g2d_fd == -1) {
		printf("EXIT:");
		exit(1);
	}

	close(test_info.g2d_fd);
	printf("EXIT:");
	exit(1);
}

int parse_cmdline(int argc, char **argv, struct test_info_t *p)
{
	int err = 0;
	int i = 0;

	while(i<argc) {
		printf("%s ",argv[i]);
		i++;
	}
	printf("\n");

	i = 0;
	while(i < argc) {
		if ( ! strcmp(argv[i], "-flag")) {
			if (argc > i+1) {
				i+=1;
				p->info.bld_cmd = atoi(argv[i]);
			}
		}

		if (!strcmp(argv[i], "-out_fb")) {
			if (argc > i + 3) {
				i++;
				p->info.dst_image.format = atoi(argv[i]);
				i++;
				p->info.dst_image.width = atoi(argv[i]);
				i++;
				p->info.dst_image.height = atoi(argv[i]);
			} else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-dst_rect")) {
			if (argc > i + 4) {
				i++;
				p->info.dst_image.clip_rect.x = atoi(argv[i]);
				i++;
				p->info.dst_image.clip_rect.y = atoi(argv[i]);
				i++;
				p->info.dst_image.clip_rect.w = atoi(argv[i]);
				i++;
				p->info.dst_image.clip_rect.h = atoi(argv[i]);
			} else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-in_fb")) {
			if (argc > i + 3) {
				i++;
				p->info.src_image[0].format = atoi(argv[i]);
				i++;
				p->info.src_image[0].width = atoi(argv[i]);
				i++;
				p->info.src_image[0].height = atoi(argv[i]);
			} else {
				printf("-in_fb para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_rect")) {
			if (argc > i + 4) {
				i++;
				p->info.src_image[0].clip_rect.x = atoi(argv[i]);
				i++;
				p->info.src_image[0].clip_rect.y = atoi(argv[i]);
				i++;
				p->info.src_image[0].clip_rect.w = atoi(argv[i]);
				i++;
				p->info.src_image[0].clip_rect.h = atoi(argv[i]);
			} else {
				printf("-src_rect para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_coor")) {
			if (argc > i + 2) {
				i++;
				p->info.src_image[0].coor.x = atoi(argv[i]);
				i++;
				p->info.src_image[0].coor.y = atoi(argv[i]);
			} else {
				printf("-src_coor para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_file")) {
			if (argc > i+1) {
				i++;
				p->filename[0][0] = '\0';
				sprintf(p->filename[0], "%s", argv[i]);
				printf("src_file=%s\n", argv[i]);
			} else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-in_fb2")) {
			if (argc > i + 3) {
				i++;
				p->info.src_image[1].format = atoi(argv[i]);
				i++;
				p->info.src_image[1].width = atoi(argv[i]);
				i++;
				p->info.src_image[1].height = atoi(argv[i]);
			} else {
				printf("-in_fb para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_rect2")) {
			if (argc > i + 4) {
				i++;
				p->info.src_image[1].clip_rect.x = atoi(argv[i]);
				i++;
				p->info.src_image[1].clip_rect.y = atoi(argv[i]);
				i++;
				p->info.src_image[1].clip_rect.w = atoi(argv[i]);
				i++;
				p->info.src_image[1].clip_rect.h = atoi(argv[i]);
			} else {
				printf("-src_rect para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_coor2")) {
			if (argc > i + 2) {
				i++;
				p->info.src_image[1].coor.x = atoi(argv[i]);
				i++;
				p->info.src_image[1].coor.y = atoi(argv[i]);
			} else {
				printf("-src_coor para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_file2")) {
			if (argc > i+1) {
				i++;
				p->filename[1][0] = '\0';
				sprintf(p->filename[1], "%s", argv[i]);
				printf("src_file2 = %s\n", argv[i]);
			} else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-dst_file")) {
			if (argc > i+1) {
				i++;
				p->filename[2][0] = '\0';
				sprintf(p->filename[2], "%s", argv[i]);
				printf("dst_file=%s\n", argv[i]);
			} else {
				printf("no file described!!\n");
				err++;
			}
		}

		if (!strcmp(argv[i], "-alpha_mode")) {
			if (argc > i+1) {
				i+=1;
				p->info.src_image[0].mode = atoi(argv[i]);
				p->info.src_image[1].mode = G2D_GLOBAL_ALPHA;
				p->info.dst_image.mode = G2D_GLOBAL_ALPHA;
			}
		}

		if (!strcmp(argv[i], "-alpha")) {
			if (argc > i+1) {
				i+=1;
				p->info.src_image[0].alpha = atoi(argv[i]);
				p->info.src_image[1].alpha = 0x50;
				p->info.dst_image.alpha = 0xff;
			}
		}

		i++;
	}

	if(err > 0) {
		printf("example : --------\n");
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

#define DISP_MEM_REQUEST 0x2c0
#define DISP_MEM_RELEASE 0x2c1
#define DISP_MEM_GETADR 0x2c2

#define DISPALIGN(value, align) ((align==0)?(unsigned long)value:((((unsigned long)value) + ((align) - 1)) & ~((align) - 1)))

static int is_iommu_enabled(void)
{
	struct stat iommu_sysfs;
	if (stat("/sys/class/iommu", &iommu_sysfs) == 0 &&
	    S_ISDIR(iommu_sysfs.st_mode))
		return 1;
	else
		return 0;
}

int ion_open(void)
{
	int fd = open("/dev/ion", O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		printf("open ion device failed!%s\n", strerror(errno));
	printf("[%s]: success fd = %d\n", __func__, fd);
	return fd;
}

int ion_memory_request(struct ion_info *ion, int mem_size)
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
		goto out;
	}
	printf("%s(%d): ION_IOC_ALLOC succes, dmabuf-fd = %d, size = %d\n",
				__func__, __LINE__, ret, data.len);
	printf("\n");

	ion->virt_addr = mmap(NULL, mem_size, PROT_READ|PROT_WRITE, MAP_SHARED, data.fd, 0);

	if (ion->virt_addr == MAP_FAILED) {
		printf("%s(%d): mmap err, ret %p\n", __func__, __LINE__, ion->virt_addr);
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

int main(int argc, char **argv)
{
	unsigned long arg[6];
	int rv;
	int i,n;
	void* virt_addr;
	int ret;
	int in_size, in_size2, out_size;
	int src_fd, src_fd2, dst_fd;

	install_sig_handler();
	memset(&test_info, 0, sizeof(struct test_info_t));
	rv = parse_cmdline(argc,argv, &test_info);
	if(rv < 0) {
		printf("parse_command fail");
		return -1;
	}

	if((test_info.g2d_fd = open("/dev/g2d",O_RDWR)) == -1) {
		printf("open g2d device fail!\n");
		close(test_info.g2d_fd);
		return -1;
	}

	in_size = test_info.info.src_image[0].width * test_info.info.src_image[0].height * 4;
	in_size2 = test_info.info.src_image[1].width * test_info.info.src_image[1].height * 4;
	out_size = test_info.info.dst_image.width * test_info.info.dst_image.height * 4;
	printf("in_size = %d, size2 = %d, out_size=%d\n", in_size, in_size2, out_size);

	printf("ready to open file %s \n", test_info.filename);
	src_fd = ion_memory_request(&test_info.src_ion, in_size);
	if(src_fd <= 0) {
		printf("request src_mem failed! \n");
		return -1;
	}

	if((test_info.fp[0] = fopen(test_info.filename[0], "rt")) == NULL) {
		printf("open file %s fail. \n", test_info.filename[0]);
		ret = -1;
		goto out1;
	} else {
		printf("open file %s ok. \n", test_info.filename[0]);
	}

	ret = fread(test_info.src_ion.virt_addr, in_size, 1, test_info.fp[0]);
	printf("fread src file finish, ret = %d\n", ret);
	munmap(test_info.src_ion.virt_addr, in_size);
	fclose(test_info.fp[0]);

	printf("ready to open file2 %s \n", test_info.filename[1]);
	src_fd2 = ion_memory_request(&test_info.src_ion2, in_size2);
	if(src_fd2 <= 0) {
		printf("request src_mem2 failed! \n");
		ret = -1;
		goto out1;
	}

	if((test_info.fp[1] = fopen(test_info.filename[1], "rt")) == NULL) {
		printf("open file2 %s fail. \n", test_info.filename[1]);
		ret = -1;
		goto out2;
	} else {
		printf("open file2 %s ok. \n", test_info.filename[1]);
	}

	ret = fread(test_info.src_ion2.virt_addr, in_size2, 1, test_info.fp[1]);
	printf("fread src file2 finish, ret = %d\n", ret);

	printf("ready to open dst file %s \n", test_info.filename[2]);
	dst_fd = ion_memory_request(&test_info.dst_ion, out_size);
	if (dst_fd <= 0) {
		printf("request dst_mem failed! \n");
		ret = -1;
		goto out2;
	}

	if((test_info.fp[2] = fopen(test_info.filename[2], "wb+")) == NULL) {
		printf("open dst file %s fail. \n", test_info.filename[2]);
		ret = -1;
		goto out3;
	} else {
		printf("open dst file %s ok. \n", test_info.filename[2]);
	}

	test_info.info.src_image[0].fd = src_fd;
	test_info.info.src_image[1].fd = src_fd2;
	test_info.info.dst_image.fd = dst_fd;

	test_info.info.src_image[0].align[0] = 0;
	test_info.info.src_image[0].align[1] = test_info.info.src_image[0].align[0];
	test_info.info.src_image[0].align[2] = test_info.info.src_image[0].align[0];
	test_info.info.src_image[1].align[0] = 0;
	test_info.info.src_image[1].align[1] = test_info.info.src_image[1].align[0];
	test_info.info.src_image[1].align[2] = test_info.info.src_image[1].align[0];
	test_info.info.dst_image.align[0] = test_info.info.src_image[0].align[0];
	test_info.info.dst_image.align[1] = test_info.info.src_image[0].align[0];
	test_info.info.dst_image.align[2] = test_info.info.src_image[0].align[0];

	printf("src:alpha mode= %d, val= %d\n\n", test_info.info.src_image[0].mode, test_info.info.src_image[0].alpha);

	printf("src:format=0x%x,\n w=%d, h=%d, x=%d, y=%d, \n image_w=%d, image_h=%d, align=%d\n\n",
		test_info.info.src_image[0].format,
		test_info.info.src_image[0].clip_rect.w,
		test_info.info.src_image[0].clip_rect.h,
		test_info.info.src_image[0].clip_rect.x,
		test_info.info.src_image[0].clip_rect.y,
		test_info.info.src_image[0].width,
		test_info.info.src_image[0].height,
		test_info.info.src_image[0].align[0]);
	printf("src2:format=0x%x,\n w=%d, h=%d, x=%d, y=%d, \n image_w=%d, image_h=%d, align=%d\n\n",
		test_info.info.src_image[1].format, test_info.info.src_image[1].clip_rect.w,
		test_info.info.src_image[1].clip_rect.h,
		test_info.info.src_image[1].clip_rect.x,
		test_info.info.src_image[1].clip_rect.y,
		test_info.info.src_image[1].width,
		test_info.info.src_image[1].height,
		test_info.info.src_image[1].align[0]);

	printf("dst:format=0x%x, \n img w=%d, h=%d, \n rect x=%d, y=%d, w=%d, h=%d, align=%d\n\n",
		test_info.info.dst_image.format,
		test_info.info.dst_image.width,
		test_info.info.dst_image.height,
		test_info.info.dst_image.clip_rect.x,
		test_info.info.dst_image.clip_rect.y,
		test_info.info.dst_image.clip_rect.w,
		test_info.info.dst_image.clip_rect.h,
		test_info.info.dst_image.align[0]);

	sleep(1);
	/* rotation */
	if(ioctl(test_info.g2d_fd , G2D_CMD_BLD_H ,(unsigned long)(&test_info.info)) < 0)
	{
		printf("[%d][%s][%s]G2D_CMD_BLD_H failure!\n",__LINE__, __FILE__,__FUNCTION__);
		ret = -1;
		goto out3;
	}
	printf("[%d][%s][%s]G2D_CMD_BLD_H Successful!\n",__LINE__, __FILE__,__FUNCTION__);

	/* save result data to file */
	printf("save result data to file %s \n", test_info.filename[2]);
	ret = fwrite(test_info.dst_ion.virt_addr, out_size, 1, test_info.fp[2]);
	printf("fwrite,ret=%d\n", ret);
	fclose(test_info.fp[2]);

out3:
	munmap(test_info.dst_ion.virt_addr, out_size);
	ion_memory_release(dst_fd);
out2:
	munmap(test_info.src_ion2.virt_addr, in_size2);
	ion_memory_release(src_fd2);
out1:
	munmap(test_info.src_ion.virt_addr, in_size);
	ion_memory_release(src_fd);
	close(test_info.ion_fd);
	close(test_info.g2d_fd);
	return ret;
}

