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
	g2d_fillrect_h info;
	int fd;
	int ion_fd;
	char filename[64];
	char out_filename[64];
	FILE *fp;
	struct ion_info dst_ion;
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

	while(i<argc) {
		printf("%s ",argv[i]);
		i++;
	}
	printf("\n");

	i = 0;
	while(i < argc) {
		if ( ! strcmp(argv[i], "-out_fb")) {
			if (argc > i+4) {
				i++;
				p->info.dst_image_h.format = atoi(argv[i]);
				i++;
				p->info.dst_image_h.width = atoi(argv[i]);
				i++;
				p->info.dst_image_h.height = atoi(argv[i]);
			}	else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-dst_rect")) {
			if (argc > i+4) {
				i++;
				p->info.dst_image_h.clip_rect.x = atoi(argv[i]);
				i++;
				p->info.dst_image_h.clip_rect.y = atoi(argv[i]);
				i++;
				p->info.dst_image_h.clip_rect.w = atoi(argv[i]);
				i++;
				p->info.dst_image_h.clip_rect.h = atoi(argv[i]);
			}	else {
				printf("-out_fb para err!\n\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-out_file")) {
			if (argc > i+1) {
				i++;
				p->out_filename[0] = '\0';
				sprintf(p->out_filename,"%s",argv[i]);
				printf("out_file=%s\n", argv[i]);
			}	else {
				printf("no file described!!\n");
				err ++;
			}
		}

		if ( ! strcmp(argv[i], "-color")) {
			if (argc > i+1) {
				i+=1;
				p->info.dst_image_h.color = atoll(argv[i]);
			}
		}

		if ( ! strcmp(argv[i], "-alpha_mode")) {
			if (argc > i+1) {
				i+=1;
				p->info.dst_image_h.mode = atoi(argv[i]);
			}
		}

		if ( ! strcmp(argv[i], "-alpha")) {
			if (argc > i+1) {
				i+=1;
				p->info.dst_image_h.alpha = atoi(argv[i]);
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

int ion_open(void)
{
	int fd = open("/dev/ion", O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		printf("open ion device failed!%s\n", strerror(errno));
	printf("[%s]: success fd = %d\n", __func__, fd);
	return fd;
}
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


int main(int argc, char **argv)
{
	unsigned long arg[6];
	int rv;
	int i,n;
	int ret;
	int out_size;
	int dst_fd = 0;
	int fb_width, fb_height;

	install_sig_handler();
	memset(&test_info, 0, sizeof(struct test_info_t));
	rv = parse_cmdline(argc,argv, &test_info);
	if(rv < 0) {
		printf("parse_command fail");
		return -1;
	}

	if((test_info.fd = open("/dev/g2d",O_RDWR)) == -1) {
		printf("open g2d device fail!\n");
		close(test_info.fd);
		return -1;
	}

	out_size = test_info.info.dst_image_h.width * test_info.info.dst_image_h.height * 4;
	printf("out_size=%d\n", out_size);

	printf("ready to open dst file %s \n", test_info.filename);
	dst_fd = ion_memory_request(&test_info.dst_ion, out_size);
	if(dst_fd <= 0)
		printf("request dst_mem failed! \n");

	test_info.info.dst_image_h.fd = dst_fd;

	fb_width = test_info.info.dst_image_h.width;
	fb_height = test_info.info.dst_image_h.height;

	printf("dst:format=0x%x,color=0x%x \n img w=%d, h=%d, \n rect x=%d, y=%d, w=%d, h=%d, align=%d\n\n",
		test_info.info.dst_image_h.format, test_info.info.dst_image_h.color,
		test_info.info.dst_image_h.width, test_info.info.dst_image_h.height,
		test_info.info.dst_image_h.clip_rect.x, test_info.info.dst_image_h.clip_rect.y,
		test_info.info.dst_image_h.clip_rect.w, test_info.info.dst_image_h.clip_rect.h,
		test_info.info.dst_image_h.align[0]);

	sleep(1);
	/* fill color */
	if(ioctl(test_info.fd , G2D_CMD_FILLRECT_H ,(unsigned long)(&test_info.info)) < 0)
	{
		printf("[%d][%s][%s]G2D_CMD_FILLRECT_H failure!\n",__LINE__, __FILE__,__FUNCTION__);
		ion_memory_release(dst_fd);
		close(test_info.fd);

		return -1;
	}
		printf("[%d][%s][%s]G2D_CMD_FILLRECT_H Successful!\n",__LINE__, __FILE__,__FUNCTION__);

	/* save result data to file */
	printf("save result data to file %s \n", test_info.out_filename);
	/* save result data to file */
	if((test_info.fp = fopen(test_info.out_filename, "wb+")) == NULL) {
		printf("open file %s fail. \n", test_info.out_filename);
		ion_memory_release(dst_fd);
		return -1;
	} else {
		printf("open file %s ok. \n", test_info.out_filename);
	}
	printf("==out_size=%d, addr=%p==\n", out_size, test_info.dst_ion.virt_addr);
	ret = fwrite(test_info.dst_ion.virt_addr, out_size, 1, test_info.fp);
	printf("fwrite,ret=%d\n", ret);
	munmap(test_info.dst_ion.virt_addr, out_size);
	fclose(test_info.fp);

	ion_memory_release(dst_fd);
	close(test_info.fd);

	return 0;
}

