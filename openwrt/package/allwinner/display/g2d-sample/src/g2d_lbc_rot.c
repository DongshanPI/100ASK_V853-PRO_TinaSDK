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

struct test_info_t
{
	int g2d_fd;
	int ion_fd;
	int is_lbc;
	g2d_lbc_rot info;
	char filename[2][64];
	FILE *fp[2];
	struct ion_info src_ion;
	struct ion_info dst_ion;
};

struct test_info_t test_info;

/* Signal handler */
static void terminate(int sig_no)
{
	int val[6];
	memset(val,0,sizeof(val));
	printf("Got signal %d, exiting ...\n", sig_no);
	if(test_info.g2d_fd == -1)
	{
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

	while (i < argc) {
		printf("%s ", argv[i]);
		i++;
	}
	printf("\n");

	i = 0;
	while(i < argc) {
		//rotation mode
		if ( ! strcmp(argv[i], "-flag")) {
			if (argc > i+1) {
				i+=1;
				p->info.blt.flag_h = atoi(argv[i]);
			}
		}

		if ( ! strcmp(argv[i], "-out_fb")) {
			if (argc > i+4) {
				i++;
				p->info.blt.dst_image_h.format = atoi(argv[i]);
				i++;
				p->info.blt.dst_image_h.width = atoi(argv[i]);
				i++;
				p->info.blt.dst_image_h.height = atoi(argv[i]);
			}	else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_rect")) {
			if (argc > i+4) {
				i++;
				p->info.blt.dst_image_h.clip_rect.x = atoi(argv[i]);
				i++;
				p->info.blt.dst_image_h.clip_rect.y = atoi(argv[i]);
				i++;
				p->info.blt.dst_image_h.clip_rect.w = atoi(argv[i]);
				i++;
				p->info.blt.dst_image_h.clip_rect.h = atoi(argv[i]);
			}	else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-in_fb")) {
			if (argc > i+4) {
				i++;
				p->info.blt.src_image_h.format = atoi(argv[i]);
				i++;
				p->info.blt.src_image_h.width = atoi(argv[i]);
				i++;
				p->info.blt.src_image_h.height = atoi(argv[i]);
			}	else {
				printf("-in_fb para err!\n\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-src_rect")) {
			if (argc > i + 4) {
				i++;
				p->info.blt.src_image_h.clip_rect.x = atoi(argv[i]);
				i++;
				p->info.blt.src_image_h.clip_rect.y = atoi(argv[i]);
				i++;
				p->info.blt.src_image_h.clip_rect.w = atoi(argv[i]);
				i++;
				p->info.blt.src_image_h.clip_rect.h = atoi(argv[i]);
			}	else {
				printf("-src_rect para err!\n\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-src_file")) {
			if (argc > i+1) {
				i++;
				p->filename[0][0] = '\0';
				sprintf(p->filename[0], "%s", argv[i]);
				printf("src_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_file")) {
			if (argc > i+1) {
				i++;
				p->filename[1][0] = '\0';
				sprintf(p->filename[1], "%s", argv[i]);
				printf("dst_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if (!strcmp(argv[i], "-lbc")) {
			if (argc > i+1) {
				i+=1;
				p->is_lbc = atoi(argv[i]);
			}
		}

		if (!strcmp(argv[i], "-cmp_ratio")) {
			if (argc > i+1) {
				i+=1;
				p->info.lbc_cmp_ratio = atoi(argv[i]);
			}
		}

		if (!strcmp(argv[i], "-enc_lossy")) {
			if (argc > i+1) {
				i+=1;
				p->info.enc_is_lossy = atoi(argv[i]);
			}
		}

		if (!strcmp(argv[i], "-dec_lossy")) {
			if (argc > i+1) {
				i+=1;
				p->info.dec_is_lossy = atoi(argv[i]);
			}
		}

		i++;
	}

	if(err > 0) {
		printf("example : ./test -flag 1024 -lbc 1 -cmp_ratio 400 -enc_lossy 1 -dec_lossy 1 -in_fb 42 1920 1088 -src_rect 0 0 1920 1088 -out_fb 42 1920 1088 -dst_rect 0 0 1920 1088 -src_file XX -dst_file XX \n");
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

#define G2DALIGN(value, align) ((align==0)?(unsigned long)value:((((unsigned long)value) + ((align) - 1)) & ~((align) - 1)))

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
	int rv;
	int ret;
	int in_size, out_size;
	int fb_width, fb_height;
	int src_fd = 0, dst_fd = 0;

	install_sig_handler();

	memset(&test_info, 0, sizeof(struct test_info_t));
	rv = parse_cmdline(argc, argv, &test_info);
	if(rv < 0) {
		printf("parse_command fail");
		return -1;
	}

	if((test_info.g2d_fd = open("/dev/g2d",O_RDWR)) == -1) {
		printf("open g2d device fail!\n");
		close(test_info.g2d_fd);
		return -1;
	}

	in_size = test_info.info.blt.src_image_h.width * test_info.info.blt.src_image_h.height * 4;
	out_size = test_info.info.blt.dst_image_h.width * test_info.info.blt.dst_image_h.height * 4;
	printf("in_size:%d, out_size=%d\n", in_size, out_size);

	printf("ready to open src file %s \n", test_info.filename[0]);
	src_fd = ion_memory_request(&test_info.src_ion, in_size);
	printf("+++src_fd = %d\n", src_fd);
	if (src_fd <= 0) {
		printf("ion_memory_request src fd failed!\n");
		goto out1;
	}

	if((test_info.fp[0] = fopen(test_info.filename[0], "rt")) == NULL) {
		printf("open file %s fail. \n", test_info.filename[0]);
		ret = -1;
		goto out1;
	} else
		printf("open file %s ok. \n", test_info.filename[0]);
	ret = fread(test_info.src_ion.virt_addr, in_size, 1, test_info.fp[0]);
	printf("read file %s ,ret = %d \n", test_info.filename[0], ret);

	printf("ready to open dst file %s \n", test_info.filename[1]);
	dst_fd = ion_memory_request(&test_info.dst_ion, out_size);
	printf("+++dst_fd = %d\n", dst_fd);
	if (dst_fd <= 0) {
		printf("ion_memory_request dst fd failed!\n");
		goto out2;
	}

	if((test_info.fp[1] = fopen(test_info.filename[1], "wb+")) == NULL) {
		printf("open file %s fail. \n", test_info.filename[1]);
		ret = -1;
		goto out2;
	} else
		printf("open file %s ok. \n", test_info.filename[1]);

	test_info.info.blt.src_image_h.fd = src_fd;
	test_info.info.blt.dst_image_h.fd = dst_fd;

	fb_width = test_info.info.blt.src_image_h.width;
	fb_height = test_info.info.blt.src_image_h.height;
	test_info.info.blt.src_image_h.align[0] = 0;
	test_info.info.blt.src_image_h.align[1] = test_info.info.blt.src_image_h.align[0];
	test_info.info.blt.src_image_h.align[2] = test_info.info.blt.src_image_h.align[0];
	test_info.info.blt.dst_image_h.align[0] = test_info.info.blt.src_image_h.align[0];
	test_info.info.blt.dst_image_h.align[1] = test_info.info.blt.src_image_h.align[0];
	test_info.info.blt.dst_image_h.align[2] = test_info.info.blt.src_image_h.align[0];

	printf("src:format=0x%x, w=%d, h=%d, x=%d, y=%d, image_w=%d, image_h=%d, align=%d\n\n",
			test_info.info.blt.src_image_h.format,
			test_info.info.blt.src_image_h.clip_rect.w,
			test_info.info.blt.src_image_h.clip_rect.h,
			test_info.info.blt.src_image_h.clip_rect.x,
			test_info.info.blt.src_image_h.clip_rect.y,
			test_info.info.blt.src_image_h.width,
			test_info.info.blt.src_image_h.height,
			test_info.info.blt.src_image_h.align[0]);

	fb_width = test_info.info.blt.dst_image_h.width;
	fb_height = test_info.info.blt.dst_image_h.height;

	printf("dst:format=0x%x, img w=%d, h=%d, rect x=%d, y=%d, w=%d, h=%d, align=%d\n\n",
			test_info.info.blt.dst_image_h.format,
			test_info.info.blt.dst_image_h.width,
			test_info.info.blt.dst_image_h.height,
			test_info.info.blt.dst_image_h.clip_rect.x,
			test_info.info.blt.dst_image_h.clip_rect.y,
			test_info.info.blt.dst_image_h.clip_rect.w,
			test_info.info.blt.dst_image_h.clip_rect.h,
			test_info.info.blt.dst_image_h.align[0]);
	printf("[G2D] LBC=%d\n", test_info.is_lbc);

	sleep(1);
	/* rotation */
	if (test_info.is_lbc) {
		if(ioctl(test_info.g2d_fd , G2D_CMD_LBC_ROT ,(unsigned long)(&test_info.info)) < 0)
		{
			printf("[%d][%s][%s]G2D_CMD_LBC_ROT failure!\n",__LINE__, __FILE__,__FUNCTION__);
			ret = -1;
			goto exit;
		}
		printf("[%d][%s][%s]G2D_CMD_LBC_ROT Successful!\n",__LINE__, __FILE__,__FUNCTION__);
	} else {
		if(ioctl(test_info.g2d_fd , G2D_CMD_BITBLT_H ,(unsigned long)(&test_info.info.blt)) < 0)
		{
			printf("[%d][%s][%s]G2D_CMD_BITBLT_H failure!\n",__LINE__, __FILE__,__FUNCTION__);
			ret = -1;
			goto exit;
		}
		printf("[%d][%s][%s]G2D_CMD_BITBLT_H Successful!\n",__LINE__, __FILE__,__FUNCTION__);
	}
	/* save result data to file */
	printf("save result data to file %s \n", test_info.filename[1]);
	/* save result data to file */
	printf("==out_size=%d, addr=%p==\n", out_size, test_info.dst_ion.virt_addr);
	ret = fwrite(test_info.dst_ion.virt_addr, out_size, 1, test_info.fp[1]);
	printf("fwrite,ret=%d\n", ret);

	ret = 0;

exit:
	munmap(test_info.dst_ion.virt_addr, out_size);
out2:
	ion_memory_release(dst_fd);
	fclose(test_info.fp[1]);

out1:
	munmap(test_info.src_ion.virt_addr, in_size);
	ion_memory_release(src_fd);
	fclose(test_info.fp[0]);
	close(test_info.ion_fd);
	close(test_info.g2d_fd);

	return ret;
}


