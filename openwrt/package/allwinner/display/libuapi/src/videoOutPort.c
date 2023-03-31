#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/fb.h>
#include <linux/videodev2.h>
#include <sys/time.h>
#include <ion_mem_alloc.h>
#include <videoOutPort.h>
#include "yuv_rotate.h"

#ifdef __SUNXI_DISPLAY__
#include "sunxi_display_v1.h"
#define CHN_NUM 2
#define ZORDER_MAX 3
#define ZORDER_MID 1
#endif

#ifdef __SUNXI_DISPLAY2__
#include "sunxi_display_v2.h"
#define CHN_NUM 3
#define ZORDER_MAX 11
#define ZORDER_MID 5
#endif

#ifdef SUNXI_DISPLAY_GPU
/*
 * Define the macro SUNXI_DISPLAY_GPU, use GPU rendering, send the RGBA data
 * after rendering to fb1 display, execute export FRAMEBUFFER=/dev/fb1
 * before executing the program, no longer support DispSetEnable, DispSetSrcRect,
 * DispSetZorder and DispSetIoctl functions.
 *
 * Define the macro SUNXI_DISPLAY_GPU and SUNXI_DISPLAY_GPU_BUF, use GPU rendering,
 * send the RGBA data after rendering to layer. No longer support DispSetSrcRect functions.
 * Default use.
 * */
#include "gpu_yuv2rgb.h"
#include <pthread.h>

typedef struct {
	Dstresource outInfo;
	Srcresource srcInfo;
	renderBuf renderBuf;
	pthread_t gpuThreadId;
	pthread_mutex_t mtx;
	pthread_mutex_t initMtx;
	pthread_cond_t cond;
	int gpuThreadRun;
#ifdef SUNXI_DISPLAY_GPU_BUF
	dispOutPort *vout_hd;
	SrcInfo argbSrcInfo;
	char *outArgbBuf[2];
	int outArgbBufFd[2];
	EGLImageKHR outArgbBufImg[2];
#endif
} GpuRenderInfo;
static GpuRenderInfo gpuRenderInfo;
#endif

#define RET_OK 0
#define RET_FAIL (-1)
#define SCREEN_0 0
#define SCREEN_1 1

#define LYL_NUM 4
#define HLAY(chn, lyl) (chn*4+lyl)
#define HD2CHN(hlay) (hlay/4)
#define HD2LYL(hlay) (hlay%4)

#define ZORDER_MIN 0
#define DISP_DEV_NAME		("/dev/disp")

/*#define DISP_DEBUG*/
#define DISP_CLAR(x)	memset(&(x),  0, sizeof(x))
static unsigned char DISP_LOG = 0;
#define DISP_DBG_LOG	 if(DISP_LOG)  printf

unsigned int mLayerStatus[CHN_NUM][LYL_NUM];

static long long GetNowUs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long) tv.tv_sec * 1000000ll + tv.tv_usec;
}

#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
static int LayerConfig(int fd, __DISP_t cmd, disp_layer_config2 *pinfo) {
	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);
	unsigned long args[4] = { 0 };
	unsigned int ret = RET_OK;

	args[0] = SCREEN_0;
	args[1] = (unsigned long) pinfo;
	args[2] = 1;
	ret = ioctl(fd, cmd, args);
	if (RET_OK != ret) {
		DISP_DBG_LOG("fail to get para\n");
		ret = RET_FAIL;
	}
	return ret;
}

static int LayerGetPara(int disp_fd, disp_layer_config2 *pinfo) {
	return LayerConfig(disp_fd, DISP_LAYER_GET_CONFIG2, pinfo);
}

static int LayerSetPara(int disp_fd, disp_layer_config2 *pinfo) {
	return LayerConfig(disp_fd, DISP_LAYER_SET_CONFIG2, pinfo);
}
#else
static int LayerConfig(int fd, __DISP_t cmd, disp_layer_config *pinfo) {
	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);
	unsigned long args[4] = { 0 };
	unsigned int ret = RET_OK;

	args[0] = SCREEN_0;
	args[1] = (unsigned long) pinfo;
	args[2] = 1;
	ret = ioctl(fd, cmd, args);
	if (RET_OK != ret) {
		DISP_DBG_LOG("fail to get para\n");
		ret = RET_FAIL;
	}
	return ret;
}

static int LayerGetPara(int disp_fd, disp_layer_config *pinfo)
{
	return LayerConfig(disp_fd, DISP_LAYER_GET_CONFIG, pinfo);
}

static int LayerSetPara(int disp_fd, disp_layer_config *pinfo)
{
	return LayerConfig(disp_fd, DISP_LAYER_SET_CONFIG, pinfo);
}
#endif

static int LayerRender(int fd, unsigned int hlay, renderBuf *picture) {
	int ret;
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif

	config.channel = HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	LayerGetPara(fd, &config);
#ifdef CONF_KERNEL_IOMMU
	config.info.fb.fd = picture->fd;
#else
	config.info.fb.addr[0] = picture->y_phaddr;
	config.info.fb.addr[1] = picture->u_phaddr;
	config.info.fb.addr[2] = picture->v_phaddr;
#endif
	ret = LayerSetPara(fd, &config);

	return ret;
}

static int layer_set_src(int fd, unsigned int hlay, SrcInfo *src,
        renderBuf *picture) {
	unsigned long args[4] = { 0 };
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif

	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);

	config.channel = HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	LayerGetPara(fd, &config);

	config.info.fb.crop.x = src->crop_x;
	config.info.fb.crop.y = src->crop_y;
	config.info.fb.crop.width = (src->crop_w);
	config.info.fb.crop.height = (src->crop_h);

	DISP_DBG_LOG("width: 0x%llx, height: 0x%llx", config.info.fb.crop.width,
	        config.info.fb.crop.height);

	config.info.fb.crop.x = config.info.fb.crop.x << 32;
	config.info.fb.crop.y = config.info.fb.crop.y << 32;
	config.info.fb.crop.width = config.info.fb.crop.width << 32;
	config.info.fb.crop.height = config.info.fb.crop.height << 32;

	DISP_DBG_LOG("width: 0x%llx, height: 0x%llx\n", config.info.fb.crop.width,
	        config.info.fb.crop.height);

	config.info.fb.size[0].width = src->w;
	config.info.fb.size[0].height = src->h;
	config.info.fb.color_space = (disp_color_space) src->color_space;
	switch (src->format) {
	case VIDEO_PIXEL_FORMAT_YUV_PLANER_420:
		config.info.fb.format = DISP_FORMAT_YUV420_P;
		config.info.fb.size[1].width = src->w / 2;
		config.info.fb.size[1].height = src->h / 2;
		config.info.fb.size[2].width = src->w / 2;
		config.info.fb.size[2].height = src->h / 2;
		break;

	case VIDEO_PIXEL_FORMAT_YV12:
		config.info.fb.format = DISP_FORMAT_YUV420_P;
		config.info.fb.size[1].width = src->w / 2;
		config.info.fb.size[1].height = src->h / 2;
		config.info.fb.size[2].width = src->w / 2;
		config.info.fb.size[2].height = src->h / 2;
		break;

	case VIDEO_PIXEL_FORMAT_NV12:
		config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
		config.info.fb.size[1].width = src->w / 2;
		config.info.fb.size[2].width = src->w / 2;
		config.info.fb.size[1].height = src->h / 2;
		config.info.fb.size[2].height = src->h / 2;
		break;

	case VIDEO_PIXEL_FORMAT_NV21:
		config.info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
		config.info.fb.size[1].width = src->w / 2;
		config.info.fb.size[2].width = src->w / 2;
		config.info.fb.size[1].height = src->h / 2;
		config.info.fb.size[2].height = src->h / 2;
		break;

	case VIDEO_PIXEL_FORMAT_YUYV:
		config.info.fb.format = DISP_FORMAT_YUV422_I_YUYV;
		config.info.fb.size[1].width = src->w / 2;
		config.info.fb.size[2].width = src->w / 2;
		config.info.fb.size[1].height = src->h / 2;
		config.info.fb.size[2].height = src->h / 2;
		break;

	default:
		config.info.fb.format = DISP_FORMAT_ARGB_8888;
		config.info.fb.size[1].width = src->w;
		config.info.fb.size[1].height = src->h;
		config.info.fb.size[2].width = src->w;
		config.info.fb.size[2].height = src->h;
		break;
	}
	DISP_DBG_LOG("set fb.format %d %d, color_space %d end\n", src->format,
	        config.info.fb.format, config.info.fb.color_space);

#if defined(CONF_KERNEL_VERSION_4_9) || defined(CONF_KERNEL_VERSION_5_4)
	/* When setting the buffer address in linux4.9, the layer must be enabled,
	 * otherwise the set buffer address will be cleared */
	config.enable = 1;
#endif

#if defined(CONF_KERNEL_VERSION_5_4)
	DISP_DBG_LOG("lbc_mode = %d, width = %d, height = %d", src->lbc_mod, src->w,
	        src->h);
	if (src->lbc_mod > 0) {
		u32 seg_width = 16, seg_height = 4;
		u32 bit_depth = 8;
		u32 cmp_ratio = 0;
		u32 seg_tar_bits, seg_tar_bits_out;
		int y_mode_bits, c_mode_bits, y_data_bits, c_data_bits;
		u32 segline_tar_bits = 0, segline_tar_bits_out;
		int frm_width = src->w;

		if (src->is_10bit_flag == 1) {
			bit_depth = 10;
		}

		if (src->lbc_mod == 1) //1.5x
		        {
			cmp_ratio = 666;
		} else if (src->lbc_mod == 2) //2x
		        {
			cmp_ratio = 500;
		} else if (src->lbc_mod == 3) //2.5x
		        {
			cmp_ratio = 400;
		}

		config.info.fb.lbc_en = 1;
		config.info.fb.lbc_info.is_lossy = src->is_lossy;
		config.info.fb.lbc_info.rc_en = src->rc_en;

		if (config.info.fb.lbc_info.is_lossy) {
			seg_tar_bits = ((seg_width * seg_height * bit_depth * cmp_ratio * 3
			        / 2000) / ALIGN) * ALIGN;
			if (config.info.fb.lbc_info.rc_en == 0) {
				segline_tar_bits = ((frm_width + seg_width - 1) / seg_width)
				        * seg_tar_bits;
			} else if (config.info.fb.lbc_info.rc_en == 1) {
				segline_tar_bits = ((frm_width + seg_width - 1) / seg_width
				        * seg_width * seg_height * bit_depth * cmp_ratio * 3
				        / 2000 + ALIGN - 1) / ALIGN * ALIGN;
			}
		} else {
			y_mode_bits = seg_width / MB_WTH
			        * (CODE_MODE_Y_BITS * 2 + BLK_MODE_BITS);
			c_mode_bits = 2 * (seg_width / 2 / MB_WTH * CODE_MODE_C_BITS);
			y_data_bits = seg_width * seg_height * bit_depth;
			c_data_bits = seg_width * seg_height * bit_depth
			        / 2+ 2 * (seg_width / 2 / MB_WTH) * C_DTS_BITS;
			seg_tar_bits = (y_data_bits + c_data_bits + y_mode_bits
			        + c_mode_bits + ALIGN - 1) / ALIGN * ALIGN;
			segline_tar_bits = ((frm_width + seg_width - 1) / seg_width
			        * seg_width / seg_width) * seg_tar_bits;
		}

		seg_tar_bits_out = seg_tar_bits / 8;
		segline_tar_bits_out = segline_tar_bits / 8;

		config.info.fb.lbc_info.pitch = segline_tar_bits_out;
		config.info.fb.lbc_info.seg_bit = seg_tar_bits_out;
	}
#endif
#ifdef CONF_KERNEL_IOMMU
	config.info.fb.fd = picture->fd;
#else
	config.info.fb.addr[0] = picture->y_phaddr;
	config.info.fb.addr[1] = picture->u_phaddr;
	config.info.fb.addr[2] = picture->v_phaddr;
#endif

	return LayerSetPara(fd, &config);
}

int LayerRequest(int *pCh, int *pId) {
	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);
	int ch;
	int id;
	{
#if defined(LUAPI_LAYER_ALLOC_CHN)
		for (id = 0; id < LYL_NUM; id++) {
			for (ch = 0; ch < CHN_NUM; ch++) {
#else
		for (ch = 0; ch < CHN_NUM; ch++) {
			for (id = 0; id < LYL_NUM; id++) {
#endif
				if (!(mLayerStatus[ch][id] & LAYER_STATUS_REQUESTED)) {
					mLayerStatus[ch][id] |= LAYER_STATUS_REQUESTED;
					goto out;
				}
			}
		}
	}
	out: if ((ch == CHN_NUM) && (id == LYL_NUM)) {
		DISP_DBG_LOG("all layer used.");
		return RET_FAIL;
	}
	*pCh = ch;
	*pId = id;
	DISP_DBG_LOG("requested: ch:%d, id:%d\n", ch, id);
	return HLAY(ch, id);
}

int LayerClose(int fd, unsigned int hlay) {
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	config.channel = HD2CHN(hlay);
	config.layer_id = HD2LYL(hlay);
	LayerGetPara(fd, &config);
	config.enable = 0;
	return LayerSetPara(fd, &config);
}

int LayerRelease(int fd, unsigned int hlay) {
	int chn = HD2CHN(hlay);
	int lyl = HD2LYL(hlay);
	int ret = LayerClose(fd, hlay);
	if (RET_OK == ret) {
		if (chn >= 0 && lyl >= 0 && mLayerStatus[chn][lyl]) {
			mLayerStatus[chn][lyl] = 0;
		}
	}
	return ret;
}
#else
static int LayerConfig(int fd, __disp_cmd_t cmd, unsigned int hlay,
        disp_layer_info *pinfo) {
	unsigned long args[4] = { 0 };
	unsigned int ret = RET_OK;
	args[0] = SCREEN_0;
	args[1] = HD2CHN(hlay);
	args[2] = (unsigned long) pinfo;
	ret = ioctl(fd, cmd, args);
	if (RET_OK != ret) {
		DISP_DBG_LOG("fail to get para\n");
		ret = RET_FAIL;
	}
	return ret;
}

static int LayerGetPara(int disp_fd, unsigned int hlay, disp_layer_info *pinfo) {
	return LayerConfig(disp_fd, DISP_CMD_LAYER_GET_INFO, hlay, pinfo);
}

static int LayerSetPara(int disp_fd, unsigned int hlay, disp_layer_info *pinfo) {
	return LayerConfig(disp_fd, DISP_CMD_LAYER_SET_INFO, hlay, pinfo);
}

int LayerRequest(int *pCh, int *pId) {
	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);
	int ch;
	int id;
#if defined(LUAPI_LAYER_ALLOC_CHN)
		for (id = 0; id < LYL_NUM; id++) {
			for (ch = 0; ch < CHN_NUM; ch++) {
#else
	for (ch = 0; ch < CHN_NUM; ch++) {
		for (id = 0; id < LYL_NUM; id++) {
#endif
			if (!(mLayerStatus[ch][id] & LAYER_STATUS_REQUESTED)) {
				mLayerStatus[ch][id] |= LAYER_STATUS_REQUESTED;
				goto out;
			}
		}
	}
	out: if ((ch == CHN_NUM) && (id == LYL_NUM)) {
		DISP_DBG_LOG("all layer used.");
		return RET_FAIL;
	}
	*pCh = ch;
	*pId = id;
	DISP_DBG_LOG("requested: ch:%d, id:%d\n", ch, id);
	return HLAY(ch, id);
}

int LayerRelease(int fd, unsigned int hlay) {
	int ret;
	int chn = HD2CHN(hlay);
	int lyl = HD2LYL(hlay);
	ret = disp_layer_off(fd, hlay);
	if (chn >= 0 && lyl >= 0 && mLayerStatus[chn][lyl])
		mLayerStatus[chn][lyl] = 0;

	return ret;
}

static int LayerRender(int fd, unsigned int hlay, renderBuf *picture) {
	int ret;
	disp_layer_info config;
	memset(&config, 0, sizeof(disp_layer_info));
	LayerGetPara(fd, hlay, &config);
	config.fb.addr[0] = picture->y_phaddr;
	config.fb.addr[1] = picture->u_phaddr;
	config.fb.addr[2] = picture->v_phaddr;
	ret = LayerSetPara(fd, hlay, &config);
	return ret;
}

int layer_set_src(int fd, unsigned int hlay, SrcInfo *src) {
	disp_layer_info layer_info;
	memset(&layer_info, 0, sizeof(disp_layer_info));
	LayerGetPara(fd, hlay, &layer_info);
	layer_info.fb.size.width = src->w;
	layer_info.fb.size.height = src->h;
	layer_info.fb.src_win.x = src->crop_x;
	layer_info.fb.src_win.y = src->crop_y;
	layer_info.fb.src_win.width = src->crop_w;
	layer_info.fb.src_win.height = src->crop_h;
	DISP_DBG_LOG("src:%d,%d,(%d,%d,%dx%d)\n", src->w, src->h, src->crop_x,
	        src->crop_y, src->crop_w, src->crop_h);
	switch (src->format) {
	case VIDEO_PIXEL_FORMAT_YUV_PLANER_422:
		layer_info.fb.format = DISP_FORMAT_YUV422_P;
		break;
	case VIDEO_PIXEL_FORMAT_YUV_PLANER_444:
		layer_info.fb.format = DISP_FORMAT_YUV444_P;
		break;
	case VIDEO_PIXEL_FORMAT_YUV_PLANER_420:
		layer_info.fb.format = DISP_FORMAT_YUV420_P;
		break;
	case VIDEO_PIXEL_FORMAT_NV21:
		layer_info.fb.format = DISP_FORMAT_YUV420_SP_VUVU;
		break;
	case VIDEO_PIXEL_FORMAT_NV12:
		layer_info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
		break;
	case VIDEO_PIXEL_FORMAT_YV12:
		layer_info.fb.format = DISP_FORMAT_YUV420_P;
		break;
	case VIDEO_PIXEL_FORMAT_YUV_MB32_420:
		layer_info.fb.format = DISP_FORMAT_YUV420_SP_TILE_UVUV;
		break;
	case VIDEO_PIXEL_FORMAT_YUV_MB32_422:
		layer_info.fb.format = DISP_FORMAT_YUV422_SP_TILE_VUVU;
		break;
	case VIDEO_PIXEL_FORMAT_YUYV:
		layer_info.fb.format = DISP_FORMAT_YUV422_I_YUYV;
		break;
	case VIDEO_PIXEL_FORMAT_ARGB:
		layer_info.fb.format = DISP_FORMAT_ARGB_8888;
		break;
	case VIDEO_PIXEL_FORMAT_RGBA:
		layer_info.fb.format = DISP_FORMAT_RGBA_8888;
		break;
	case VIDEO_PIXEL_FORMAT_ABGR:
		layer_info.fb.format = DISP_FORMAT_ABGR_8888;
		break;
	case VIDEO_PIXEL_FORMAT_BGRA:
		layer_info.fb.format = DISP_FORMAT_BGRA_8888;
		break;
	default:
		layer_info.fb.format = DISP_FORMAT_ARGB_8888;
		break;
	}
	return LayerSetPara(fd, hlay, &layer_info);
}

int layer_set_rect(int fd, unsigned int hlay, VoutRect *view) {
	disp_layer_info layer_info;
	memset(&layer_info, 0, sizeof(disp_layer_info));
	LayerGetPara(fd, hlay, &layer_info);
	layer_info.screen_win.x = view->x;
	layer_info.screen_win.y = view->y;
	layer_info.screen_win.width = view->width;
	layer_info.screen_win.height = view->height;
	DISP_DBG_LOG("screen rect: (%d,%d, %dx%d)\n", view->x, view->y, view->width,
	        view->height);
	return LayerSetPara(fd, hlay, &layer_info);
}

int layer_set_src_rect(int fd, unsigned int hlay, VoutRect *view) {
	disp_layer_info layer_info;
	memset(&layer_info, 0, sizeof(disp_layer_info));
	LayerGetPara(fd, hlay, &layer_info);
	layer_info.fb.src_win.x = view->x;
	layer_info.fb.src_win.y = view->y;
	layer_info.fb.src_win.width = view->width;
	layer_info.fb.src_win.height = view->height;
	return LayerSetPara(fd, hlay, &layer_info);
}

static int layer_set_zorder(int fd, unsigned int hlay, int zorder) {
	disp_layer_info config;
	videoZorder layer_zorder = (videoZorder) zorder;
	if ((layer_zorder < VIDEO_ZORDER_TOP)
	        || (layer_zorder < VIDEO_ZORDER_TOP)) {
		DISP_DBG_LOG("(%s)invalid zorder\n", __FUNCTION__);
		return RET_FAIL;
	}
	memset(&config, 0, sizeof(disp_layer_info));
	LayerGetPara(fd, hlay, &config);
	switch (layer_zorder) {
	case VIDEO_ZORDER_TOP:
		config.zorder = ZORDER_MAX;
		break;
	case VIDEO_ZORDER_BOTTOM:
		config.zorder = ZORDER_MIN;
		break;
	default:
		break;
	}
	return LayerSetPara(fd, hlay, &config);
}

int layer_set_colorkey(int fd, unsigned int hlay, unsigned int color) {
	unsigned long args[4] = { 0 };
	int ret = RET_OK;
	disp_color_info ck;
	unsigned int r;
	unsigned int g;
	unsigned int b;
	r = (color >> 16) & 0xff;
	g = (color >> 8) & 0xff;
	b = (color >> 0) & 0xff;
	ck.alpha = 0xff;
	ck.red = r;
	ck.green = g;
	ck.blue = b;
	args[0] = SCREEN_0;
	args[1] = (unsigned long) &ck;
	ioctl(fd, DISP_CMD_SET_COLORKEY, (void*) args);
	if (RET_OK != ret) {
		DISP_DBG_LOG("fail to set colorkey\n");
		ret = RET_FAIL;
	}
	return ret;
}

int layer_set_normal(int fd, unsigned int hlay) {
	disp_layer_info layer_info;
	LayerGetPara(fd, hlay, &layer_info);
	layer_info.fb.src_win.x = 0;
	layer_info.fb.src_win.y = 0;
	layer_info.fb.src_win.width = 1;
	layer_info.fb.src_win.height = 1;
	layer_info.fb.size.width = 1;
	layer_info.fb.size.height = 1;
	layer_info.screen_win.x = 0;
	layer_info.screen_win.y = 0;
	layer_info.screen_win.width = 1;
	layer_info.screen_win.height = 1;
	layer_info.mode = DISP_LAYER_WORK_MODE_NORMAL;
	LayerSetPara(fd, hlay, &layer_info);
	return 0;
}
#endif

int disp_layer_on(int dispFd, unsigned int dispLayer) {
	int ret = 0;
	unsigned long ioctlParam[4];

	DISP_CLAR(ioctlParam);

	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = HD2CHN(dispLayer);

#ifdef __SUNXI_DISPLAY2__
	ret = ioctl(dispFd, DISP_LAYER_ENABLE, (void*) ioctlParam);
#else
	ret = ioctl(dispFd, DISP_CMD_LAYER_ENABLE, (void*) ioctlParam);
#endif

	if (ret < 0) {
		DISP_DBG_LOG("%s:ioctl disp disp on failed!\r\n", __func__);
		ret = -1;
		goto errHdl;
	}

	errHdl: return ret;
}

int disp_layer_off(int dispFd, unsigned int dispLayer) {
	int ret = 0;
	unsigned int ioctlParam[4];

	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = HD2CHN(dispLayer);

#ifdef __SUNXI_DISPLAY2__
	ret = ioctl(dispFd, DISP_LAYER_DISABLE, (void*) ioctlParam);
#else
	ret = ioctl(dispFd, DISP_CMD_LAYER_DISABLE, (void*) ioctlParam);
#endif
	if (ret < 0) {
		DISP_DBG_LOG("%s:ioctl disp disp off failed!\r\n", __func__);
		ret = -1;
		goto errHdl;
	}

	errHdl: return ret;
}

static int Yuv2YuvRotate(void *src, void *dest, SrcInfo *srcinfo,
        rotateDegree degree) {
	unsigned int width, height;
	rotateDegree rot_angle;
	VideoPixelFormat fmt;

	if ((!src) || (!dest) || (!srcinfo))
		return -1;

	rot_angle = degree;
	fmt = srcinfo->format;
	width = srcinfo->w;
	height = srcinfo->h;

	switch (rot_angle) {
	case ROTATION_ANGLE_90:
		nv_rotage90(width, height, (unsigned char*) src, (unsigned char*) dest);
		break;
	case ROTATION_ANGLE_270:

		nv_rotage270(width, height, (unsigned char*) src,
		        (unsigned char*) dest);
		break;
	case ROTATION_ANGLE_180:
	default:
		break;
	}
	return 0;
}

int DispAllocateVideoMem(void *hdl, videoParam *param) {
	unsigned int nBufWidth;
	unsigned int nBufHeight;
	float bufferSizeNum;
	char *pVirBuf[2];
	char *pPhyBuf[2];
	int channel;
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	nBufWidth = param->srcInfo.w;
	nBufHeight = param->srcInfo.h;

	if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
		bufferSizeNum = 2.0;
	} else if ((param->srcInfo.format == VIDEO_PIXEL_FORMAT_ARGB)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_ABGR)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_RGBA)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_BGRA)) {
		bufferSizeNum = 4.0;
	} else {
		bufferSizeNum = 3.0 / 2;
	}

	/* Native Window will malloc double buffer */
	channel = HD2CHN(vout_hd->hlayer);
#ifdef CONF_KERNEL_IOMMU
	pVirBuf[0] = (char*) SunxiMemPalloc(vout_hd->pMemops,
	        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
	pPhyBuf[0] = (char*) SunxiMemGetPhysicAddressCpu(vout_hd->pMemops, pVirBuf[0]);

	if ((NULL == pVirBuf[0]) || (NULL == pPhyBuf[0])) {
		DISP_DBG_LOG("(%s %d) ION Malloc failed\n", __FUNCTION__, __LINE__);
		return RET_FAIL;
	}

	pVirBuf[1] = (char*) SunxiMemPalloc(vout_hd->pMemops,
	        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
	pPhyBuf[1] = (char*) SunxiMemGetPhysicAddressCpu(vout_hd->pMemops, pVirBuf[1]);

	if ((NULL == pVirBuf[1]) || (NULL == pPhyBuf[1])) {
		DISP_DBG_LOG("(%s %d) ION Malloc failed\n", __FUNCTION__, __LINE__);
		SunxiMemPfree(vout_hd->pMemops, (void*) pVirBuf[0]);
		return RET_FAIL;
	}

	vout_hd->renderbuf[channel][0].fd = SunxiMemGetBufferFd(vout_hd->pMemops,
	        (void*) pVirBuf[0]);
	vout_hd->renderbuf[channel][1].fd = SunxiMemGetBufferFd(vout_hd->pMemops,
	        (void*) pVirBuf[1]);
#else
	pVirBuf[0] = (char*) SunxiMemPalloc(vout_hd->pMemops,
	        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum * 2));
	pPhyBuf[0] = (char*) SunxiMemGetPhysicAddressCpu(vout_hd->pMemops,
	        pVirBuf[0]);

	if ((NULL == pVirBuf[0]) || (NULL == pPhyBuf[0])) {
		DISP_DBG_LOG("(%s %d) ION Malloc failed\n", __FUNCTION__, __LINE__);
		return RET_FAIL;
	}

	pVirBuf[1] = (unsigned long) (pPhyBuf[0]
	        + (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
	pPhyBuf[1] = (unsigned long) (pVirBuf[0]
	        + (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));

	vout_hd->renderbuf[channel][0].fd = 0;
	vout_hd->renderbuf[channel][1].fd = 0;
#endif

	vout_hd->renderbuf[channel][0].phy_addr = (unsigned long) pPhyBuf[0];
	vout_hd->renderbuf[channel][0].vir_addr = (unsigned long) pVirBuf[0];
	vout_hd->renderbuf[channel][0].y_phaddr = (unsigned long) pPhyBuf[0];
	vout_hd->renderbuf[channel][0].u_phaddr = (unsigned long) (pPhyBuf[0]
	        + nBufWidth * nBufHeight);
	vout_hd->renderbuf[channel][0].v_phaddr =
	        vout_hd->renderbuf[channel][0].u_phaddr
	                + (nBufWidth * nBufHeight) / 4;

	vout_hd->renderbuf[channel][1].phy_addr = (unsigned long) pPhyBuf[1];
	vout_hd->renderbuf[channel][1].vir_addr = (unsigned long) pVirBuf[1];
	vout_hd->renderbuf[channel][1].y_phaddr = (unsigned long) pPhyBuf[1];
	vout_hd->renderbuf[channel][1].u_phaddr = (unsigned long) (pPhyBuf[1]
	        + nBufWidth * nBufHeight);
	vout_hd->renderbuf[channel][1].v_phaddr =
	        vout_hd->renderbuf[channel][1].u_phaddr
	                + (nBufWidth * nBufHeight) / 4;

	vout_hd->interBufSet[channel] = 1;
	return RET_OK;
}

int DispFreeVideoMem(void *hdl) {
	int channel;

	dispOutPort *vout_hd = (dispOutPort*) hdl;
	channel = HD2CHN(vout_hd->hlayer);
	SunxiMemPfree(vout_hd->pMemops,
	        (void*) vout_hd->renderbuf[channel][0].vir_addr);
#ifdef CONF_KERNEL_IOMMU
	SunxiMemPfree(vout_hd->pMemops,
	        (void*) vout_hd->renderbuf[channel][1].vir_addr);
#endif
	vout_hd->interBufSet[channel] = 0;

	return RET_OK;
}

int DispWriteData(void *hdl, void *data, int size, videoParam *param) {
	unsigned int nBufWidth;
	unsigned int nBufHeight;
	float bufferSizeNum;
	int channel;
	int index = 0;
	char *pVirBuf;
	char *pPhyBuf;
	renderBuf renderbuf;
#ifdef DISP_DEBUG
	long long start_tm, end_tm;
#endif
	SrcInfo src_info;
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	if (!data)
		return -1;

#ifdef DISP_DEBUG
	start_tm = GetNowUs();
#endif
	channel = HD2CHN(vout_hd->hlayer);
	nBufWidth = param->srcInfo.w;
	nBufHeight = param->srcInfo.h;
	if ((nBufWidth <= 0) || (nBufHeight <= 0) || (param->srcInfo.crop_w <= 0)
	        || (param->srcInfo.crop_h <= 0)) {
		DISP_DBG_LOG("(%s %d) Bad parameter\n", __FUNCTION__, __LINE__);
		return RET_FAIL;
	}
	if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
		bufferSizeNum = 2.0;
	} else if ((param->srcInfo.format == VIDEO_PIXEL_FORMAT_ARGB)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_ABGR)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_RGBA)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_BGRA)) {
		bufferSizeNum = 4.0;
	} else {
		bufferSizeNum = 3.0 / 2;
	}

	if ((unsigned int) (nBufWidth * nBufHeight * bufferSizeNum) > size)
		return -1;

	if (vout_hd->interBufSet[channel]) {
		index = (vout_hd->bufindex[channel] == 1) ? 1 : 0;
		pVirBuf = (char*) vout_hd->renderbuf[channel][index].vir_addr;
		pPhyBuf = (char*) vout_hd->renderbuf[channel][index].phy_addr;
#ifdef CONF_KERNEL_IOMMU
		renderbuf.fd = vout_hd->renderbuf[channel][index].fd;
#endif
		vout_hd->bufindex[channel] = (vout_hd->bufindex[channel] == 1) ? 0 : 1;
	} else {
		DISP_DBG_LOG("(%s %d) No Video memory\n", __FUNCTION__, __LINE__);

		return RET_FAIL;
	}
	memset(&src_info, 0, sizeof(SrcInfo));
	switch (vout_hd->route) {
	case VIDEO_SRC_FROM_ISP:
	case VIDEO_SRC_FROM_CAM:
	case VIDEO_SRC_FROM_FILE:
		if ((vout_hd->rotate == ROTATION_ANGLE_90)
		        || (vout_hd->rotate == ROTATION_ANGLE_270)) {
			if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
				//TODO YUYV rotation does not support
				printf("YUYV rotation does not support\n");
			} else {
				Yuv2YuvRotate(data, pVirBuf, &param->srcInfo, vout_hd->rotate);
			}
			src_info.w = param->srcInfo.h;
			src_info.h = param->srcInfo.w;
			src_info.crop_x = param->srcInfo.crop_x;
			src_info.crop_y = param->srcInfo.crop_y;
			src_info.crop_w = param->srcInfo.crop_h;
			src_info.crop_h = param->srcInfo.crop_w;
			src_info.format = param->srcInfo.format;
			src_info.color_space = param->srcInfo.color_space;
		} else {
			src_info.w = param->srcInfo.w;
			src_info.h = param->srcInfo.h;
			src_info.crop_x = param->srcInfo.crop_x;
			src_info.crop_y = param->srcInfo.crop_y;
			src_info.crop_w = param->srcInfo.crop_w;
			src_info.crop_h = param->srcInfo.crop_h;
			src_info.format = param->srcInfo.format;
			src_info.color_space = param->srcInfo.color_space;
			memcpy(pVirBuf, data,
			        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
		}
		break;
	case VIDEO_SRC_FROM_VE:
		if ((vout_hd->rotate == ROTATION_ANGLE_90)
		        || (vout_hd->rotate == ROTATION_ANGLE_270)) {
			if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
				//TODO YUYV rotation does not support
				printf("YUYV rotation does not support\n");
			} else {
				Yuv2YuvRotate(data, pVirBuf, &param->srcInfo, vout_hd->rotate);
			}
			src_info.w = param->srcInfo.h;
			src_info.h = param->srcInfo.w;
			src_info.crop_x = param->srcInfo.crop_x;
			src_info.crop_y = param->srcInfo.crop_y;
			src_info.crop_w = param->srcInfo.crop_h;
			src_info.crop_h = param->srcInfo.crop_w;
			src_info.format = param->srcInfo.format;
			src_info.color_space = param->srcInfo.color_space;
		} else {
			src_info.w = param->srcInfo.w;
			src_info.h = param->srcInfo.h;
			src_info.crop_x = param->srcInfo.crop_x;
			src_info.crop_y = param->srcInfo.crop_y;
			src_info.crop_w = param->srcInfo.crop_w;
			src_info.crop_h = param->srcInfo.crop_h;
			src_info.format = param->srcInfo.format;
			src_info.color_space = param->srcInfo.color_space;
			memcpy(pVirBuf, data,
			        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
		}
		break;
	default:
		DISP_DBG_LOG("(%s %d) Not support disp src\n", __FUNCTION__, __LINE__);
		break;
	}
	SunxiMemFlushCache(vout_hd->pMemops, pVirBuf,
	        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
	renderbuf.y_phaddr = (unsigned long) pPhyBuf;
	if (src_info.format == VIDEO_PIXEL_FORMAT_YV12) {
		renderbuf.v_phaddr = (unsigned long) (pPhyBuf + nBufWidth * nBufHeight);
		renderbuf.u_phaddr = renderbuf.v_phaddr + (nBufWidth * nBufHeight) / 4;
	} else {
		renderbuf.u_phaddr = (unsigned long) (pPhyBuf + nBufWidth * nBufHeight);
		renderbuf.v_phaddr = renderbuf.u_phaddr + (nBufWidth * nBufHeight) / 4;
	}
#ifdef SUNXI_DISPLAY_GPU
	gpuRenderInfo.srcInfo.cropx = src_info.crop_x;
	gpuRenderInfo.srcInfo.cropy = src_info.crop_y;
	gpuRenderInfo.srcInfo.cropwidth = src_info.crop_w;
	gpuRenderInfo.srcInfo.cropheight = src_info.crop_h;
	gpuRenderInfo.srcInfo.srcwidth = src_info.w;
	gpuRenderInfo.srcInfo.srcheight = src_info.h;
	if (src_info.format == VIDEO_PIXEL_FORMAT_NV21)
		gpuRenderInfo.srcInfo.srcformat = FORMAT_NV21;
	else
		gpuRenderInfo.srcInfo.srcformat = FORMAT_YV12;
	gpuRenderInfo.renderBuf = renderbuf;
#ifdef SUNXI_DISPLAY_GPU_BUF
	gpuRenderInfo.argbSrcInfo.color_space = src_info.color_space;
#endif
	pthread_cond_signal(&gpuRenderInfo.cond);
#else
#ifdef __SUNXI_DISPLAY2__
	layer_set_src(vout_hd->disp_fd, vout_hd->hlayer, &src_info, &renderbuf);
#else
	layer_set_src(vout_hd->disp_fd, vout_hd->hlayer, &src_info);
	LayerRender(vout_hd->disp_fd, vout_hd->hlayer, &renderbuf);
#endif
#endif
#ifdef DISP_DEBUG
	end_tm = GetNowUs();
	DISP_DBG_LOG("%s cost %lld us, pts:%lld\n",
			__FUNCTION__, end_tm - start_tm, start_tm);
#endif
	return RET_OK;
}

int DispDequeue(void *hdl, renderBuf *rBuf) {
	int channel;
	int index = 0;
	char *pVirBuf;
	char *pPhyBuf;

	dispOutPort *vout_hd = (dispOutPort*) hdl;
	channel = HD2CHN(vout_hd->hlayer);

	if (vout_hd->interBufSet[channel]) {
		index = (vout_hd->bufindex[channel] == 1) ? 1 : 0;
		pVirBuf = (char*) vout_hd->renderbuf[channel][index].vir_addr;
		pPhyBuf = (char*) vout_hd->renderbuf[channel][index].phy_addr;
#ifdef CONF_KERNEL_IOMMU
		rBuf->fd = vout_hd->renderbuf[channel][index].fd;
#endif
		vout_hd->bufindex[channel] = (vout_hd->bufindex[channel] == 1) ? 0 : 1;
	} else {
		DISP_DBG_LOG("(%s %d) No Video memory\n", __FUNCTION__, __LINE__);
		return RET_FAIL;
	}

	rBuf->isExtPhy = VIDEO_USE_INTERN_ALLOC_BUF;
	rBuf->phy_addr = (unsigned long) pPhyBuf;
	rBuf->vir_addr = (unsigned long) pVirBuf;

	return RET_OK;
}

int DispQueueToDisplay(void *hdl, int size, videoParam *param, renderBuf *rBuf) {
#ifdef __SUNXI_DISPLAY2__
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned int nBufWidth;
	unsigned int nBufHeight;
	float bufferSizeNum;
	renderBuf renderbuf;
	SrcInfo src_info;

	nBufWidth = param->srcInfo.w;
	nBufHeight = param->srcInfo.h;

	if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
		bufferSizeNum = 2.0;
	} else if ((param->srcInfo.format == VIDEO_PIXEL_FORMAT_ARGB)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_ABGR)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_RGBA)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_BGRA)) {
		bufferSizeNum = 4.0;
	} else {
		bufferSizeNum = 3.0 / 2;
	}

	if ((unsigned int) (nBufWidth * nBufHeight * bufferSizeNum) > size)
		return -1;

	memset(&src_info, 0, sizeof(SrcInfo));
	switch (vout_hd->route) {
	case VIDEO_SRC_FROM_ISP:
	case VIDEO_SRC_FROM_CAM:
	case VIDEO_SRC_FROM_FILE:
	case VIDEO_SRC_FROM_VE:
	default:
		src_info.w = param->srcInfo.w;
		src_info.h = param->srcInfo.h;
		src_info.crop_x = param->srcInfo.crop_x;
		src_info.crop_y = param->srcInfo.crop_y;
		src_info.crop_w = param->srcInfo.crop_w;
		src_info.crop_h = param->srcInfo.crop_h;
		src_info.format = param->srcInfo.format;
		src_info.color_space = param->srcInfo.color_space;
#if defined(CONF_KERNEL_VERSION_5_4)
		src_info.lbc_mod = param->srcInfo.lbc_mod;
		src_info.is_lossy = param->srcInfo.is_lossy;
		src_info.rc_en = param->srcInfo.rc_en;
#endif
		DISP_DBG_LOG("(%s %d)default src mode\n", __FUNCTION__, __LINE__);
		break;
	}

	if (rBuf->isExtPhy == VIDEO_USE_EXTERN_ION_BUF) {
		renderbuf.y_phaddr = (unsigned long) rBuf->y_phaddr;
		if (src_info.format == VIDEO_PIXEL_FORMAT_YV12) {
			renderbuf.u_phaddr = (unsigned long) (rBuf->v_phaddr);
			renderbuf.v_phaddr = (unsigned long) (rBuf->u_phaddr);
		} else {
			renderbuf.u_phaddr = (unsigned long) (rBuf->u_phaddr);
			renderbuf.v_phaddr = (unsigned long) (rBuf->v_phaddr);
		}
	} else {
		SunxiMemFlushCache(vout_hd->pMemops, (void*) rBuf->vir_addr,
		        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
		renderbuf.y_phaddr = (unsigned long) rBuf->phy_addr;
		if (src_info.format == VIDEO_PIXEL_FORMAT_YV12) {
			renderbuf.v_phaddr = (unsigned long) (rBuf->phy_addr
			        + nBufWidth * nBufHeight);
			renderbuf.u_phaddr = renderbuf.v_phaddr
			        + (nBufWidth * nBufHeight) / 4;
		} else {
			renderbuf.u_phaddr = (unsigned long) (rBuf->phy_addr
			        + nBufWidth * nBufHeight);
			renderbuf.v_phaddr = renderbuf.u_phaddr
			        + (nBufWidth * nBufHeight) / 4;
		}
	}

#ifdef CONF_KERNEL_IOMMU
	renderbuf.fd = rBuf->fd;
#endif

	layer_set_src(vout_hd->disp_fd, vout_hd->hlayer, &src_info, &renderbuf);
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned int nBufWidth;
	unsigned int nBufHeight;
	float bufferSizeNum;
	renderBuf renderbuf;
	SrcInfo src_info;
	nBufWidth = param->srcInfo.w;
	nBufHeight = param->srcInfo.h;

	if (param->srcInfo.format == VIDEO_PIXEL_FORMAT_YUYV) {
		bufferSizeNum = 2.0;
	} else if ((param->srcInfo.format == VIDEO_PIXEL_FORMAT_ARGB)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_ABGR)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_RGBA)
	        || (param->srcInfo.format == VIDEO_PIXEL_FORMAT_BGRA)) {
		bufferSizeNum = 4.0;
	} else {
		bufferSizeNum = 3.0 / 2;
	}

	if ((unsigned int) (nBufWidth * nBufHeight * bufferSizeNum) > size)
		return -1;
	memset(&src_info, 0, sizeof(SrcInfo));
	switch (vout_hd->route) {
	case VIDEO_SRC_FROM_ISP:
	case VIDEO_SRC_FROM_CAM:
	case VIDEO_SRC_FROM_FILE:
	case VIDEO_SRC_FROM_VE:
	default:
		src_info.w = param->srcInfo.w;
		src_info.h = param->srcInfo.h;
		src_info.crop_x = param->srcInfo.crop_x;
		src_info.crop_y = param->srcInfo.crop_y;
		src_info.crop_w = param->srcInfo.crop_w;
		src_info.crop_h = param->srcInfo.crop_h;
		src_info.format = param->srcInfo.format;
		src_info.color_space = param->srcInfo.color_space;
		break;
	}
	if (rBuf->isExtPhy == VIDEO_USE_EXTERN_ION_BUF) {
		renderbuf.y_phaddr = (unsigned long) rBuf->y_phaddr;
		if (src_info.format == VIDEO_PIXEL_FORMAT_YV12) {
			renderbuf.u_phaddr = (unsigned long) (rBuf->v_phaddr);
			renderbuf.v_phaddr = (unsigned long) (rBuf->u_phaddr);
		} else {
			renderbuf.u_phaddr = (unsigned long) (rBuf->u_phaddr);
			renderbuf.v_phaddr = (unsigned long) (rBuf->v_phaddr);
		}
	} else {
		SunxiMemFlushCache(vout_hd->pMemops, (void*) rBuf->vir_addr,
		        (unsigned int) (nBufWidth * nBufHeight * bufferSizeNum));
		renderbuf.y_phaddr = (unsigned long) rBuf->phy_addr;
		if (src_info.format == VIDEO_PIXEL_FORMAT_YV12) {
			renderbuf.v_phaddr = (unsigned long) (rBuf->phy_addr
			        + nBufWidth * nBufHeight);
			renderbuf.u_phaddr = renderbuf.v_phaddr
			        + (nBufWidth * nBufHeight) / 4;
		} else {
			renderbuf.u_phaddr = (unsigned long) (rBuf->phy_addr
			        + nBufWidth * nBufHeight);
			renderbuf.v_phaddr = renderbuf.u_phaddr
			        + (nBufWidth * nBufHeight) / 4;
		}
	}
#ifdef SUNXI_DISPLAY_GPU
	gpuRenderInfo.srcInfo.cropx = src_info.crop_x;
	gpuRenderInfo.srcInfo.cropy = src_info.crop_y;
	gpuRenderInfo.srcInfo.cropwidth = src_info.crop_w;
	gpuRenderInfo.srcInfo.cropheight = src_info.crop_h;
	gpuRenderInfo.srcInfo.srcwidth = src_info.w;
	gpuRenderInfo.srcInfo.srcheight = src_info.h;
	if (src_info.format == VIDEO_PIXEL_FORMAT_NV21)
		gpuRenderInfo.srcInfo.srcformat = FORMAT_NV21;
	else
		gpuRenderInfo.srcInfo.srcformat = FORMAT_YV12;
	gpuRenderInfo.renderBuf = renderbuf;
#ifdef SUNXI_DISPLAY_GPU_BUF
	gpuRenderInfo.argbSrcInfo.color_space = src_info.color_space;
#endif
	pthread_cond_signal(&gpuRenderInfo.cond);
#else
	layer_set_src(vout_hd->disp_fd, vout_hd->hlayer, &src_info);
	LayerRender(vout_hd->disp_fd, vout_hd->hlayer, &renderbuf);
#endif
#endif
	return 0;
}

int DispSetEnable(void *hdl, int enable) {
#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	int ret;
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	config.channel = HD2CHN(vout_hd->hlayer);
	config.layer_id = HD2LYL(vout_hd->hlayer);
	LayerGetPara(vout_hd->disp_fd, &config);

	config.enable = enable;

	config.info.screen_win.x = vout_hd->rect.x;
	config.info.screen_win.y = vout_hd->rect.y;
	config.info.screen_win.width = vout_hd->rect.width;
	config.info.screen_win.height = vout_hd->rect.height;

	switch (vout_hd->layer_zorder) {
	case VIDEO_ZORDER_TOP:
		config.info.zorder = ZORDER_MAX;
		break;
	case VIDEO_ZORDER_MIDDLE:
		config.info.zorder = ZORDER_MID;
		break;
	case VIDEO_ZORDER_BOTTOM:
		config.info.zorder = ZORDER_MIN;
		break;
	default:
		break;
	}

	//vout_hd->enable = enable;
	ret = LayerSetPara(vout_hd->disp_fd, &config);
#if defined(CONF_KERNEL_VERSION_4_9) || defined(CONF_KERNEL_VERSION_5_4)
	/* Immediately releasing the buffer may display anomalies
	* because the display layer has not been closed, 1000(ms)/60(fps)=17 */
	if(!enable)
		usleep(17 * 1000);
#endif
	return ret;
#else
#ifdef SUNXI_DISPLAY_GPU
#ifdef SUNXI_DISPLAY_GPU_BUF
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	if (enable)
		return disp_layer_on(vout_hd->disp_fd, vout_hd->hlayer);
	else
		return disp_layer_off(vout_hd->disp_fd, vout_hd->hlayer);
#endif
	/* printf("SUNXI_DISPLAY_GPU DispSetEnable is not support\n"); */
	return RET_OK;
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	if (enable)
		return disp_layer_on(vout_hd->disp_fd, vout_hd->hlayer);
	else
		return disp_layer_off(vout_hd->disp_fd, vout_hd->hlayer);
#endif
#endif
}

int DispSetSrcRect(void *hdl, VoutRect *src_rect) {
#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	config.channel = HD2CHN(vout_hd->hlayer);
	config.layer_id = HD2LYL(vout_hd->hlayer);
	LayerGetPara(vout_hd->disp_fd, &config);

	config.info.fb.crop.x = src_rect->x;
	config.info.fb.crop.y = src_rect->y;
	config.info.fb.crop.width = (src_rect->width);
	config.info.fb.crop.height = (src_rect->height);

	DISP_DBG_LOG("width: 0x%llx, height: 0x%llx", config.info.fb.crop.width,
	        config.info.fb.crop.height);

	config.info.fb.crop.x = config.info.fb.crop.x << 32;
	config.info.fb.crop.y = config.info.fb.crop.y << 32;
	config.info.fb.crop.width = config.info.fb.crop.width << 32;
	config.info.fb.crop.height = config.info.fb.crop.height << 32;

	return LayerSetPara(vout_hd->disp_fd, &config);
#else
#ifdef SUNXI_DISPLAY_GPU
	/* DISP_DBG_LOG("SUNXI_DISPLAY_GPU DispSetSrcRect is not support\n"); */
	return RET_OK;
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	return layer_set_src_rect(vout_hd->disp_fd, vout_hd->hlayer, src_rect);
#endif
#endif
}

int DispSetRect(void *hdl, VoutRect *rect) {
#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	config.channel = HD2CHN(vout_hd->hlayer);
	config.layer_id = HD2LYL(vout_hd->hlayer);
	LayerGetPara(vout_hd->disp_fd, &config);

	vout_hd->rect.x = rect->x;
	vout_hd->rect.y = rect->y;
	vout_hd->rect.width = rect->width;
	vout_hd->rect.height = rect->height;

	config.info.screen_win.x = rect->x;
	config.info.screen_win.y = rect->y;
	config.info.screen_win.width = rect->width;
	config.info.screen_win.height = rect->height;
	return LayerSetPara(vout_hd->disp_fd, &config);
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
#ifdef SUNXI_DISPLAY_GPU
	gpuRenderInfo.outInfo.dstx = rect->x;
	gpuRenderInfo.outInfo.dsty = rect->y;
	gpuRenderInfo.outInfo.dstwidth = rect->width;
	gpuRenderInfo.outInfo.dstheight = rect->height;
#ifdef SUNXI_DISPLAY_GPU_BUF
	int i = 0;
	pthread_mutex_lock(&gpuRenderInfo.initMtx);
	pthread_mutex_lock(&gpuRenderInfo.mtx);
	gpuRenderInfo.outInfo.dstx = 0;
	gpuRenderInfo.outInfo.dsty = 0;
	gpuRenderInfo.argbSrcInfo.crop_w = rect->width;
	gpuRenderInfo.argbSrcInfo.crop_h = rect->height;
	gpuRenderInfo.argbSrcInfo.w = rect->width;
	gpuRenderInfo.argbSrcInfo.h = rect->height;
	for (i = 0; i < 2; i++) {
		if (gpuRenderInfo.outArgbBuf[i]) {
			SunxiMemPfree(gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);
			gpuRenderInfo.outArgbBuf[i] = NULL;
			DestoryDstKHR(gpuRenderInfo.outArgbBufImg[i]);
		}

		if (!gpuRenderInfo.outArgbBuf[i]) {
			gpuRenderInfo.outArgbBuf[i] = (char*) SunxiMemPalloc(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outInfo.dstwidth
			                * gpuRenderInfo.outInfo.dstheight * 4);
			gpuRenderInfo.outArgbBufFd[i] = SunxiMemGetBufferFd(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);

			gpuRenderInfo.outArgbBufImg[i] = gl_createDSTKHR(
			        gpuRenderInfo.outArgbBufFd[i], &gpuRenderInfo.outInfo);
		}
	}
	pthread_mutex_unlock(&gpuRenderInfo.initMtx);
	pthread_mutex_unlock(&gpuRenderInfo.mtx);
	return layer_set_rect(vout_hd->disp_fd, vout_hd->hlayer, rect);
#endif
	return RET_OK;
#else
	return layer_set_rect(vout_hd->disp_fd, vout_hd->hlayer, rect);
#endif
#endif
}

int DispSetRectFake(void *hdl, VoutRect *rect) {
#ifdef __SUNXI_DISPLAY2__
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	vout_hd->rect.x = rect->x;
	vout_hd->rect.y = rect->y;
	vout_hd->rect.width = rect->width;
	vout_hd->rect.height = rect->height;
	/* Do not actually set it, set it when you call enable */
	/* Avoid video stretching problems in the same chn and different layers */
	/* return LayerSetPara(vout_hd->disp_fd, &config); */
	return RET_OK;
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
#ifdef SUNXI_DISPLAY_GPU
	gpuRenderInfo.outInfo.dstx = rect->x;
	gpuRenderInfo.outInfo.dsty = rect->y;
	gpuRenderInfo.outInfo.dstwidth = rect->width;
	gpuRenderInfo.outInfo.dstheight = rect->height;
#ifdef SUNXI_DISPLAY_GPU_BUF
	int i = 0;
	pthread_mutex_lock(&gpuRenderInfo.initMtx);
	pthread_mutex_lock(&gpuRenderInfo.mtx);
	gpuRenderInfo.outInfo.dstx = 0;
	gpuRenderInfo.outInfo.dsty = 0;
	gpuRenderInfo.argbSrcInfo.crop_w = rect->width;
	gpuRenderInfo.argbSrcInfo.crop_h = rect->height;
	gpuRenderInfo.argbSrcInfo.w = rect->width;
	gpuRenderInfo.argbSrcInfo.h = rect->height;
	for (i = 0; i < 2; i++) {
		if (gpuRenderInfo.outArgbBuf[i]) {
			SunxiMemPfree(gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);
			gpuRenderInfo.outArgbBuf[i] = NULL;
			DestoryDstKHR(gpuRenderInfo.outArgbBufImg[i]);
		}

		if (!gpuRenderInfo.outArgbBuf[i]) {
			gpuRenderInfo.outArgbBuf[i] = (char*) SunxiMemPalloc(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outInfo.dstwidth
			                * gpuRenderInfo.outInfo.dstheight * 4);
			gpuRenderInfo.outArgbBufFd[i] = SunxiMemGetBufferFd(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);

			gpuRenderInfo.outArgbBufImg[i] = gl_createDSTKHR(
			        gpuRenderInfo.outArgbBufFd[i], &gpuRenderInfo.outInfo);
		}
	}
	pthread_mutex_unlock(&gpuRenderInfo.initMtx);
	pthread_mutex_unlock(&gpuRenderInfo.mtx);
	return layer_set_rect(vout_hd->disp_fd, vout_hd->hlayer, rect);
#endif
	return RET_OK;
#else
	return layer_set_rect(vout_hd->disp_fd, vout_hd->hlayer, rect);
#endif
#endif
}

int DispSetRotateAngel(void *hdl, int degree) {
	int ret = RET_OK;
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	rotateDegree rot_deg = (rotateDegree) degree;

	/*unsigned long args[4]={0};
	 args[0] = SCREEN_0;
	 args[1] = degree;
	 ret = ioctl(vout_hd->disp_fd, DISP_ROTATION_SW_SET_ROT, args);
	 */
	vout_hd->rotate = rot_deg;

	return ret;
}

int DispSetRoute(void *hdl, int route) {
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	vout_hd->route = (videoSource) route;
	return RET_OK;
}
int DispSetZorder(void *hdl, int zorder) {
#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	videoZorder layer_zorder = (videoZorder) zorder;
	if ((layer_zorder < VIDEO_ZORDER_TOP)
	        || (layer_zorder < VIDEO_ZORDER_TOP)) {
		DISP_DBG_LOG("(%s)invalid zorder\n", __FUNCTION__);
		return RET_FAIL;
	}

	config.channel = HD2CHN(vout_hd->hlayer);
	config.layer_id = HD2LYL(vout_hd->hlayer);
	vout_hd->layer_zorder = layer_zorder;
	LayerGetPara(vout_hd->disp_fd, &config);
	switch (layer_zorder) {
	case VIDEO_ZORDER_TOP:
		config.info.zorder = ZORDER_MAX;
		break;
	case VIDEO_ZORDER_MIDDLE:
		config.info.zorder = ZORDER_MID;
		break;
	case VIDEO_ZORDER_BOTTOM:
		config.info.zorder = ZORDER_MIN;
		break;
	default:
		break;
	}
	return LayerSetPara(vout_hd->disp_fd, &config);
#else
#ifdef SUNXI_DISPLAY_GPU
#ifdef SUNXI_DISPLAY_GPU_BUF
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	return layer_set_zorder(vout_hd->disp_fd, vout_hd->hlayer, zorder);
#endif
	/* DISP_DBG_LOG("SUNXI_DISPLAY_GPU DispSetZorder is not support\n"); */
	return 0;
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	return layer_set_zorder(vout_hd->disp_fd, vout_hd->hlayer, zorder);
#endif
#endif
}
int DispGetScreenWidth(void *hdl) {
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned long ioctlParam[4];
	unsigned int width;

	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = 0;
#ifdef __SUNXI_DISPLAY2__
	width = ioctl(vout_hd->disp_fd, DISP_GET_SCN_WIDTH, ioctlParam);
#else
	width = ioctl(vout_hd->disp_fd, DISP_CMD_GET_SCN_WIDTH, ioctlParam);
#endif
	return width;
}

int DispGetScreenHeight(void *hdl) {
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned long ioctlParam[4];
	unsigned int height;
#ifdef __SUNXI_DISPLAY2__
	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = 0;
	height = ioctl(vout_hd->disp_fd, DISP_GET_SCN_HEIGHT, ioctlParam);
#else
	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = 0;
	height = ioctl(vout_hd->disp_fd, DISP_CMD_GET_SCN_HEIGHT, ioctlParam);
#endif
	return height;
}

int DispSetIoctl(void *hdl, unsigned int cmd, unsigned long para) {
	int ret = 0;
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned long ioctlParam[4];
	unsigned int disp_cmd;
	DISP_CLAR(ioctlParam);

	ioctlParam[0] = SCREEN_0;
#ifdef __SUNXI_DISPLAY2__
	switch (cmd) {
	case VIDEO_CMD_SET_BRIGHTNESS:
		disp_cmd = DISP_LCD_SET_BRIGHTNESS;
		break;
	case VIDEO_CMD_SET_CONTRAST:
	case VIDEO_CMD_SET_HUE:
	case VIDEO_CMD_SET_SATURATION:
		break;
	case VIDEO_CMD_SET_VEDIO_ENHANCE_DEFAULT:
		disp_cmd = DISP_ENHANCE_SET_MODE;
		break;
	}
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 *config = (disp_layer_config2*) para;
#else
	disp_layer_config *config = (disp_layer_config*) para;
#endif
	config->channel = HD2CHN(vout_hd->hlayer);
	config->layer_id = HD2LYL(vout_hd->hlayer);
	ioctlParam[1] = para;
	ioctlParam[2] = 1;
#else
	switch (cmd) {
	case VIDEO_CMD_SET_BRIGHTNESS:
		disp_cmd = DISP_CMD_LCD_SET_BRIGHTNESS;
		break;
	case VIDEO_CMD_SET_CONTRAST:
		disp_cmd = DISP_CMD_LAYER_SET_CONTRAST;
		break;
	case VIDEO_CMD_SET_HUE:
		disp_cmd = DISP_CMD_LAYER_SET_HUE;
		break;
	case VIDEO_CMD_SET_SATURATION:
		disp_cmd = DISP_CMD_LAYER_SET_SATURATION;
		break;
	case VIDEO_CMD_SET_VEDIO_ENHANCE_DEFAULT:
		disp_cmd = DISP_CMD_SET_ENHANCE_MODE;
		break;
	}
	ioctlParam[1] = HD2CHN(vout_hd->hlayer);
	ioctlParam[2] = (unsigned int) para;
#endif

#ifdef SUNXI_DISPLAY_GPU
#ifdef SUNXI_DISPLAY_GPU_BUF
	ret = ioctl(vout_hd->disp_fd, cmd, (void*) ioctlParam);
#else
	/* DISP_DBG_LOG("SUNXI_DISPLAY_GPU DispSetIoctl is not support\n"); */
	return RET_OK;
#endif
#else
	ret = ioctl(vout_hd->disp_fd, cmd, (void*) ioctlParam);
#endif
	if (ret < 0) {
		DISP_DBG_LOG("%s: fail to open backlight!\r\n", __func__);
		ret = -2;
		goto errHdl;
	}
	errHdl: return ret;
}

#ifdef SUNXI_DISPLAY_GPU
static void* GpuRenderThreadProc(void *arg) {
	int videoDataFd;

	pthread_mutex_lock(&gpuRenderInfo.initMtx);
#ifdef SUNXI_DISPLAY_GPU_BUF
	InitGLResource(GPU_IONBUF);
	int flag = 0;
	int i = 0;
	for (i = 0; i < 2; i++) {
		if (!gpuRenderInfo.outArgbBuf[i]) {
			gpuRenderInfo.outArgbBuf[i] = (char*) SunxiMemPalloc(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outInfo.dstwidth
			                * gpuRenderInfo.outInfo.dstheight * 4);
			gpuRenderInfo.outArgbBufFd[i] = SunxiMemGetBufferFd(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);

			gpuRenderInfo.outArgbBufImg[i] = gl_createDSTKHR(
			        gpuRenderInfo.outArgbBufFd[i], &gpuRenderInfo.outInfo);
		}
	}
#else
	InitGLResource(GPU_FRAMEBUFFER);
#endif
	gpuRenderInfo.gpuThreadRun = 1;
	pthread_mutex_unlock(&gpuRenderInfo.initMtx);

	while (1) {
		pthread_mutex_lock(&gpuRenderInfo.mtx);
		pthread_cond_wait(&gpuRenderInfo.cond, &gpuRenderInfo.mtx);

		if (gpuRenderInfo.gpuThreadRun) {

			videoDataFd = sunxi_ion_alloc_get_bufferFd(
			        sunxi_ion_alloc_phy2vir_cpu(
			                gpuRenderInfo.renderBuf.y_phaddr));

			gl_createSRCKHR(videoDataFd, &gpuRenderInfo.srcInfo);
#ifdef SUNXI_DISPLAY_GPU_BUF
			gl_bindFBO(gpuRenderInfo.outArgbBufImg[flag]);
#endif
			ShaderYUVtoRGB_toionbuf(&gpuRenderInfo.outInfo);
			DestorySrcKHR();

#ifdef SUNXI_DISPLAY_GPU_BUF
			gpuRenderInfo.renderBuf.y_phaddr = SunxiMemGetPhysicAddressCpu(
			        gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[flag]);
			gpuRenderInfo.renderBuf.v_phaddr = 0;
			gpuRenderInfo.renderBuf.u_phaddr = 0;
			flag = !flag;
			layer_set_src(gpuRenderInfo.vout_hd->disp_fd,
			        gpuRenderInfo.vout_hd->hlayer, &gpuRenderInfo.argbSrcInfo);
			LayerRender(gpuRenderInfo.vout_hd->disp_fd,
			        gpuRenderInfo.vout_hd->hlayer, &gpuRenderInfo.renderBuf);
#endif
			pthread_mutex_unlock(&gpuRenderInfo.mtx);
		} else {
			pthread_mutex_unlock(&gpuRenderInfo.mtx);
			break;
		}
	}

#ifdef SUNXI_DISPLAY_GPU_BUF
	for (i = 0; i < 2; i++) {
		if (gpuRenderInfo.outArgbBuf[i]) {
			SunxiMemPfree(gpuRenderInfo.vout_hd->pMemops,
			        gpuRenderInfo.outArgbBuf[i]);
			gpuRenderInfo.outArgbBuf[i] = NULL;
			DestoryDstKHR(gpuRenderInfo.outArgbBufImg[i]);
		}
	}
	DestoryGLResource(GPU_IONBUF);
#else
	DestoryGLResource(GPU_FRAMEBUFFER);
#endif
	pthread_exit(0);
	return (void*) 0;
}
#endif

int DispInit(void *hdl, int enable, int rotate, VoutRect *rect) {
	int ret = 0;
	int hlay;
	int ch, id;
	unsigned int width, height;
	unsigned long ioctlParam[4];
#ifdef __SUNXI_DISPLAY2__
#ifdef CONF_KERNEL_IOMMU
	disp_layer_config2 config;
	memset(&config, 0, sizeof(disp_layer_config2));
#else
	disp_layer_config config;
	memset(&config, 0, sizeof(disp_layer_config));
#endif
	dispOutPort *vout_hd = (dispOutPort*) hdl;

	vout_hd->disp_fd = open(DISP_DEV_NAME, O_RDWR);
	if (vout_hd->disp_fd < 0) {
		DISP_DBG_LOG("%s:open disp handle error!\r\n", __func__);
		goto errHdl;
	}
	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = 0;
	width = ioctl(vout_hd->disp_fd, DISP_GET_SCN_WIDTH, ioctlParam);
	height = ioctl(vout_hd->disp_fd, DISP_GET_SCN_HEIGHT, ioctlParam);

	hlay = LayerRequest(&ch, &id);
	config.channel = ch;
	config.layer_id = id;
	config.enable = enable;
	config.info.screen_win.x = 0;
	config.info.screen_win.y = 0;
	config.info.screen_win.width = width;
	config.info.screen_win.height = height;
	config.info.mode = LAYER_MODE_BUFFER;
	config.info.alpha_mode = 0;
	config.info.alpha_value = 0x80;
	config.info.fb.flags = DISP_BF_NORMAL;
	config.info.fb.scan = DISP_SCAN_PROGRESSIVE;
	config.info.fb.color_space = (rect->height < 720) ? DISP_BT601 : DISP_BT709;
	config.info.zorder = HLAY(ch, id);
	LayerSetPara(vout_hd->disp_fd, &config);

	DISP_DBG_LOG("hlay:%d, zorder=%d,", hlay, config.info.zorder);

	vout_hd->hlayer = hlay;
	vout_hd->enable = enable;
	vout_hd->rotate = (rotateDegree) rotate;
	vout_hd->rect.x = rect->x;
	vout_hd->rect.y = rect->y;
	vout_hd->rect.width = rect->width;
	vout_hd->rect.height = rect->height;
	vout_hd->pMemops = GetMemAdapterOpsS();
	SunxiMemOpen(vout_hd->pMemops);

	errHdl: return ret;
#else
	disp_layer_info config;
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	vout_hd->disp_fd = open(DISP_DEV_NAME, O_RDWR);
	if (vout_hd->disp_fd < 0) {
		DISP_DBG_LOG("%s:open disp handle error!\r\n", __func__);
		goto errHdl;
	}
	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = 0;
	width = ioctl(vout_hd->disp_fd, DISP_CMD_GET_SCN_WIDTH, ioctlParam);
	height = ioctl(vout_hd->disp_fd, DISP_CMD_GET_SCN_HEIGHT, ioctlParam);
	memset(&config, 0, sizeof(disp_layer_info));
	config.fb.src_win.x = 0;
	config.fb.src_win.y = 0;
	config.fb.src_win.width = 1;
	config.fb.src_win.height = 1;
	config.fb.size.width = width;
	config.fb.size.height = height;
	config.screen_win.x = 0;
	config.screen_win.y = 0;
	config.screen_win.width = width;
	config.screen_win.height = height;
	config.alpha_mode = 0;
	config.fb.pre_multiply = 0;
	config.alpha_value = 0xff;
	hlay = LayerRequest(&ch, &id);
	config.id = id;
	config.zorder = 0;
	config.mode = DISP_LAYER_WORK_MODE_SCALER;
	config.pipe = ch;
	DISP_DBG_LOG("hlay:%d, zorder=%d,", hlay, config.zorder);
#ifdef SUNXI_DISPLAY_GPU
	memset(&gpuRenderInfo, 0, sizeof(GpuRenderInfo));
	gpuRenderInfo.outInfo.dstx = 0;
	gpuRenderInfo.outInfo.dsty = 0;
	gpuRenderInfo.outInfo.dstwidth = width;
	gpuRenderInfo.outInfo.dstheight = height;
	gpuRenderInfo.outInfo.screenheight = height;
	gpuRenderInfo.outInfo.dstformat = FORMAT_RGBA8888;
	pthread_mutex_init(&gpuRenderInfo.mtx, NULL);
	pthread_mutex_init(&gpuRenderInfo.initMtx, NULL);
	pthread_cond_init(&gpuRenderInfo.cond, NULL);
	if (pthread_create(&gpuRenderInfo.gpuThreadId, NULL, GpuRenderThreadProc,
	        NULL)) {
		DISP_DBG_LOG("%s Create gpu thread failed!\r\n", __func__);
	}
#ifdef SUNXI_DISPLAY_GPU_BUF
	gpuRenderInfo.argbSrcInfo.crop_x = 0;
	gpuRenderInfo.argbSrcInfo.crop_y = 0;
	gpuRenderInfo.argbSrcInfo.crop_w = width;
	gpuRenderInfo.argbSrcInfo.crop_h = height;
	gpuRenderInfo.argbSrcInfo.w = width;
	gpuRenderInfo.argbSrcInfo.h = height;
	gpuRenderInfo.argbSrcInfo.format = VIDEO_PIXEL_FORMAT_ABGR;
	LayerSetPara(vout_hd->disp_fd, hlay, &config);
	disp_layer_off(vout_hd->disp_fd, hlay);
#endif
#else
	LayerSetPara(vout_hd->disp_fd, hlay, &config);
	disp_layer_off(vout_hd->disp_fd, hlay);
#endif
	vout_hd->hlayer = hlay;
	vout_hd->enable = enable;
	vout_hd->rotate = (rotateDegree) rotate;
	vout_hd->rect.x = rect->x;
	vout_hd->rect.y = rect->y;
	vout_hd->rect.width = rect->width;
	vout_hd->rect.height = rect->height;
	vout_hd->pMemops = GetMemAdapterOpsS();
	SunxiMemOpen(vout_hd->pMemops);
#ifdef SUNXI_DISPLAY_GPU
#ifdef SUNXI_DISPLAY_GPU_BUF
	gpuRenderInfo.vout_hd = vout_hd;
#endif
#endif
	errHdl: return ret;
#endif
}

int DispDeinit(void *hdl) {
#ifdef __SUNXI_DISPLAY2__
	int ret = 0;
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	LayerRelease(vout_hd->disp_fd, vout_hd->hlayer);

	ret = close(vout_hd->disp_fd);
	if (ret != 0) {
		DISP_DBG_LOG("%s:close disp handle failed!\r\n", __func__);
		goto errHdl;
	}
	SunxiMemClose(vout_hd->pMemops);
	errHdl: return ret;
#else
	dispOutPort *vout_hd = (dispOutPort*) hdl;
#ifdef SUNXI_DISPLAY_GPU
	gpuRenderInfo.gpuThreadRun = 0;
	pthread_cond_signal(&gpuRenderInfo.cond);
	pthread_mutex_destroy(&gpuRenderInfo.mtx);
	pthread_mutex_destroy(&gpuRenderInfo.initMtx);
	pthread_cond_destroy(&gpuRenderInfo.cond);
	pthread_join(gpuRenderInfo.gpuThreadId, NULL);
	gpuRenderInfo.gpuThreadId = 0;
#ifdef SUNXI_DISPLAY_GPU_BUF
	LayerRelease(vout_hd->disp_fd, vout_hd->hlayer);
#endif
#else
	LayerRelease(vout_hd->disp_fd, vout_hd->hlayer);
#endif
	if (vout_hd->disp_fd)
		close(vout_hd->disp_fd);
	if (vout_hd->pMemops)
		SunxiMemClose(vout_hd->pMemops);
	return 0;
#endif
}

#ifdef __SUNXI_DISPLAY__
int DispResize(void *hdl, __disp_fb_t *input_fb, __disp_fb_t *output_fb)
{
	int hdle;
	dispOutPort *vout_hd = (dispOutPort *)hdl;
	unsigned long int args[4]={0};
	__disp_scaler_para_t scaler_para;

	scaler_para.input_fb = *input_fb;

	scaler_para.source_regn.x = 0;
	scaler_para.source_regn.y = 0;
	scaler_para.source_regn.width = scaler_para.input_fb.size.width;
	scaler_para.source_regn.height = scaler_para.input_fb.size.height;
	scaler_para.output_fb = *output_fb;

	args[0] = 0;
	args[1] = 0;
	args[2] = 0;
	args[3] = 0;
	hdle = ioctl(vout_hd->disp_fd, DISP_CMD_SCALER_REQUEST, args);

	args[0] = 0;
	args[1] = hdle;
	args[2] = (unsigned long int)&scaler_para;
	args[3] = 0;
	ioctl(vout_hd->disp_fd, DISP_CMD_SCALER_EXECUTE, args);

	args[0] = 0;
	args[1] = hdle;
	args[2] = 0;
	args[3] = 0;
	hdle = ioctl(vout_hd->disp_fd, DISP_CMD_SCALER_RELEASE, args);
	DISP_DBG_LOG("(%s %d)\n", __FUNCTION__, __LINE__);

	return 0;
}
#endif

int DispSetCache(void *hdl, int enable) {
	dispOutPort *vout_hd = (dispOutPort*) hdl;
	unsigned long ioctlParam[4];

	DISP_CLAR(ioctlParam);
	ioctlParam[0] = SCREEN_0;
	ioctlParam[1] = enable;

#ifdef __SUNXI_DISPLAY2__
	ioctl(vout_hd->disp_fd, DISP_SHADOW_PROTECT, ioctlParam);
#else
	ioctl(vout_hd->disp_fd, DISP_SHADOW_PROTECT, ioctlParam);
#endif
	return 0;
}

dispOutPort* CreateVideoOutport(int index) {
	dispOutPort *voutport = (dispOutPort*) malloc(sizeof(dispOutPort));
	if (voutport == NULL)
		return NULL;
	voutport->init = DispInit;
	voutport->deinit = DispDeinit;
	voutport->writeData = DispWriteData;
	voutport->dequeue = DispDequeue;
	voutport->queueToDisplay = DispQueueToDisplay;
	voutport->setEnable = DispSetEnable;
	voutport->setRect = DispSetRect;
	voutport->setRectFake = DispSetRectFake;
	voutport->setRotateAngel = DispSetRotateAngel;
	voutport->getScreenWidth = DispGetScreenWidth;
	voutport->getScreenHeight = DispGetScreenHeight;
	voutport->setRoute = DispSetRoute;
	voutport->SetZorder = DispSetZorder;
	voutport->allocateVideoMem = DispAllocateVideoMem;
	voutport->freeVideoMem = DispFreeVideoMem;
	voutport->setSrcRect = DispSetSrcRect;
	voutport->setIoctl = DispSetIoctl;
	voutport->setCache = DispSetCache;
	return voutport;
}

int DestroyVideoOutport(dispOutPort *hdl) {
	if (hdl != NULL)
		free(hdl);

	return 0;
}
