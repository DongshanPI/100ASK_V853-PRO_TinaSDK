
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : vdecoder.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

#include <pthread.h>
#include <memory.h>
#include "vdecoder.h"
#include "sbm.h"
#include "fbm.h"
#include "videoengine.h"
#include "veAdapter.h"
#include "CdcIonUtil.h"

#include "memoryAdapter.h"
#include "gralloc_metadata/sunxi_metadata_def.h"

#include <cdc_log.h>
#include <cdc_version.h>
#include <stdio.h>

#include "sunxi_tr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "aftertreatment.h"
#include "CdcIniparserapi.h"
#include "CdcSinkInterface.h"
#include "CdcTimeUtil.h"


//* should include other  '*.h'  before CdcMalloc.h
#include "CdcMalloc.h"

#define DEBUG_SAVE_BITSTREAM    (0)
#define DEBUG_SAVE_PICTURE      (0)

/* show decoder speed  */
#define AW_VDECODER_SPEED_INFO  (0)
#define DEBUG_SAVE_FRAME_TIME   (0)
#define DEBUG_MAX_FRAME_IN_LIST 16

#define DEBUG_MAKE_SPECIAL_STREAM (0)
#define PIC_SAVE_FILE "/data/camera/picture.dat"
#define BIT_STREAM_FILE "/data/camera/bitstream.dat"

#define CHECK_METADATA_INFO (0)
#define DECODER_SHOW_DETIAL_LOG (0)

//* setup the ve freq

#ifdef CONF_VE_FREQ_ENABLE_SETUP
#ifdef CONF_CERES_VE_FREQ_ENABLE_SETUP
#define VE_FREQ_SETTING_VALUE (576) //* just for h265 on a100
#else
#define VE_FREQ_SETTING_VALUE (648) //* just for h265 on h6
#endif
#endif

//* for check the proc info
#define ENABLE_SET_PROC_INFO (0)
#define SET_PROC_INFO_FREQ   (60)


#define dy_log(fmt, arg...) do{ \
    if(p->bShowLogFlag == 1) \
        logd(fmt, arg); \
    else \
        logi(fmt, arg); \
}while(0)

int bDynamicShowLogFlag;
extern const char* strCodecFormat[];

const char* strDecodeResult[] =
{
    "UNSUPPORTED",
    "OK",
    "FRAME_DECODED",
    "CONTINU",
    "KEYFRAME_DECODED",
    "NO_FRAME_BUFFER",
    "NO_BITSTREAM",
    "RESOLUTION_CHANGE"
};

const char* strPixelFormat[] =
{
    "DEFAULT",          "YUV_PLANER_420",           "YUV_PLANER_420",
    "YUV_PLANER_444",   "YV12",                     "NV1",
    "YUV_MB32_420",     "MB32_422",                 "MB32_444",
};

typedef struct VideoDecoderDebugContext
{
    int      nFrameNum;
    int      nFrameNumDuration;
    int      nThreshHoldNum;
    float    nMaxCostTime;
    float    nCostTimeList[DEBUG_MAX_FRAME_IN_LIST];
    int64_t  nStartTime;
    int64_t  nDuration;
    int64_t  nCurrentTime;
    int64_t  nHardwareCurrentTime;
    int64_t  nHardwareDuration;
    int64_t  nHardwareTotalTime;
    int64_t  nGetOneFrameFinishTime;
    float    nExtraDecodeTimeList[DEBUG_MAX_FRAME_IN_LIST];
    float    nFrameTimeList[DEBUG_MAX_FRAME_IN_LIST];
    float    nFrameSmoothTime;
    int      nWaitingForDispPicNum[DEBUG_MAX_FRAME_IN_LIST];
    int      nValidDisplayPic;
    int      nStartIndexExtra;
    unsigned int *pFrameTime;
    int      nFrameTimeNum;
    unsigned int *pDispFrameNum;
    float    nMax_hardDecodeFps;
    float    nMin_hardDecodeFps;
}VideoDecoderDebugContext;

typedef struct VideoDecoderContext
{
    VConfig             vconfig;
    VideoStreamInfo     videoStreamInfo;
    VideoEngine*        pVideoEngine;
    int                 nFbmNum;
    Fbm*                pFbm[2];
    int                 nSbmNum;
    SbmInterface*       pSbm[2];
    VideoStreamDataInfo partialStreamDataInfo[2];

    VideoDecoderDebugContext debug;
    struct ScMemOpsS *memops;
    int                 bInterVe;
    ATMInstant          pATM;
    int                 nIonFd;
    int                 nDecFrameCount;

    int                 bSaveSpecialStreamFlag;

    int                 bShowSpeedInfo;
    int                 bShowLogFlag;
    CdcSinkInterface*   pBSSink;
    CdcSinkInterface*   pPicSink;
}VideoDecoderContext;

static int  DecideStreamBufferSize(VideoDecoderContext* p);
static void UpdateVideoStreamInfo(VideoDecoderContext* p, VideoPicture* pPicture);
extern void ConvertPixelFormat(VideoPicture* pPictureIn, VideoPicture* pPictureOut);
extern int  RotatePicture0Degree(VideoPicture* pPictureIn,
                             VideoPicture* pPictureOut,
                             int nGpuYAlign,
                             int nGpuCAlign);
extern int  RotatePicture90Degree(VideoPicture* pPictureIn, VideoPicture* pPictureOut);
extern int  RotatePicture180Degree(VideoPicture* pPictureIn, VideoPicture* pPictureOut);
extern int  RotatePicture270Degree(VideoPicture* pPictureIn, VideoPicture* pPictureOut);
int CheckSbmDataFirstLastFlag(VideoDecoder*        pDecoder,
                          VideoStreamDataInfo* pDataInfo,
                          int                  nStreamBufIndex);

#if(CHECK_METADATA_INFO)
static void printfMetaDataInfo(struct sunxi_metadata* pSunxi_metadata)
{

    logd("metadata--hdr: d_x: %d, %d, %d, d_y: %d, %d, %d, w_x = %d, w_y = %d,\
maxDML = %d, minDML = %d",
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_x[0],
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_x[1],
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_x[2],
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_y[0],
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_y[1],
         pSunxi_metadata->hdr_smetada.disp_master.display_primaries_y[2],
         pSunxi_metadata->hdr_smetada.disp_master.white_point_x,
         pSunxi_metadata->hdr_smetada.disp_master.white_point_y,
         pSunxi_metadata->hdr_smetada.disp_master.max_display_mastering_luminance,
         pSunxi_metadata->hdr_smetada.disp_master.min_display_mastering_luminance);
    logd("metadata--hdr: maxCLL = %d, maxFALL = %d",
         pSunxi_metadata->hdr_smetada.maximum_content_light_level ,
         pSunxi_metadata->hdr_smetada.maximum_frame_average_light_level);

    char* pSignature = (char*)&pSunxi_metadata->afbc_head.signature;
    logd("metadata-afbc: signature = %c%c%c%c, filehdrSize = %d, version = %d, bodySize = %d",
        pSignature[0],pSignature[1],pSignature[2],pSignature[3],
        pSunxi_metadata->afbc_head.filehdr_size,
        pSunxi_metadata->afbc_head.version,
        pSunxi_metadata->afbc_head.body_size);
    logd("metadata-afbc: ncomponents = %d, header_layout = %d, yuvTran = %d, blockSplit = %d",
        pSunxi_metadata->afbc_head.ncomponents,
        pSunxi_metadata->afbc_head.header_layout,
        pSunxi_metadata->afbc_head.yuv_transform,
        pSunxi_metadata->afbc_head.block_split);
    logd("metadata-afbc: inputbits: %d, %d, %d, %d",
        pSunxi_metadata->afbc_head.inputbits[0],
        pSunxi_metadata->afbc_head.inputbits[1],
        pSunxi_metadata->afbc_head.inputbits[2],
        pSunxi_metadata->afbc_head.inputbits[3]);
    logd("metadata-afbc: block_w = %d, block_h = %d, w = %d, h = %d",
        pSunxi_metadata->afbc_head.block_width,
        pSunxi_metadata->afbc_head.block_height,
        pSunxi_metadata->afbc_head.width,
        pSunxi_metadata->afbc_head.height);
    logd("metadata-afbc: left_crop = %d, top_crop = %d, block_layout = %d",
        pSunxi_metadata->afbc_head.left_crop,
        pSunxi_metadata->afbc_head.top_crop,
        pSunxi_metadata->afbc_head.block_layout);

};
#endif

static int DebugCheckConfig(VideoDecoderContext* p)
{
    CdcSinkParam nSinkParm;
    int bSaveStreamFlag = 0;
    int bSavePicFlag = 0;

    //* save bitstream
#if DEBUG_SAVE_BITSTREAM
    bSaveStreamFlag = 1;
#if DEBUG_MAKE_SPECIAL_STREAM
    p->bSaveSpecialStreamFlag = 1;
#endif
#endif

    if(bSaveStreamFlag == 0)
    {
        bSaveStreamFlag = CdcGetConfigParamterInt("vdecoder_save_bitstream", 0);
    }

    if(bSaveStreamFlag == 1)
    {
        nSinkParm.pFileName = CdcGetConfigParamterString("vdecoder_save_bitstream_path","null");
        p->bSaveSpecialStreamFlag =  CdcGetConfigParamterInt("vdecoder_save_special_bitstream", 0);
        if(nSinkParm.pFileName == NULL)
            nSinkParm.pFileName = BIT_STREAM_FILE;
        p->pBSSink = CdcSinkIFCreate(CDC_SINK_TYPE_BS);
        if(p->pBSSink)
        {
            nSinkParm.nLen = strlen(nSinkParm.pFileName) + 1;
            CdcSinkSetParam(p->pBSSink, &nSinkParm);
        }
    }

    //* save picture
#if DEBUG_SAVE_PICTURE
    bSavePicFlag = 1;
#endif

    if(bSavePicFlag == 0)
        bSavePicFlag = CdcGetConfigParamterInt("vdecoder_save_picture_en", 0);

    if(bSavePicFlag == 1)
    {
        nSinkParm.nStartNum = CdcGetConfigParamterInt("vdecoder_save_picture_start_num", 0);
        nSinkParm.nToatalNum = CdcGetConfigParamterInt("vdecoder_save_picture_total_num", 0);
        nSinkParm.pFileName = CdcGetConfigParamterString("vdecoder_save_picture_path","null");
        p->pPicSink = CdcSinkIFCreate(CDC_SINK_TYPE_PIC);
        if(nSinkParm.pFileName == NULL)
        {
            nSinkParm.nStartNum = 0;
            nSinkParm.nToatalNum = 10;
            nSinkParm.pFileName = PIC_SAVE_FILE;
            nSinkParm.bMd5Open = 1;
        }
        if(p->pPicSink)
        {
            nSinkParm.nLen = strlen(nSinkParm.pFileName) + 1;
            CdcSinkSetParam(p->pPicSink, &nSinkParm);
        }
    }

    //* show speed info
#if AW_VDECODER_SPEED_INFO
    p->bShowSpeedInfo = 1;
#endif
    if(p->bShowSpeedInfo == 0)
    {
        p->bShowSpeedInfo = CdcGetConfigParamterInt("vdecoder_show_speed_info", 0);
    }

    //* printf detail log
#if DECODER_SHOW_DETIAL_LOG
    p->bShowLogFlag = 1;
#endif

    if(p->bShowLogFlag == 0)
    {
        p->bShowLogFlag = CdcGetConfigParamterInt("vdecoder_printf_log", 0);
        log_set_level(CdcGetConfigParamterInt("cdc_log_level", 4));
        loge("now cedarc log level:%d", CdcGetConfigParamterInt("cdc_log_level", 4));
    }
    return 0;
}

VideoDecoder* CreateVideoDecoder(void)
{
    CdcMallocCheckFlag();

    bDynamicShowLogFlag = 0;

    //LogVersionInfo();

    VideoDecoderContext* p;

    p = (VideoDecoderContext*)malloc(sizeof(VideoDecoderContext));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }

    memset(p, 0, sizeof(VideoDecoderContext));

    //* the partialStreamDataInfo is for data submit status recording.
    p->partialStreamDataInfo[0].nStreamIndex = 0;
    p->partialStreamDataInfo[0].nPts         = -1;
    p->partialStreamDataInfo[1].nStreamIndex = 1;
    p->partialStreamDataInfo[1].nPts         = -1;

    DebugCheckConfig(p);
    logi("CreateVideoDecoder p:%p", p);

    return (VideoDecoder*)p;
}

void DestroyVideoDecoder(VideoDecoder* pDecoder)
{
    VideoDecoderContext* p;
    //int bSecureosEn = 0;
    //int bVirMallocSbm = 0;

    logi("DestroyVideoDecoder, pDecoder=%p", pDecoder);

    p = (VideoDecoderContext*)pDecoder;

    //bSecureosEn = p->vconfig.bSecureosEn;
    //bVirMallocSbm = p->vconfig.bVirMallocSbm;
#if DEBUG_SAVE_FRAME_TIME
    if(p->debug.pFrameTime != NULL)
    {
         FILE *fp = fopen("/data/camera/vdecoderFrameTime.txt", "wt");
         if(fp != NULL)
         {
             int i;
             loge(" vdecoder saving frame time. number: %d ", p->debug.nFrameTimeNum);
             for(i = 0; i < p->debug.nFrameTimeNum; i++)
             {
                 fprintf(fp, "%u\n", p->debug.pFrameTime[i]);
             }
             fclose(fp);
         }
         else
         {
             loge("vdecoder.c save frame open file error ");
         }
         free(p->debug.pFrameTime);
         p->debug.pFrameTime = NULL;
    }
    if(p->debug.pDispFrameNum != NULL)
    {
        FILE *fp = fopen("/data/camera/vdecoderHadSendDispNum.txt", "wt");
         if(fp != NULL)
         {
             int i;
             loge(" vdecoder saving frame time. number: %d ", p->debug.nFrameTimeNum);
             for(i = 0; i < p->debug.nFrameTimeNum; i++)
             {
                 fprintf(fp, "%u\n", p->debug.pDispFrameNum[i]);
             }
             fclose(fp);
         }
         else
         {
             loge("vdecoder.c save frame open file error ");
         }
         free(p->debug.pDispFrameNum);
         p->debug.pDispFrameNum = NULL;
    }

#endif //DEBUG_SAVE_FRAME_TIME

    //* Destroy the video engine first.
    if(p->pVideoEngine != NULL)
    {
        VideoEngineDestroy(p->pVideoEngine);
    }

    //* the memory space for codec specific data was allocated in InitializeVideoDecoder.
    if(p->videoStreamInfo.pCodecSpecificData != NULL)
    {
        free(p->videoStreamInfo.pCodecSpecificData);
        p->videoStreamInfo.pCodecSpecificData = NULL;
    }

    CdcMemClose(p->memops);

    if(p->bInterVe)
        CdcVeRelease(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);

    if(p->pBSSink)
        CdcSinkIFDestory(p->pBSSink, CDC_SINK_TYPE_BS);

    if(p->pPicSink)
        CdcSinkIFDestory(p->pPicSink, CDC_SINK_TYPE_PIC);

    free(p);
    p = NULL;

    //CdcMallocReport();
    return;
}

int InitializeVideoDecoder(VideoDecoder* pDecoder, VideoStreamInfo* pVideoInfo,  VConfig* pVconfig)
{
    VideoDecoderContext* p;
    int                  nStreamBufferSize;
    //int                  i;
    p = (VideoDecoderContext*)pDecoder;

    #ifdef CONF_VE_FREQ_ENABLE_SETUP
    if(pVconfig->nVeFreq == 0 && pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_H265)
        pVconfig->nVeFreq = VE_FREQ_SETTING_VALUE;
    #endif

    if(pVconfig->bThumbnailMode == 0)
    {
        LogVersionInfo();
        logd("*** pVconfig->nVeFreq = %d",pVconfig->nVeFreq);
    }

    if(pVconfig->bSetProcInfoEnable == 0)
    {
        pVconfig->bSetProcInfoEnable = ENABLE_SET_PROC_INFO;
        pVconfig->nSetProcInfoFreq   = SET_PROC_INFO_FREQ;
    }
    else
    {
        if(pVconfig->nSetProcInfoFreq == 0)
            pVconfig->nSetProcInfoFreq   = SET_PROC_INFO_FREQ;
    }
    if(pVconfig->bSecureosEn == 1)
        pVconfig->bSetProcInfoEnable = 0;

    //* check codec format.
    if(pVideoInfo->eCodecFormat > VIDEO_CODEC_FORMAT_MAX ||
        pVideoInfo->eCodecFormat <  VIDEO_CODEC_FORMAT_MIN)
    {
        loge("codec format(0x%x) invalid.", pVideoInfo->eCodecFormat);
        return -1;
    }

    if(pVideoInfo->nHeight > 3840 || pVideoInfo->nWidth > 6144)//>6k
    {
        loge("pVideoInfo->nWidth = %d, pVideoInfo->nHeight = %d unsupported!!",
             pVideoInfo->nWidth, pVideoInfo->nHeight);
        return VDECODE_RESULT_UNSUPPORTED;
    }

    //* mjpeg decoder just support 16-align
    if(pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_MJPEG
        && pVconfig->nAlignStride > 16)
    {
        int nWidth = pVideoInfo->nWidth;
        if(pVconfig->bScaleDownEn == 1 && pVconfig->nHorizonScaleDownRatio !=0)
            nWidth = pVideoInfo->nWidth >> pVconfig->nHorizonScaleDownRatio;

        if(nWidth > 0 && (nWidth%pVconfig->nAlignStride) != 0)
        {
            logw("change nAlignStride from %d to 16, \
            as width of mjpeg decoder just support 16-align", pVconfig->nAlignStride);
            pVconfig->nAlignStride = 16;
        }
    }

    if(pVconfig->memops == NULL)
    {
        logd("user not set memops, we get internal!");
        if(pVconfig->bSecureosEn == 1)
            p->memops = SecureMemAdapterGetOpsS();
        else
            p->memops = MemAdapterGetOpsS();
    }
    else
    {
        p->memops = pVconfig->memops;
    }

    if(pVconfig->veOpsS == NULL)
    {
        logd("user not set veops, we get internal!");
        pVconfig->veOpsS = GetVeOpsS(VE_OPS_TYPE_NORMAL);
        VeConfig mVeConfig;
        memset(&mVeConfig, 0, sizeof(VeConfig));
        mVeConfig.nDecoderFlag = 1;
        mVeConfig.nEncoderFlag = 0;
        mVeConfig.nFormat      = pVideoInfo->eCodecFormat;
        mVeConfig.nWidth       = pVideoInfo->nWidth;
        mVeConfig.nVeFreq      = pVconfig->nVeFreq;
        mVeConfig.memops       = p->memops;

        if(pVconfig->bIsTvStream == 1)
        {
            mVeConfig.nResetVeMode = RESET_VE_SPECIAL;
        }
        else
        {
            mVeConfig.nResetVeMode = RESET_VE_NORMAL;
        }
        pVconfig->pVeOpsSelf = CdcVeInit(pVconfig->veOpsS, &mVeConfig);

        uint64_t nICVersion = CdcVeGetIcVeVersion(pVconfig->veOpsS, pVconfig->pVeOpsSelf);
        if(nICVersion == 0x3101000012010
           && pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_VP9)
        {
            CdcVeRelease(pVconfig->veOpsS, pVconfig->pVeOpsSelf);
            pVconfig->veOpsS = GetVeOpsS(VE_OPS_TYPE_VP9);
            pVconfig->pVeOpsSelf = CdcVeInit(pVconfig->veOpsS,&mVeConfig);
        }
        p->bInterVe = 1;
    }

    pVconfig->memops = p->memops;
    if(CdcMemOpen2(p->memops,(void*)pVconfig->veOpsS, pVconfig->pVeOpsSelf) < 0)
        return -1;

    if(pVconfig->bATMFlag)
    {
        p->pATM = ATMCreate(p->memops, pVconfig->veOpsS, pVconfig->pVeOpsSelf);
        ATMInitialize(p->pATM);
    }
    //* print video stream information.
    {
        logv("Video Stream Information:");
        logv("     codec          = %s",
            strCodecFormat[pVideoInfo->eCodecFormat - VIDEO_CODEC_FORMAT_MIN]);
        logv("     width          = %d pixels", pVideoInfo->nWidth);
        logv("     height         = %d pixels", pVideoInfo->nHeight);
        logv("     frame rate     = %d",        pVideoInfo->nFrameRate);
        logv("     frame duration = %d us",     pVideoInfo->nFrameDuration);
        logv("     aspect ratio   = %d",        pVideoInfo->nAspectRatio);
        logv("     is 3D stream   = %s",        pVideoInfo->bIs3DStream ? "yes" : "no");
        logv("     csd data len   = %d",        pVideoInfo->nCodecSpecificDataLen);
        logv("     bIsFrameCtsTestFlag  = %d",  pVideoInfo->bIsFrameCtsTestFlag);
    }

    //* save the video stream information.
    p->videoStreamInfo.eCodecFormat          = pVideoInfo->eCodecFormat;
    p->videoStreamInfo.nWidth                = pVideoInfo->nWidth;
    p->videoStreamInfo.nHeight               = pVideoInfo->nHeight;
    p->videoStreamInfo.nFrameRate            = pVideoInfo->nFrameRate;
    p->videoStreamInfo.nFrameDuration        = pVideoInfo->nFrameDuration;
    p->videoStreamInfo.nAspectRatio          = pVideoInfo->nAspectRatio;
    p->videoStreamInfo.bIs3DStream           = pVideoInfo->bIs3DStream;
    p->videoStreamInfo.bIsFramePackage       = pVideoInfo->bIsFramePackage;
    p->videoStreamInfo.nCodecSpecificDataLen = pVideoInfo->nCodecSpecificDataLen;
    p->videoStreamInfo.bSecureStreamFlag     = pVideoInfo->bSecureStreamFlag;
    p->videoStreamInfo.bSecureStreamFlagLevel1 = pVideoInfo->bSecureStreamFlagLevel1;
    p->videoStreamInfo.bIsFrameCtsTestFlag = pVideoInfo->bIsFrameCtsTestFlag;

    if(p->videoStreamInfo.nCodecSpecificDataLen > 0)
    {
        int   nSize = p->videoStreamInfo.nCodecSpecificDataLen;
        char* pMem  = (char*)malloc(nSize);

        if(pMem == NULL)
        {
            p->videoStreamInfo.nCodecSpecificDataLen = 0;
            loge("memory alloc fail.");
            return -1;
        }

        memcpy(pMem, pVideoInfo->pCodecSpecificData, nSize);
        p->videoStreamInfo.pCodecSpecificData = pMem;
    }

    if(pVconfig->nDecodeSmoothFrameBufferNum <= 0
       || pVconfig->nDecodeSmoothFrameBufferNum > 16)
    {
        if(pVconfig->bThumbnailMode == 0)
        {
            logw("warning: the nDecodeSmoothFrameBufferNum is %d",
            pVconfig->nDecodeSmoothFrameBufferNum);
        }
    }
    if(pVconfig->nDeInterlaceHoldingFrameBufferNum <= 0
       || pVconfig->nDeInterlaceHoldingFrameBufferNum > 16)
    {
        if(pVconfig->bThumbnailMode == 0)
        {
            logw("warning: the nDeInterlaceHoldingFrameBufferNum is %d",
            pVconfig->nDeInterlaceHoldingFrameBufferNum);
        }
    }
    if(pVconfig->nDisplayHoldingFrameBufferNum <= 0
       || pVconfig->nDisplayHoldingFrameBufferNum > 16)
    {
        if(pVconfig->bThumbnailMode == 0)
        {
            logw("warning: the nDisplayHoldingFrameBufferNum is %d",
            pVconfig->nDisplayHoldingFrameBufferNum);
        }
    }
    if(pVideoInfo->nHeight >= 3840 || pVideoInfo->nWidth >= 5120)//5K
    {
        pVconfig->bScaleDownEn = 1;
        pVconfig->nHorizonScaleDownRatio = 1;
        pVconfig->nVerticalScaleDownRatio = 1;
    }

    //logi("=====enableAfbc = %d, ENABLE_AFBC = %d", pVconfig->bEnableAfbcFlag, ENABLE_AFBC);
    memcpy(&p->vconfig, pVconfig, sizeof(VConfig));

    //* create stream buffer.
    nStreamBufferSize = DecideStreamBufferSize(p);

    //* check and fix the configuration for decoder.
    //CheckConfiguration(p);

    //* create video engine.
    p->pVideoEngine = VideoEngineCreate(&p->vconfig, &p->videoStreamInfo);
    if(p->pVideoEngine == NULL)
    {
        loge("create video engine fail.");
        return -1;
    }

    SbmConfig mSbmConfig;
    memset(&mSbmConfig, 0, sizeof(SbmConfig));
    mSbmConfig.bVirFlag = p->vconfig.bVirMallocSbm;
    mSbmConfig.memops   = p->memops;
    mSbmConfig.nSbmBufferTotalSize = nStreamBufferSize;
    mSbmConfig.veOpsS   = p->vconfig.veOpsS;
    mSbmConfig.pVeOpsSelf = p->vconfig.pVeOpsSelf;
    mSbmConfig.bSecureVideoFlag = p->vconfig.bSecureosEn;
    mSbmConfig.nNaluLength = 4;

    int nSbmType = SBM_TYPE_STREAM;
    #if(ENABLE_SBM_FRAME_INTERFACE)
    if(pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_H265 &&
       p->pVideoEngine->bIsSoftwareDecoder == 0)
        nSbmType = SBM_TYPE_FRAME;
    else if(pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_H264)
    {
       nSbmType = SBM_TYPE_FRAME_AVC;
       if((pVideoInfo->nCodecSpecificDataLen > 0) &&
          (pVideoInfo->pCodecSpecificData != NULL))//frame package  or ts stream package
        {
            if((pVideoInfo->pCodecSpecificData[0] == 0x01) &&
               (pVideoInfo->nCodecSpecificDataLen >= 7))
            //the first byte 01, junge one sps or many sps
            {
                 mSbmConfig.nNaluLength = (pVideoInfo->pCodecSpecificData[4]&0x03) + 1;
                 logv("calculate mSbmConfig->nNaluLength= %d\n",mSbmConfig.nNaluLength);
            }
        }
    }
    else if(pVideoInfo->eCodecFormat == VIDEO_CODEC_FORMAT_AVS2)
    {
        nSbmType = SBM_TYPE_FRAME_AVS2;
    }
    else
        nSbmType = SBM_TYPE_STREAM;
    #endif

#if 0
    if(mSbmConfig.bSecureVideoFlag)
        nSbmType = SBM_TYPE_STREAM;
#endif

    p->pSbm[0] = GetSbmInterface(nSbmType);

    if(p->pSbm[0] == NULL)
    {
        loge("create stream buffer fail.");
        return -1;
    }

    if(SbmInit(p->pSbm[0], &mSbmConfig) != 0)
    {
        loge(" sbm init failed");
        //SbmDestroy(p->pSbm[0]);
        p->pSbm[0] = NULL;
        return -1;
    }
    VideoEngineSetSbm(p->pVideoEngine, p->pSbm[0], 0);
    p->nSbmNum++;

    if(p->videoStreamInfo.bIs3DStream)
    {
        p->pSbm[1] = GetSbmInterface(nSbmType);
        if(p->pSbm[1] == NULL)
        {
            loge("create stream buffer fail.");
            return -1;
        }
        if(SbmInit(p->pSbm[1], &mSbmConfig) != 0)
        {
            loge(" sbm init failed");
            SbmDestroy(p->pSbm[1]);
            p->pSbm[1] = NULL;
            return -1;
        }
        VideoEngineSetSbm(p->pVideoEngine, p->pSbm[1], 1);
        p->nSbmNum++;
    }

#if DEBUG_SAVE_FRAME_TIME
    if(p->debug.pFrameTime == NULL)
    {
        p->debug.pFrameTime  = calloc(1, 4*1024*1024);
        if(p->debug.pFrameTime == NULL)
        {
             loge(" save frame time calloc error  ");
        }
    }
    if(p->debug.pDispFrameNum == NULL)
    {
        p->debug.pDispFrameNum = calloc(1, 4*1024*1024);
        if(p->debug.pDispFrameNum == NULL)
        {
             loge(" save frame time calloc error  ");
        }
    }
#endif //DEBUG_SAVE_FRAME_TIM

    if(p->pBSSink != NULL)
    {
        logd("%d\n", p->videoStreamInfo.nCodecSpecificDataLen);
        logd("pts=0\n");
        CdcBitStream nBitSteram;
        nBitSteram.pData = (void*)p->videoStreamInfo.pCodecSpecificData;
        nBitSteram.nLen = p->videoStreamInfo.nCodecSpecificDataLen;
        nBitSteram.mHead.mMask[0] = 'a';
        nBitSteram.mHead.mMask[1] = 'w';
        nBitSteram.mHead.mMask[2] = 's';
        nBitSteram.mHead.mMask[3] = 'p';
        nBitSteram.mHead.mLen = nBitSteram.nLen;
        nBitSteram.mHead.mBSIndex = 0;
        nBitSteram.mHead.mPts = 0;
        if(p->bSaveSpecialStreamFlag)
            nBitSteram.nAddHead = 1;

        CdcSinkWriteBSData(p->pBSSink, &nBitSteram);
    }
    return 0;
}

void ResetVideoDecoder(VideoDecoder* pDecoder)
{
    int                  i;
    VideoDecoderContext* p;

    logv("ResetVideoDecoder, pDecoder=%p", pDecoder);

    p = (VideoDecoderContext*)pDecoder;
    //Version = VeGetIcVersion();

    //* reset the video engine.
    if(p->pVideoEngine)
    {
        VideoEngineReset(p->pVideoEngine);
    }

    //* flush pictures.
    if(p->nFbmNum == 0 && p->pVideoEngine != NULL)
    {
        p->nFbmNum = VideoEngineGetFbmNum(p->pVideoEngine);
    }

    for(i=0; i<p->nFbmNum; i++)
    {
        if(p->pFbm[i] == NULL && p->pVideoEngine != NULL)
        {
            p->pFbm[i] = VideoEngineGetFbm(p->pVideoEngine, i);
        }

        if(p->pFbm[i] != NULL)
            FbmFlush(p->pFbm[i]);
    }

    //* flush stream data.
    for(i=0; i<p->nSbmNum; i++)
    {
        if(p->pSbm[i] != NULL)
            SbmReset(p->pSbm[i]);
    }

    //* clear the partialStreamDataInfo.
    memset(p->partialStreamDataInfo, 0, sizeof(VideoStreamDataInfo)*2);

    return;
}

int DecoderSetSpecialData(VideoDecoder* pDecoder, void *pArg)
{
    int ret;
    VideoDecoderContext* p;
    DecoderInterface *pDecoderInterface;
    p = (VideoDecoderContext*)pDecoder;

    if(p == NULL ||
        p->pVideoEngine == NULL ||
        p->pVideoEngine->pDecoderInterface == NULL)
    {
        loge(" set decoder special data fail ");
        return VDECODE_RESULT_UNSUPPORTED;
    }

    pDecoderInterface = p->pVideoEngine->pDecoderInterface;
    ret = 0;
    if(pDecoderInterface->SetSpecialData != NULL)
    {
        ret = pDecoderInterface->SetSpecialData(pDecoderInterface, pArg);
    }
    return ret;
}

#if DEBUG_SAVE_FRAME_TIME
static int decoderSendToDisplayPictureNum(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;

    p    = (VideoDecoderContext*)pDecoder;
    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);
        logi("Fbm module of video stream %d not create yet, ValidPictureNum() fail.", nStreamIndex);
        return 0;
    }
    return FbmGetDisplayBufferNum(pFbm);
}
#endif

#define DEBUG_VDECODER_SWAP(a,b,tmp) {(tmp)=(a);(a)=(b);(b)=(tmp);}

static void DebugUpDateCostTimeList(float *list, float time)
{
    int i;
    float tmp;
    if(list == NULL)
        return;
    if(time <= list[0])
        return;
    list[0] = time;
    for(i = 0; i < DEBUG_MAX_FRAME_IN_LIST - 1; i++)
    {
        if(list[i+1] < time)
            DEBUG_VDECODER_SWAP(list[i+1], list[i], tmp)
        else
            break;
    }
}
static void DebugShowExtraTime(float *list, int index)
{
    int i, j;
    float f[DEBUG_MAX_FRAME_IN_LIST];
    j = index;
    for(i = 0; i <= DEBUG_MAX_FRAME_IN_LIST - 1; i++)
    {
        f[i] = list[j];
        j++;
        if(j >= DEBUG_MAX_FRAME_IN_LIST - 1)
            j = 0;
    }
    if(1)
    {
        float *l = f;
        logd("time [1-8]: %.3f, %.3f, %.3f, %.3f,  %.3f, %.3f, %.3f, %.3f",
                l[0],l[1],l[2],l[3], l[4],l[5],l[6],l[7]);
        logd("           [9-16]:  %.3f, %.3f, %.3f, %.3f,  %.3f, %.3f, %.3f, %.3f",
                l[8],l[9],l[10],l[11],  l[12],l[13],l[14],l[15]);
    }
}
static void DebugShowValidPicList(int *list, int index)
{
    int i, j;
    int f[DEBUG_MAX_FRAME_IN_LIST];
    j = index;
    for(i = 0; i <= DEBUG_MAX_FRAME_IN_LIST - 1; i++)
    {
        f[i] = list[j];
        j++;
        if(j >= DEBUG_MAX_FRAME_IN_LIST - 1)
            j = 0;
    }
    if(1)
    {
        int *l = f;
        logd(" when decoding the last 16 frame, waiting for display number   ");
        logd("valid frame [1-8]: %d, %d, %d, %d,  %d, %d, %d, %d",
                l[0],l[1],l[2],l[3], l[4],l[5],l[6],l[7]);
        logd("           [9-16]:  %d, %d, %d, %d,  %d, %d, %d, %d",
                l[8],l[9],l[10],l[11],  l[12],l[13],l[14],l[15]);
    }
}

static void DebugDecoderSpeedInfo(VideoDecoder* pDecoder)
{
    VideoDecoderContext* p;
    int nDeltaFrame = 64;
    int64_t hwtime;
    int64_t nCurrentFrameCostTime;
    int     nCostTime;
    float   fCostTime;
    p = (VideoDecoderContext*)pDecoder;
    hwtime = DBGetCurrentTime();
    nCurrentFrameCostTime = (hwtime - p->debug.nHardwareCurrentTime);
    nCostTime = nCurrentFrameCostTime;
    fCostTime = nCostTime/1000.0;
#if DEBUG_SAVE_FRAME_TIME
    if(p->debug.pFrameTime != NULL)
    {
        p->debug.pFrameTime[p->debug.nFrameTimeNum] = nCurrentFrameCostTime;
    }
    if(p->debug.pDispFrameNum != NULL)
    {
        int num = decoderSendToDisplayPictureNum(pDecoder, 0);
        p->debug.pDispFrameNum[p->debug.nFrameTimeNum] = num;
    }
    p->debug.nFrameTimeNum++;
#endif //DEBUG_SAVE_FRAME_TIME
    if(p->debug.nFrameNum > 8 && fCostTime > p->debug.nCostTimeList[0])
    {
        if(fCostTime > p->debug.nMaxCostTime)
            p->debug.nMaxCostTime = fCostTime;
        if(fCostTime >= 34.0)
            p->debug.nThreshHoldNum++;
        DebugUpDateCostTimeList(p->debug.nCostTimeList, fCostTime);
    }
    if(p->debug.nFrameNum > 8)
    {
        int64_t last = p->debug.nGetOneFrameFinishTime;
         p->debug.nGetOneFrameFinishTime = DBGetCurrentTime();
         if(1)
         {
             int64_t nExtraTime = 0;
             int nExtTimeInt;
             float fExtTime;
            nExtraTime = p->debug.nGetOneFrameFinishTime - last;
            nExtTimeInt = nExtraTime;
            fExtTime = nExtTimeInt/1000.0;
            p->debug.nExtraDecodeTimeList[p->debug.nStartIndexExtra] = fExtTime - fCostTime;
            p->debug.nFrameTimeList[p->debug.nStartIndexExtra] = fCostTime;
             p->debug.nWaitingForDispPicNum[p->debug.nStartIndexExtra] = p->debug.nValidDisplayPic;
            p->debug.nStartIndexExtra++;
            if(p->debug.nStartIndexExtra >= DEBUG_MAX_FRAME_IN_LIST - 1)
                p->debug.nStartIndexExtra = 0;
         }
    }
    if(fCostTime < 1.0)
    {
        logd("warning, maybe drop frame enable, cost time: %.2f ms", fCostTime);
        return;
    }
    p->debug.nHardwareDuration += nCurrentFrameCostTime;
    p->debug.nFrameNumDuration += 1;
    p->debug.nFrameNum += 1;
    if(p->debug.nFrameNumDuration >= nDeltaFrame)
    {
        int64_t nTime, nTotalDiff, hwDiff, hwAvgTime;
        nTime = DBGetCurrentTime();
        p->debug.nHardwareTotalTime += p->debug.nHardwareDuration;
        p->debug.nDuration = nTime - p->debug.nCurrentTime;
        if(1)
        {
            float PlayerCurrFps, PlayerAverageFps, HardwareCurrFps, HardwareAvgFps;
            nTotalDiff = (nTime - p->debug.nStartTime) / 1000; /* ms */
            nTime = p->debug.nDuration / 1000;    /* ms */
            hwDiff = p->debug.nHardwareDuration / 1000; /* ms */
            hwAvgTime = p->debug.nHardwareTotalTime / 1000;
            PlayerCurrFps = (float)(p->debug.nFrameNumDuration * 1000.0) / nTime;
            PlayerAverageFps = (float)(p->debug.nFrameNum * 1000.0) / nTotalDiff;
            HardwareCurrFps = (float)(p->debug.nFrameNumDuration * 1000.0) / hwDiff;
            HardwareAvgFps = (float)(p->debug.nFrameNum * 1000.0) / hwAvgTime;

            if(HardwareCurrFps < p->debug.nMin_hardDecodeFps || p->debug.nMin_hardDecodeFps == 0)
                p->debug.nMin_hardDecodeFps = HardwareCurrFps;

            if(HardwareCurrFps > p->debug.nMax_hardDecodeFps || p->debug.nMax_hardDecodeFps == 0)
                p->debug.nMax_hardDecodeFps = HardwareCurrFps;

            logd("hardware speed: avr = %.2f fps, max = %.2f fps, min = %.2f fps",
                  HardwareAvgFps, p->debug.nMax_hardDecodeFps, p->debug.nMin_hardDecodeFps);

            logd("player speed: %.2ffps, \
                    average: %.2ffps;slowest frame cost time: %.2f ms; \
                    hardware speed: %.2ffps, average: %.2ffps", \
                    PlayerCurrFps, PlayerAverageFps, p->debug.nMaxCostTime,\
                    HardwareCurrFps, HardwareAvgFps);
            logv(" 0527 vdecoder hardware speed: %.2f fps, \
                    slowest frame cost time: %.2f ms,  average speed: %.2f fps", \
                    HardwareCurrFps, p->debug.nMaxCostTime, HardwareAvgFps);
            logv(" 0527  vdecoder   player current speed: %.2f fps; player average speed: %.2f fps",
                    PlayerCurrFps, PlayerAverageFps);
        }
        if(0)
        {
            float *l = p->debug.nCostTimeList;
            logd("cost most time frame list[1-8]: \
                    %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f", \
                    l[0],l[1],l[2],l[3], l[4],l[5],l[6],l[7]);
            logd("cost most time frame list[9-16]: \
                    %.2f, %.2f, %.2f, %.2f,  %.2f, %.2f, %.2f, %.2f", \
                    l[8],l[9],l[10],l[11],  l[12],l[13],l[14],l[15]);
            logd("decoded frame number: %d,  cost time >= 34ms frame number: %d", \
                    p->debug.nFrameNum, p->debug.nThreshHoldNum);
        }
         if(0)
         {
             logd("current frame cost time: %.3fms last 16 frame extra time:  ", fCostTime);
             DebugShowExtraTime(p->debug.nExtraDecodeTimeList, p->debug.nStartIndexExtra);
             logd("    last 16 frame decode time: , smooth time: %.3f ", p->debug.nFrameSmoothTime);
             p->debug.nFrameSmoothTime = 0;
             DebugShowExtraTime(p->debug.nFrameTimeList, p->debug.nStartIndexExtra);
             DebugShowValidPicList(p->debug.nWaitingForDispPicNum, p->debug.nStartIndexExtra);
             logd(" ");
         }
        p->debug.nDuration = 0;
        p->debug.nHardwareDuration = 0;
        p->debug.nFrameNumDuration = 0;
        p->debug.nCurrentTime = DBGetCurrentTime();
    }
}

static int updateProcInfo(VideoDecoderContext* p)
{
    logv("updateProcInfo");
    char* pDebugInfo = NULL;
    int count = 0;

    pDebugInfo = (char*)calloc(1, 10*1024);
    if(pDebugInfo == NULL)
    {
        loge("malloc for pDebugInfo falied");
        return -1;
    }

    count += sprintf(pDebugInfo + count, "dec: codec[0x%x], scaledown[%d,%d,%d], rotate[%d,%d], \
extraNum[%d,%d,%d,%d]\n",
        p->videoStreamInfo.eCodecFormat,
        p->vconfig.bScaleDownEn,
        p->vconfig.nHorizonScaleDownRatio,
        p->vconfig.nVerticalScaleDownRatio,
        p->vconfig.bRotationEn,
        p->vconfig.nRotateDegree,
        p->vconfig.nDeInterlaceHoldingFrameBufferNum,
        p->vconfig.nDisplayHoldingFrameBufferNum,
        p->vconfig.nRotateHoldingFrameBufferNum,
        p->vconfig.nDecodeSmoothFrameBufferNum);

    logv("count = %d",count);
    CdcVeProcInfoUpdate(p->vconfig.veOpsS,p->vconfig.pVeOpsSelf,
                         pDebugInfo, count, 0);

    free(pDebugInfo);

    return 0;
}

int DecodeVideoStream(VideoDecoder* pDecoder,
                      int           bEndOfStream,
                      int           bDecodeKeyFrameOnly,
                      int           bDropBFrameIfDelay,
                      int64_t       nCurrentTimeUs)
{
    int                  ret;
    VideoDecoderContext* p;
    int bUpdateProcInfo = 0;

    logi("DecodeVideoStream, pDecoder=%p, \
            bEndOfStream=%d, bDropBFrameIfDelay=%d, nCurrentTimeUs=%lld", \
            pDecoder, bEndOfStream, bDropBFrameIfDelay, (long long int)nCurrentTimeUs);

   p = (VideoDecoderContext*)pDecoder;

   //eVersion = VeGetIcVersion();

    if(p->pVideoEngine == NULL)
        return VDECODE_RESULT_UNSUPPORTED;

    if(p->bShowSpeedInfo == 1)
    {
        p->debug.nHardwareCurrentTime = DBGetCurrentTime();
        if(p->debug.nCurrentTime == 0)
            p->debug.nCurrentTime = p->debug.nHardwareCurrentTime;
        if(p->debug.nStartTime == 0)
            p->debug.nStartTime = p->debug.nHardwareCurrentTime;
        p->debug.nValidDisplayPic = ValidPictureNum(pDecoder, 0);
    }

    if(bEndOfStream == 1)
    {
        if(p->pSbm[0])
            SbmSetEos(p->pSbm[0], bEndOfStream);
        if(p->pSbm[1])
            SbmSetEos(p->pSbm[1], bEndOfStream);
    }

    if(p->videoStreamInfo.bSecureStreamFlagLevel1)
        CdcMemSetup(p->memops);

    if(p->vconfig.bSetProcInfoEnable && p->vconfig.nSetProcInfoFreq > 0
        && p->nDecFrameCount%p->vconfig.nSetProcInfoFreq == 0)
    {
        bUpdateProcInfo = 1;
    }

    //* start update proc info
    if(bUpdateProcInfo)
    {
        CdcVeProcInfoReset(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);
    }

    //* update
    if(bUpdateProcInfo)
    {
        updateProcInfo(p);

        if(p->pSbm[0])
            SbmUpdateProcInfo(p->pSbm[0]);
        if(p->pSbm[1])
            SbmUpdateProcInfo(p->pSbm[1]);

        if(p->pFbm[0])
            FbmUpdateProcInfo(p->pFbm[0]);

        if(p->pFbm[1])
            FbmUpdateProcInfo(p->pFbm[1]);
    }

    //* also can update proc info in the inter-decode-function
    ret = VideoEngineDecode(p->pVideoEngine,
                            bEndOfStream,
                            bDecodeKeyFrameOnly,
                            bDropBFrameIfDelay,
                            nCurrentTimeUs);
    //* finish update proc info
    if(bUpdateProcInfo)
    {
        CdcVeProcInfoUpdate(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf, NULL, 0, 1);
    }

    if(p->videoStreamInfo.bSecureStreamFlagLevel1)
        CdcMemShutdown(p->memops);
    dy_log("decode method return %s", strDecodeResult[ret-VDECODE_RESULT_MIN]);

    if(ret == VDECODE_RESULT_FRAME_DECODED ||
       ret == VDECODE_RESULT_KEYFRAME_DECODED)
    {
        p->nDecFrameCount++;
        if(p->pATM != NULL)
            ATMAfterTreat(p->pATM);
    }


    if(p->bShowSpeedInfo == 1
        && (ret == VDECODE_RESULT_FRAME_DECODED
            || ret == VDECODE_RESULT_KEYFRAME_DECODED))
    {
        DebugDecoderSpeedInfo(pDecoder);
    }

    //* if in thumbnail mode and picture decoded,
    //* reset the video engine to flush the picture out to FBM.
    if(p->vconfig.bThumbnailMode)
    {
        if(ret == VDECODE_RESULT_FRAME_DECODED || ret == VDECODE_RESULT_KEYFRAME_DECODED)
        {
            //VideoEngineReset(p->pVideoEngine);
        }
    }
    else if(bEndOfStream && ret == VDECODE_RESULT_NO_BITSTREAM)
    {
        VideoEngineReset(p->pVideoEngine);
    }

    return ret;
}

int GetVideoStreamInfo(VideoDecoder* pDecoder, VideoStreamInfo* pVideoInfo)
{
    VideoDecoderContext* p;

    logv("GetVideoStreamInfo, pDecoder=%p, pVideoInfo=%p", pDecoder, pVideoInfo);

    p = (VideoDecoderContext*)pDecoder;

    //* set the video stream information.
    pVideoInfo->eCodecFormat   = p->videoStreamInfo.eCodecFormat;
    pVideoInfo->nWidth         = p->videoStreamInfo.nWidth;
    pVideoInfo->nHeight        = p->videoStreamInfo.nHeight;
    pVideoInfo->nFrameRate     = p->videoStreamInfo.nFrameRate;
    pVideoInfo->nFrameDuration = p->videoStreamInfo.nFrameDuration;
    pVideoInfo->nAspectRatio   = p->videoStreamInfo.nAspectRatio;
    pVideoInfo->bIs3DStream    = p->videoStreamInfo.bIs3DStream;

    //* print video stream information.
    {
        logi("Video Stream Information:");
        logi("     codec          = %s",
            strCodecFormat[pVideoInfo->eCodecFormat - VIDEO_CODEC_FORMAT_MIN]);
        logi("     width          = %d pixels", pVideoInfo->nWidth);
        logi("     height         = %d pixels", pVideoInfo->nHeight);
        logi("     frame rate     = %d",        pVideoInfo->nFrameRate);
        logi("     frame duration = %d us",     pVideoInfo->nFrameDuration);
        logi("     aspect ratio   = %d",        pVideoInfo->nAspectRatio);
        logi("     is 3D stream   = %s",        pVideoInfo->bIs3DStream ? "yes" : "no");
    }

    return 0;
}

int RequestVideoStreamBuffer(VideoDecoder* pDecoder,
                             int           nRequireSize,
                             char**        ppBuf,
                             int*          pBufSize,
                             char**        ppRingBuf,
                             int*          pRingBufSize,
                             int           nStreamBufIndex)
{
    char*                pStart;
    char*                pStreamBufEnd;
    char*                pMem;
    int                  nFreeSize;
    SbmInterface*        pSbm;
    VideoDecoderContext* p;

    logi("RequestVideoStreamBuffer, pDecoder=%p, nRequireSize=%d, nStreamBufIndex=%d",
            pDecoder, nRequireSize, nStreamBufIndex);

    p = (VideoDecoderContext*)pDecoder;

    *ppBuf        = NULL;
    *ppRingBuf    = NULL;
    *pBufSize     = 0;
    *pRingBufSize = 0;

    pSbm          = p->pSbm[nStreamBufIndex];

    if(pSbm == NULL)
    {
        logw("pSbm of video stream %d is NULL, RequestVideoStreamBuffer fail.", nStreamBufIndex);
        return -1;
    }

    //* sometimes AVI parser will pass empty stream frame to help pts calculation.
    //* in this case give four bytes even the parser does not need.
    if(nRequireSize == 0)
        nRequireSize = 4;

    //* we've filled partial frame data but not added to the SBM before,
    //* we need to calculate the actual buffer pointer by self.
    nRequireSize += p->partialStreamDataInfo[nStreamBufIndex].nLength;
    if(SbmRequestBuffer(pSbm, nRequireSize, &pMem, &nFreeSize) < 0)
    {
        logw("request stream buffer fail,  \
                %d bytes valid data in SBM[%d], total buffer size is %d bytes.",
                SbmStreamDataSize(pSbm),
                nStreamBufIndex,
                SbmBufferSize(pSbm));

        return -1;
    }

    //* check the free buffer is larger than the partial data we filled before.
    if(nFreeSize <= p->partialStreamDataInfo[nStreamBufIndex].nLength)
    {
        logw("require stream buffer get %d bytes, \
                but this buffer has been filled with partial \
                frame data of %d bytes before, nStreamBufIndex=%d.",
                nFreeSize, p->partialStreamDataInfo[nStreamBufIndex].nLength, nStreamBufIndex);

        return -1;
    }

    //* calculate the output buffer pos.
    pStreamBufEnd = (char*)SbmBufferAddress(pSbm) + SbmBufferSize(pSbm);
    pStart        = pMem + p->partialStreamDataInfo[nStreamBufIndex].nLength;
    if(pStart >= pStreamBufEnd)
        pStart -= SbmBufferSize(pSbm);
    nFreeSize -= p->partialStreamDataInfo[nStreamBufIndex].nLength;

    if(pStart + nFreeSize <= pStreamBufEnd) //* check if buffer ring back.
    {
        *ppBuf    = pStart;
        *pBufSize = nFreeSize;
    }
    else
    {
        //* the buffer ring back.
        *ppBuf        = pStart;
        *pBufSize     = pStreamBufEnd - pStart;
        *ppRingBuf    = SbmBufferAddress(pSbm);
        *pRingBufSize = nFreeSize - *pBufSize;
        logi("stream buffer %d ring back.", nStreamBufIndex);
    }

    return 0;
}

void FlushSbmData(VideoDecoderContext* p, SbmInterface*pSbm,
                       VideoStreamDataInfo* pPartialStreamDataInfo)
{
    if((p->vconfig.bVirMallocSbm ==0) &&(pSbm->bUseNewVeMemoryProgram==0))
    {
        //* we need to flush data from cache to memory for the VE hardware module.
        char* pStreamBufEnd = (char*)SbmBufferAddress(pSbm) + SbmBufferSize(pSbm);
        if(pPartialStreamDataInfo->pData + pPartialStreamDataInfo->nLength <= pStreamBufEnd)
        {
            CdcMemFlushCache(p->memops, \
                pPartialStreamDataInfo->pData, \
                pPartialStreamDataInfo->nLength);
        }
        else
        {
            //* buffer ring back.
            int nPartialLen = pStreamBufEnd - pPartialStreamDataInfo->pData;
            CdcMemFlushCache(p->memops, pPartialStreamDataInfo->pData, nPartialLen);
            CdcMemFlushCache(p->memops, \
                SbmBufferAddress(pSbm), \
                pPartialStreamDataInfo->nLength - nPartialLen);

        }
    }
}

static void DebugSaveBitStream(VideoDecoderContext* p, int nIndex, int nLen, u64 nPts)
{
    SbmInterface* pSbm        = NULL;
    CdcBitStream nBitSteram;
    char* pWriteAddr          = NULL;
    char* pSbmBufferStartAddr = NULL;
    char* pSbmBufferEndAddr   = NULL;
    int   nSbmBufferSize      = 0;

    pSbm = p->pSbm[nIndex];
    if(p->pBSSink != NULL)
    {
        logd("lenth=%d\n", nLen);
        logd("pts=%lld\n", (long long)nPts);
        pWriteAddr          = SbmBufferWritePointer(pSbm);
        pSbmBufferStartAddr = SbmBufferAddress(pSbm);
        nSbmBufferSize      = SbmBufferSize(pSbm);
        pSbmBufferEndAddr   = pSbmBufferStartAddr + nSbmBufferSize - 1;
        logv("%p, %p, %p, %d",pWriteAddr, pSbmBufferStartAddr,
                              pSbmBufferEndAddr, nSbmBufferSize);
        if(p->bSaveSpecialStreamFlag == 1)
            nBitSteram.nAddHead = 1;

        if(pWriteAddr+nLen <= pSbmBufferEndAddr)
        {
            nBitSteram.pData = (void*)pWriteAddr;
            nBitSteram.nLen = nLen;
            CdcSinkWriteBSData(p->pBSSink, &nBitSteram);
        }
        else
        {
            nBitSteram.pData = (void*)pWriteAddr;
            nBitSteram.nLen = pSbmBufferEndAddr-pWriteAddr+1;
            nBitSteram.mHead.mMask[0] = 'a';
            nBitSteram.mHead.mMask[1] = 'w';
            nBitSteram.mHead.mMask[2] = 's';
            nBitSteram.mHead.mMask[3] = 'p';
            nBitSteram.mHead.mLen = nBitSteram.nLen;
            nBitSteram.mHead.mBSIndex = nIndex;
            nBitSteram.mHead.mPts = nPts;
            CdcSinkWriteBSData(p->pBSSink, &nBitSteram);

            nBitSteram.pData = pSbmBufferStartAddr;
            nBitSteram.nLen = nLen-(pSbmBufferEndAddr-pWriteAddr+1);
            nBitSteram.nAddHead = 0;
            CdcSinkWriteBSData(p->pBSSink, &nBitSteram);
        }
    }
}

int SubmitVideoStreamData(VideoDecoder*        pDecoder,
                          VideoStreamDataInfo* pDataInfo,
                          int                  nStreamBufIndex)
{
    SbmInterface*                 pSbm;
    VideoDecoderContext* p;
    VideoStreamDataInfo* pPartialStreamDataInfo;

    p = (VideoDecoderContext*)pDecoder;

    dy_log("Submit, pDecoder=%p, pDataInfo=%p, Index=%d, len = %d, pts = %lld",
            pDecoder, pDataInfo, nStreamBufIndex,
            pDataInfo->nLength, (long long int)pDataInfo->nPts);

    pSbm = p->pSbm[nStreamBufIndex];

    if(pSbm == NULL)
    {
        logw("pSbm of video stream %d is NULL, SubmitVideoStreamData fail.", nStreamBufIndex);
        return -1;
    }

    #if 0  // some mjpeg stream does not have the 0xFFD9
    if(p->videoStreamInfo.eCodecFormat == VIDEO_CODEC_FORMAT_MJPEG)
    {
        CheckSbmDataFirstLastFlag(pDecoder, pDataInfo, nStreamBufIndex);
    }
    #endif

    pPartialStreamDataInfo = &p->partialStreamDataInfo[nStreamBufIndex];

    //* chech wheter a new stream frame.
    if(pDataInfo->bIsFirstPart)
    {
        if(pPartialStreamDataInfo->nLength != 0)    //* last frame is not complete yet.
        {
            logv("stream data frame uncomplete.");
            DebugSaveBitStream(p, nStreamBufIndex, pPartialStreamDataInfo->nLength, pPartialStreamDataInfo->nPts);
            FlushSbmData(p, pSbm, pPartialStreamDataInfo);
            SbmAddStream(pSbm, pPartialStreamDataInfo);
        }

        //* set the data address and pts.
        pPartialStreamDataInfo->pData        = pDataInfo->pData;
        pPartialStreamDataInfo->nLength      = pDataInfo->nLength;
        pPartialStreamDataInfo->nPts         = pDataInfo->nPts;
        pPartialStreamDataInfo->nPcr         = pDataInfo->nPcr;
        pPartialStreamDataInfo->bIsFirstPart = pDataInfo->bIsFirstPart;
        pPartialStreamDataInfo->bIsLastPart  = 0;
        pPartialStreamDataInfo->bVideoInfoFlag = pDataInfo->bVideoInfoFlag;
        pPartialStreamDataInfo->pVideoInfo     = pDataInfo->pVideoInfo;
        pPartialStreamDataInfo->bValid       = pDataInfo->bValid;
    }
    else
    {
        if(pPartialStreamDataInfo->pData == NULL)
        {
            logd("pPartialStreamDataInfo->pData == NULL");
            pPartialStreamDataInfo->pData = pDataInfo->pData;
        }
        pPartialStreamDataInfo->nLength += pDataInfo->nLength;
        if(pPartialStreamDataInfo->nPts == -1 && pDataInfo->nPts != -1)
            pPartialStreamDataInfo->nPts = pDataInfo->nPts;
    }

    //* check whether a stream frame complete.
    if(pDataInfo->bIsLastPart)
    {
        if(pPartialStreamDataInfo->pData != NULL && pPartialStreamDataInfo->nLength != 0)
        {
            FlushSbmData(p, pSbm, pPartialStreamDataInfo);
        }
        else
        {
            //* maybe it is a empty frame for MPEG4 decoder from AVI parser.
            logw("empty stream data frame submitted, pData=%p, nLength=%d",
                pPartialStreamDataInfo->pData, pPartialStreamDataInfo->nLength);
        }

        //* submit stream frame to the SBM.
        DebugSaveBitStream(p, nStreamBufIndex, pPartialStreamDataInfo->nLength, pPartialStreamDataInfo->nPts);

        SbmAddStream(pSbm, pPartialStreamDataInfo);

        //* clear status of stream data info.
        pPartialStreamDataInfo->pData        = NULL;
        pPartialStreamDataInfo->nLength      = 0;
        pPartialStreamDataInfo->nPts         = -1;
        pPartialStreamDataInfo->nPcr         = -1;
        pPartialStreamDataInfo->bIsLastPart  = 0;
        pPartialStreamDataInfo->bIsFirstPart = 0;
        pPartialStreamDataInfo->bVideoInfoFlag = 0;
        pPartialStreamDataInfo->pVideoInfo     = NULL;
        pPartialStreamDataInfo->bValid         = 0;
    }

    return 0;
}

void* VideoStreamBufferAddress(VideoDecoder* pDecoder, int nStreamBufIndex)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;
    logi("VideoStreamBufferSize, nStreamBufIndex=%d", nStreamBufIndex);
    if(p->pSbm[nStreamBufIndex] == NULL)
        return NULL;
    return SbmBufferAddress(p->pSbm[nStreamBufIndex]);
}

int VideoStreamBufferSize(VideoDecoder* pDecoder, int nStreamBufIndex)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;

    logi("VideoStreamBufferSize, nStreamBufIndex=%d", nStreamBufIndex);

    if(p->pSbm[nStreamBufIndex] == NULL)
        return 0;

    return SbmBufferSize(p->pSbm[nStreamBufIndex]);
}

int VideoStreamDataSize(VideoDecoder* pDecoder, int nStreamBufIndex)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;

    logi("VideoStreamDataSize, nStreamBufIndex=%d", nStreamBufIndex);

    if(p->pSbm[nStreamBufIndex] == NULL)
        return 0;

    return SbmStreamDataSize(p->pSbm[nStreamBufIndex]);
}

int VideoStreamFrameNum(VideoDecoder* pDecoder, int nStreamBufIndex)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;

    logi("VideoStreamFrameNum, nStreamBufIndex=%d", nStreamBufIndex);

    if(p->pSbm[nStreamBufIndex] == NULL)
        return 0;

    return SbmStreamFrameNum(p->pSbm[nStreamBufIndex]);
}

void* VideoStreamDataInfoPointer(VideoDecoder* pDecoder, int nStreamBufIndex)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;

    logi("VideoStreamDataSize, nStreamBufIndex=%d", nStreamBufIndex);

    if(p->pSbm[nStreamBufIndex] == NULL)
        return NULL;

    return SbmBufferDataInfo(p->pSbm[nStreamBufIndex]);
}

static void DebugSavePicture(VideoDecoderContext* p, VideoPicture* pPicture)
{
    CdcSinkPicture mPic;

    if(pPicture->pData0 == NULL)
    {
        logw("save picture failed: pPicture->pData0 is null");
        return ;
    }
    if(p->pPicSink != NULL)
    {
        switch(pPicture->ePixelFormat)
        {
            case PIXEL_FORMAT_YV12:
            case PIXEL_FORMAT_YUV_PLANER_420:
                if(pPicture->ePixelFormat == PIXEL_FORMAT_YUV_PLANER_420)
                    mPic.nFormat = CDC_SINK_PIC_FORMAT_YU12;
                else
                    mPic.nFormat = CDC_SINK_PIC_FORMAT_YV12;
                mPic.pData[0] = pPicture->pData0;
                mPic.pData[1] = (char*)mPic.pData[0] + pPicture->nWidth * pPicture->nHeight;
                mPic.pData[2] = (char*)mPic.pData[1] + pPicture->nWidth * pPicture->nHeight/2;
                break;
            case PIXEL_FORMAT_NV12:
            case PIXEL_FORMAT_NV21:
                if(pPicture->ePixelFormat == PIXEL_FORMAT_NV12)
                    mPic.nFormat = CDC_SINK_PIC_FORMAT_NV12;
                else
                    mPic.nFormat = CDC_SINK_PIC_FORMAT_NV21;
                mPic.pData[0] = pPicture->pData0;
                mPic.pData[1] = (char*)mPic.pData[0] + pPicture->nWidth * pPicture->nHeight;
                break;
            default:
                logw("not support now");
        }
        mPic.nWidth  = pPicture->nWidth;
        mPic.nHeight = pPicture->nHeight;
        mPic.nAlign  = 16;
        mPic.nBpp    = pPicture->b10BitPicFlag;
        mPic.nAfbc   = pPicture->bEnableAfbcFlag;
        mPic.nAfbcSize = pPicture->nAfbcSize;
        mPic.nTop    = pPicture->nTopOffset;
        mPic.nBottom = pPicture->nBottomOffset;
        mPic.nLeft   = pPicture->nLeftOffset;
        mPic.nRight  = pPicture->nRightOffset;
        CdcMemFlushCache(p->memops, pPicture->pData0, mPic.nWidth*mPic.nHeight*3/2);
        CdcSinkWritePic(p->pPicSink, &mPic);
    }
}

VideoPicture* RequestPicture(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;
    VideoPicture*        pPicture = NULL;

    logi("RequestPicture, nStreamIndex=%d", nStreamIndex);

    p = (VideoDecoderContext*)pDecoder;
    if(p->pATM != NULL)
    {
        ATMRequstPicture(p->pATM, &pPicture);
        return pPicture;
    }

    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        //* get FBM handle from video engine.
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);

        if(pFbm == NULL)
        {
            logi("Fbm module of video stream %d not create yet, \
                RequestPicture fail.", nStreamIndex);
            return NULL;
        }
    }

    //* request picture.
    pPicture = FbmRequestPicture(pFbm);

    if(pPicture != NULL)
    {
        if(pPicture->colour_primaries == 9)
        {
            pPicture->matrix_coeffs = VIDEO_MATRIX_COEFFS_BT2020;
        }

        #if(CHECK_METADATA_INFO)
        printfMetaDataInfo((struct sunxi_metadata*)pPicture->pMetaData);
        #endif

        logv("**** color_primaries = %d, matrix_coeffs = %d",
             pPicture->colour_primaries,pPicture->matrix_coeffs);
        //* set stream index to the picture, it will be useful when picture returned.
        pPicture->nStreamIndex = nStreamIndex;

        //* update video stream information.
        UpdateVideoStreamInfo(p, pPicture);
        dy_log("picture w: %d,  h: %d, addr: %p, top: %d, \
                bottom: %d, left: %d, right: %d, stride: %d", \
                pPicture->nWidth, pPicture->nHeight, pPicture,
                pPicture->nTopOffset, pPicture->nBottomOffset,
                pPicture->nLeftOffset, pPicture->nRightOffset, pPicture->nLineStride);

        DebugSavePicture(p, pPicture);

    }

    return pPicture;
}

int ReturnPicture(VideoDecoder* pDecoder, VideoPicture* pPicture)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;
    int                  nStreamIndex;
    int                  ret;

    logi("ReturnPicture, pPicture=%p", pPicture);

    p = (VideoDecoderContext*)pDecoder;

    if(pPicture == NULL)
        return 0;
    if(p->pATM != NULL)
    {
        ATMReturnPicture(p->pATM, pPicture);
        return 0;
    }
    nStreamIndex = pPicture->nStreamIndex;
    if(nStreamIndex < 0 || nStreamIndex > 2)
    {
        loge("invalid stream index(%d), pPicture->nStreamIndex must had been \
                changed by someone incorrectly, ReturnPicture fail.", nStreamIndex);
        return -1;
    }

    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        logw("pFbm is NULL when returning a picture, ReturnPicture fail.");
        return -1;
    }

    ret = FbmReturnPicture(pFbm, pPicture);
    if(ret != 0)
    {
        logw("FbmReturnPicture return fail,\
            it means the picture being returned it not one of this FBM.");
    }

    return ret;
}

VideoPicture* NextPictureInfo(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;
    VideoPicture*        pPicture;

    logi("RequestPicture, nStreamIndex=%d", nStreamIndex);

    p = (VideoDecoderContext*)pDecoder;

    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        //* get FBM handle from video engine.
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);

        logi("Fbm module of video stream %d not create yet, NextPictureInfo() fail.", nStreamIndex);
        return NULL;
    }

    //* request picture.
    pPicture = FbmNextPictureInfo(pFbm);

    if(pPicture != NULL)
    {
        //* set stream index to the picture, it will be useful when picture returned.
        pPicture->nStreamIndex = nStreamIndex;

        //* update video stream information.
        UpdateVideoStreamInfo(p, pPicture);
    }

    return pPicture;
}

int TotalPictureBufferNum(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;

    logv("TotalPictureBufferNum, nStreamIndex=%d", nStreamIndex);

    p    = (VideoDecoderContext*)pDecoder;
    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        //* get FBM handle from video engine.
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);

        logi("Fbm module of video stream %d not create yet, \
            TotalPictureBufferNum() fail.", nStreamIndex);
        return 0;
    }

    return FbmTotalBufferNum(pFbm);
}

int EmptyPictureBufferNum(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;

    logv("EmptyPictureBufferNum, nStreamIndex=%d", nStreamIndex);

    p    = (VideoDecoderContext*)pDecoder;
    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        //* get FBM handle from video engine.
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);

        logi("Fbm module of video stream %d not create yet, \
            EmptyPictureBufferNum() fail.", nStreamIndex);
        return 0;
    }

    return FbmEmptyBufferNum(pFbm);
}

int ValidPictureNum(VideoDecoder* pDecoder, int nStreamIndex)
{
    VideoDecoderContext* p;
    Fbm*                 pFbm;

//    logv("ValidPictureNum, nStreamIndex=%d", nStreamIndex);

    p    = (VideoDecoderContext*)pDecoder;
    pFbm = p->pFbm[nStreamIndex];
    if(pFbm == NULL)
    {
        //* get FBM handle from video engine.
        if(p->pVideoEngine != NULL)
            pFbm = p->pFbm[nStreamIndex] = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);

        logi("Fbm module of video stream %d not create yet, ValidPictureNum() fail.", nStreamIndex);
        if(pFbm == NULL)
        {
            return 0;
        }
    }

    return FbmValidPictureNum(pFbm);
}

//***********************************************//
//***********************************************//
//***added by xyliu at 2015-12-23****************//
/*if you want to scale the output video frame, but you donot know the real size of video,
 cannot set the scale params (nHorizonScaleDownRatio,nVerticalScaleDownRatio,
 bRotationEn), so you can set value to ExtraVidScaleInfo
*/
int ConfigExtraScaleInfo(VideoDecoder* pDecoder, int nWidthTh,
            int nHeightTh, int nHorizonScaleRatio, int nVerticalScaleRatio)
{
    VideoDecoderContext* p;
    int ret;
    DecoderInterface *pDecoderInterface;
    p = (VideoDecoderContext*)pDecoder;

    logd("*************config ConfigExtraScaleInfo\n");

    if(p == NULL ||p->pVideoEngine == NULL ||
            p->pVideoEngine->pDecoderInterface == NULL)
    {
        loge(" set decoder extra scale info fail ");
        return VDECODE_RESULT_UNSUPPORTED;
    }
    pDecoderInterface = p->pVideoEngine->pDecoderInterface;
    ret = 0;
    if(pDecoderInterface->SetExtraScaleInfo != NULL)
    {
        ret = pDecoderInterface->SetExtraScaleInfo(pDecoderInterface, \
                    nWidthTh, nHeightTh,nHorizonScaleRatio, nVerticalScaleRatio);
    }
    return ret;
}

int ConfigRotateInfo(VideoDecoder* pDecoder, int nDegree)
{
    VideoDecoderContext* p;
    int ret;
    DecoderInterface *pDecoderInterface;
    p = (VideoDecoderContext*)pDecoder;

    logd("*************config ConfigDegreeInfo\n");

    if(p == NULL ||p->pVideoEngine == NULL ||
            p->pVideoEngine->pDecoderInterface == NULL)
    {
        loge(" set decoder rotate info fail ");
        return VDECODE_RESULT_UNSUPPORTED;
    }
    pDecoderInterface = p->pVideoEngine->pDecoderInterface;
    ret = 0;
    if(pDecoderInterface->SetRotateInfo != NULL)
    {
        ret = pDecoderInterface->SetRotateInfo(pDecoderInterface, nDegree);
    }
    return ret;
}

int ReopenVideoEngine(VideoDecoder* pDecoder, VConfig* pVConfig, VideoStreamInfo* pStreamInfo)
{
    int                  i;
    VideoDecoderContext* p;
    int                  ret = 0;

    logv("ReopenVideoEngine");

    p = (VideoDecoderContext*)pDecoder;
    if(p->pVideoEngine == NULL)
    {
        logw("video decoder is not initialized yet, ReopenVideoEngine() fail.");
        return -1;
    }
    //* create a video engine again,
    //* before that, we should set vconfig and videoStreamInfo again.
    if(p->videoStreamInfo.pCodecSpecificData != NULL)
    {
        free(p->videoStreamInfo.pCodecSpecificData);
        p->videoStreamInfo.pCodecSpecificData = NULL;
    }

    ret = VideoEngineReopen(p->pVideoEngine,pVConfig, pStreamInfo);
    if(ret < 0)
    {
        loge("VideoEngineReopen result is %d\n", ret);
        return -1;
    }

    p->pFbm[0] = p->pFbm[1] = NULL;
    p->nFbmNum = 0;

    memcpy(&p->vconfig,pVConfig,sizeof(VConfig));
    memcpy(&p->videoStreamInfo,pStreamInfo,sizeof(VideoStreamInfo));
    pStreamInfo->bReOpenEngine = 1;
    p->vconfig.pVeOpsSelf = p->pVideoEngine->pVeOpsSelf;
    p->vconfig.veOpsS = p->pVideoEngine->veOpsS;

    if(pStreamInfo->pCodecSpecificData)
    {
        p->videoStreamInfo.pCodecSpecificData = (char*)malloc(pStreamInfo->nCodecSpecificDataLen);
        if(p->videoStreamInfo.pCodecSpecificData == NULL)
        {
            loge("malloc video codec specific data fail!");
            return -1;
        }
        memcpy(p->videoStreamInfo.pCodecSpecificData,
               pStreamInfo->pCodecSpecificData,
               pStreamInfo->nCodecSpecificDataLen);
        p->videoStreamInfo.nCodecSpecificDataLen = pStreamInfo->nCodecSpecificDataLen;
    }
    else
    {
       p->videoStreamInfo.pCodecSpecificData    = NULL;
       p->videoStreamInfo.nCodecSpecificDataLen = 0;
    }

    //* set sbm module to the video engine.
    for(i=0; i<p->nSbmNum; i++)
        VideoEngineSetSbm(p->pVideoEngine, p->pSbm[i], i);

    return 0;
}

int RotatePictureHw(VideoDecoder* pDecoder,
                    VideoPicture* pPictureIn,
                    VideoPicture* pPictureOut,
                    int           nRotateDegree)
{
    VideoDecoderContext* p;
    p = (VideoDecoderContext*)pDecoder;

    return VideoEngineRotatePicture(p->pVideoEngine, pPictureIn, pPictureOut, nRotateDegree);
}

int RotatePicture(struct ScMemOpsS* memOps,
                  VideoPicture* pPictureIn,
                  VideoPicture* pPictureOut,
                  int           nRotateDegree,
                  int           nGpuYAlign,
                  int           nGpuCAlign)
{
    VideoPicture* pOrgPicture = NULL;
    VideoPicture* pPictureMid = NULL;
    int bMallocBuffer = 0;
    int nMemSizeY = 0;
    int nMemSizeC = 0;
    int ret = 0;

    if(pPictureOut == NULL)
    {
        return -1;
    }
    GetBufferSize(pPictureIn->ePixelFormat,
        pPictureIn->nWidth, pPictureIn->nHeight,
        &nMemSizeY, &nMemSizeC, NULL, NULL, 0);
    CdcMemFlushCache(memOps, (void*)pPictureIn->pData0, nMemSizeY+2*nMemSizeC);
    pPictureOut->nPts = pPictureIn->nPts;
    pOrgPicture = pPictureIn;

    if(pPictureOut->ePixelFormat != pPictureIn->ePixelFormat)
    {
        if(nRotateDegree == 0)
        {
            ConvertPixelFormat(pPictureIn, pPictureOut);
            return 0;
        }
        else
        {
            pPictureMid = malloc(sizeof(VideoPicture));
            pPictureMid->ePixelFormat =  pPictureOut->ePixelFormat;
            pPictureMid->nWidth =  pPictureIn->nWidth;
            pPictureMid->nHeight =  pPictureIn->nHeight;
            if((pPictureMid->ePixelFormat==PIXEL_FORMAT_YV12)
               || (pPictureMid->ePixelFormat==PIXEL_FORMAT_NV21))
            {

                int nHeight16Align = (pPictureIn->nHeight+15) &~ 15;
                int nMemSizeY = pPictureIn->nLineStride*nHeight16Align;
                int nMemSizeC = nMemSizeY>>2;
                pPictureMid->pData0 = malloc(nMemSizeY+nMemSizeC*2);
                if(pPictureMid->pData0 == NULL)
                {
                    logd("malloc memory for pPictureMid failed\n");
                }
                ConvertPixelFormat(pPictureIn, pPictureMid);
                pOrgPicture = pPictureMid;
                bMallocBuffer = 1;
            }
        }
    }

    if(nRotateDegree == 0)
    {
        ret = RotatePicture0Degree(pOrgPicture, pPictureOut,nGpuYAlign,nGpuCAlign);
    }
    else if(nRotateDegree == 90)
    {
        ret = RotatePicture90Degree(pOrgPicture, pPictureOut);
    }
    else if(nRotateDegree == 180)
    {
        ret = RotatePicture180Degree(pOrgPicture, pPictureOut);
    }
    else if(nRotateDegree == 270)
    {
        ret = RotatePicture270Degree(pOrgPicture, pPictureOut);
    }
    if(pPictureMid != NULL)
    {
        if(bMallocBuffer)
            free(pPictureMid->pData0);
        free(pPictureMid);
    }
    return ret;
}

#if 0  //remove the function for iommu mode
VideoPicture* AllocatePictureBuffer(struct ScMemOpsS* memOps,
            int nWidth, int nHeight, int nLineStride, int ePixelFormat)
{
    VideoPicture* p;

    p = (VideoPicture*)malloc(sizeof(VideoPicture));

    logi("AllocatePictureBuffer, nWidth=%d, nHeight=%d, nLineStride=%d, ePixelFormat=%s",
            nWidth, nHeight, nLineStride, strPixelFormat[ePixelFormat]);

    if(p == NULL)
    {
        logw("memory alloc fail, allocate %d bytes in AllocatePictureBuffer()", \
                (int)sizeof(VideoPicture));
        return NULL;
    }

    memset(p, 0, sizeof(VideoPicture));
    p->nWidth       = nWidth;
    p->nHeight      = nHeight;
    p->nLineStride  = nLineStride;
    p->ePixelFormat = ePixelFormat;
    int nAlignValue = 0;

    if(FbmAllocatePictureBuffer(memOps, p, &nAlignValue, nWidth, nHeight) != 0)
    {
        logw("memory alloc fail, FbmAllocatePictureBuffer() fail in AllocatePictureBuffer().");
        free(p);
        p = NULL;
    }

    return p;
}

int FreePictureBuffer(struct ScMemOpsS* memOps, VideoPicture* pPicture)
{
    //VideoDecoderContext* p;

    logi("FreePictureBuffer, pPicture=%p", pPicture);

    if(pPicture == NULL)
    {
        loge("invaid picture, FreePictureBuffer fail.");
        return -1;
    }

    FbmFreePictureBuffer(memOps, pPicture);
    free(pPicture);
    return 0;
}
#endif

#define BUFFER_SIZE_480P   340000  //(720x480)
#define BUFFER_SIZE_720P   770000  //(1080x720)
#define BUFFER_SIZE_1080P  2000000 //(1920x1080)
#define BUFFER_SIZE_2K     4500000 //(2560x1920)
#define BUFFER_SIZE_4K     7000000 //(3840x2160)

static int DecideStreamBufferSize(VideoDecoderContext* p)
{
    int nPixels;
    int eCodecFormat;
    int nBufferSize = 8*1024*1024;

    //* we decide stream buffer size by resolution and codec format.
    nPixels = p->videoStreamInfo.nWidth * p->videoStreamInfo.nHeight;
    eCodecFormat = p->videoStreamInfo.eCodecFormat;

    //* for skia create sbm
    if((p->vconfig.eOutputPixelFormat == PIXEL_FORMAT_RGBA)
        && (eCodecFormat == VIDEO_CODEC_FORMAT_MJPEG)
        && p->vconfig.nVbvBufferSize)
    {
        nBufferSize = p->vconfig.nVbvBufferSize;
        logd("nBufferSize=%d", nBufferSize);
        return nBufferSize;
    }

    //* if resolution is unknown, treat it as full HD source.
    if(nPixels == 0)
        nPixels = BUFFER_SIZE_1080P;

    if(nPixels < BUFFER_SIZE_480P)
        nBufferSize = 2*1024*1024;
    else if (nPixels < BUFFER_SIZE_720P)
        nBufferSize = 4*1024*1024;
    else if(nPixels < BUFFER_SIZE_1080P)
        nBufferSize = 6*1024*1024;
    else if(nPixels < BUFFER_SIZE_2K)
        nBufferSize = 8*1024*1024;
    else if(nPixels < BUFFER_SIZE_4K)
        nBufferSize = 16*1024*1024;
    else
        nBufferSize = 32*1024*1024;

    if(eCodecFormat == VIDEO_CODEC_FORMAT_MJPEG ||
       eCodecFormat == VIDEO_CODEC_FORMAT_MPEG1 ||
       eCodecFormat == VIDEO_CODEC_FORMAT_MPEG2)
    {
        nBufferSize += 2*1024*1024; //* for old codec format, compress rate is low.
    }
    if(eCodecFormat == VIDEO_CODEC_FORMAT_RX)
    {
        nBufferSize = 16*1024*1024;
    }

    if(nPixels >= BUFFER_SIZE_4K && nBufferSize<=32*1024*1024)
    {
        nBufferSize = 32*1024*1024;
    }

    //* decrease the size as the parser is simple on ipc/cdr-chip,such as V3 V5
#ifdef CONF_SIMPLE_PARSER_CASE
    if(nPixels >= BUFFER_SIZE_4K)
        nBufferSize = 12*1024*1024;
#endif
    if((p->vconfig.nVbvBufferSize>=1*1024*1024) && (p->vconfig.nVbvBufferSize<=32*1024*1024))
    {
        if(nBufferSize >=  p->vconfig.nVbvBufferSize)
            nBufferSize = p->vconfig.nVbvBufferSize;
    }
    logd("nBufferSize=%d, p->vconfig.nVbvBufferSize=%d\n", nBufferSize, p->vconfig.nVbvBufferSize);
    return nBufferSize;
}


static void UpdateVideoStreamInfo(VideoDecoderContext* p, VideoPicture* pPicture)
{
    //* update video resolution.
    if(p->vconfig.bRotationEn   == 0 ||
       p->vconfig.nRotateDegree == 0 ||
       p->vconfig.nRotateDegree == 3)
    {
        //* the picture was not rotated or rotated by 180 degree.
        if(p->vconfig.bScaleDownEn == 0)
        {
            //* the picture was not scaled.
            if(p->videoStreamInfo.nWidth  != pPicture->nWidth ||
               p->videoStreamInfo.nHeight != pPicture->nHeight)
            {
                p->videoStreamInfo.nWidth  = pPicture->nWidth;
                p->videoStreamInfo.nHeight = pPicture->nHeight;
            }
        }
        else
        {
            //* the picture was scaled.
            if(p->videoStreamInfo.nWidth  != \
                (pPicture->nWidth<<p->vconfig.nHorizonScaleDownRatio) ||
               p->videoStreamInfo.nHeight != \
               (pPicture->nHeight<<p->vconfig.nVerticalScaleDownRatio))
            {
                p->videoStreamInfo.nWidth  = \
                    pPicture->nWidth << p->vconfig.nHorizonScaleDownRatio;
                p->videoStreamInfo.nHeight = \
                    pPicture->nHeight << p->vconfig.nVerticalScaleDownRatio;
            }
        }
    }
    else
    {
        //* the picture was rotated by 90 or 270 degree.
        if(p->vconfig.bScaleDownEn == 0)
        {
            //* the picture was not scaled.
            if(p->videoStreamInfo.nWidth  != pPicture->nHeight ||
               p->videoStreamInfo.nHeight != pPicture->nWidth)
            {
                p->videoStreamInfo.nWidth  = pPicture->nHeight;
                p->videoStreamInfo.nHeight = pPicture->nWidth;
            }
        }
        else
        {
            //* the picture was scaled.
            if(p->videoStreamInfo.nWidth  != \
                (pPicture->nHeight<<p->vconfig.nHorizonScaleDownRatio) ||
               p->videoStreamInfo.nHeight != \
               (pPicture->nWidth<<p->vconfig.nVerticalScaleDownRatio))
            {
                p->videoStreamInfo.nWidth  = \
                    pPicture->nHeight << p->vconfig.nHorizonScaleDownRatio;
                p->videoStreamInfo.nHeight = \
                    pPicture->nWidth << p->vconfig.nVerticalScaleDownRatio;
            }
        }
    }

    //* update frame rate.
    if(p->videoStreamInfo.nFrameRate != pPicture->nFrameRate && pPicture->nFrameRate != 0)
        p->videoStreamInfo.nFrameRate = pPicture->nFrameRate;

    return;
}

//*****************************************************************************************//
//***added new interface function for new display structure********************************//
//******************************************************************************************//
FbmBufInfo* GetVideoFbmBufInfo(VideoDecoder* pDecoder)
{
     VideoDecoderContext* p;
     FbmBufInfo* pBufInfo;
     ATMParam param;
     p = (VideoDecoderContext*)pDecoder;

     if(p == NULL)
     {
         return NULL;
     }
     if(p->pVideoEngine != NULL)
     {
         int nStreamIndex = 0;
         if(VideoEngineGetFbm(p->pVideoEngine, nStreamIndex) == NULL)
         {
             return NULL;
         }
         pBufInfo = FbmGetVideoBufferInfo(&(p->pVideoEngine->fbmInfo));
         if(p->pATM != NULL && pBufInfo != NULL)
         {
               param.bInterAllocFlag = 0;
               param.nAlignSize = pBufInfo->nAlignValue;
               if(param.nAlignSize < 32)
               {
                    logw("align value must more than 32, so change it");
                    param.nAlignSize = 32;
                    pBufInfo->nAlignValue = 32;
               }
               param.nMaxFrameNum = 6;
               param.nPixelFormat = pBufInfo->ePixelFormat;
               param.nPicWidth = p->vconfig.nSDWidth;
               param.nPicHeight = p->vconfig.nSDHeight;
               param.pFbm = VideoEngineGetFbm(p->pVideoEngine, nStreamIndex);
               pBufInfo->nBufWidth = (param.nPicWidth+param.nAlignSize-1) &~ (param.nAlignSize-1);
               pBufInfo->nBufHeight = (param.nPicHeight+param.nAlignSize-1) &~ (param.nAlignSize-1);
               pBufInfo->nBufNum = 6;
               ATMSetParam(p->pATM, &param);
               ATMScaleDownPrepare(p->pATM);
               ATMCreateFrameQueue(p->pATM, param.nMaxFrameNum);
         }
         return pBufInfo;
     }
     return NULL;
}

VideoPicture* SetVideoFbmBufAddress(VideoDecoder* pDecoder,
                                        VideoPicture* pVideoPicture, int bForbidUseFlag)
{
    VideoDecoderContext* p;
    p = (VideoDecoderContext*)pDecoder;

    if(p == NULL)
    {
        return NULL;
    }

    if((CdcIonGetMemType() == MEMORY_IOMMU) && p->vconfig.bGpuBufValid)
    {

        struct user_iommu_param sUserIommuBuf;

//#ifdef __ANDROID__
        sUserIommuBuf.fd = pVideoPicture->nBufFd;
        if(p->pVideoEngine->bIsSoftwareDecoder == 0)
        {
            CdcVeGetIommuAddr(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf, &sUserIommuBuf);
        }
        pVideoPicture->phyYBufAddr = sUserIommuBuf.iommu_addr;
        pVideoPicture->phyYBufAddr -= CdcVeGetPhyOffset(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);
        pVideoPicture->phyCBufAddr = pVideoPicture->phyYBufAddr +
                                            (pVideoPicture->nHeight * pVideoPicture->nLineStride);
        if(pVideoPicture->nHeight * pVideoPicture->nLineStride % 1024 != 0)
        {
            loge("phyCBufAddr is not aligned to 1024");
        }
//#endif
    }
    if(p->pVideoEngine != NULL)
    {
        if(p->pATM != NULL) {
            ATMSetFrameAddr(p->pATM, pVideoPicture);
            return NULL;
        }
        else
        {
            return FbmSetFbmBufAddress(&(p->pVideoEngine->fbmInfo), pVideoPicture, bForbidUseFlag);
        }
    }
    return NULL;
}

int SetVideoFbmBufRelease(VideoDecoder* pDecoder)
{
    VideoDecoderContext* p;
    p = (VideoDecoderContext*)pDecoder;

    if(p == NULL)
    {
        return -1;
    }
    if(p->pVideoEngine != NULL)
    {
        return FbmSetNewDispRelease(&(p->pVideoEngine->fbmInfo));
    }
    return -1;
}

VideoPicture* RequestReleasePicture(VideoDecoder* pDecoder)
{
    VideoDecoderContext* p;
    p = (VideoDecoderContext*)pDecoder;
    VideoPicture* pVpicture = NULL;

    if(p == NULL)
    {
        return NULL;
    }
    if(p->pVideoEngine != NULL)
    {
         //return FbmRequestReleasePicture(&(p->pVideoEngine->fbmInfo));
        pVpicture = FbmRequestReleasePicture(&(p->pVideoEngine->fbmInfo));
        if((CdcIonGetMemType()==MEMORY_IOMMU)&&(pVpicture!=NULL))
        {
            struct user_iommu_param sUserIommuBuf;
            sUserIommuBuf.fd = pVpicture->nBufFd;
            VideoFbmInfo* pFbmInfo = (VideoFbmInfo*)(p->pFbm[0]->pFbmInfo);
            if(pFbmInfo->bIsSoftDecoderFlag==0)
            {
                CdcVeFreeIommuAddr(p->pVideoEngine->veOpsS,
                                   p->pVideoEngine->pVeOpsSelf, &sUserIommuBuf);
            }
        }
        return pVpicture;
    }
    return NULL;
}

VideoPicture*  ReturnReleasePicture(VideoDecoder* pDecoder, \
        VideoPicture* pVpicture, int bForbidUseFlag)
{
    VideoDecoderContext* p;
    p = (VideoDecoderContext*)pDecoder;

    if(p == NULL)
    {
        return NULL;
    }
    if(p->pVideoEngine != NULL)
    {
        if((CdcIonGetMemType()==MEMORY_IOMMU)&& (p->vconfig.bGpuBufValid))
        {
            struct user_iommu_param sUserIommuBuf;
            sUserIommuBuf.fd = pVpicture->nBufFd;
            if(p->pVideoEngine->bIsSoftwareDecoder == 0)
            {
                CdcVeGetIommuAddr(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf, &sUserIommuBuf);
            }
            pVpicture->phyYBufAddr = sUserIommuBuf.iommu_addr;
            pVpicture->phyYBufAddr -= CdcVeGetPhyOffset(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);
            pVpicture->phyCBufAddr = pVpicture->phyYBufAddr +
                                     (pVpicture->nHeight * pVpicture->nLineStride);
        }
        return FbmReturnReleasePicture(&(p->pVideoEngine->fbmInfo), pVpicture, bForbidUseFlag);
    }
    return NULL;
}

int SetDecodePerformCmd(VideoDecoder* pDecoder, enum EVDECODERSETPERFORMCMD performCmd)
{
     VideoDecoderContext* p;
     int ret;
     DecoderInterface *pDecoderInterface;

     p = (VideoDecoderContext*)pDecoder;

     if(p == NULL ||p->pVideoEngine == NULL ||
          p->pVideoEngine->pDecoderInterface == NULL)
     {
            loge("SetDecodeDebugCmd fail\n");
            return VDECODE_RESULT_UNSUPPORTED;
     }
    //eVersion = VeGetIcVersion();
     pDecoderInterface = p->pVideoEngine->pDecoderInterface;
     ret = 0;
     if(pDecoderInterface->SetExtraScaleInfo != NULL)
     {
         ret = pDecoderInterface->SetPerformCmd(pDecoderInterface,performCmd);
     }
     return ret;
}

int GetDecodePerformInfo(VideoDecoder* pDecoder,
        enum EVDECODERGETPERFORMCMD performCmd, VDecodePerformaceInfo** performInfo)
{
     VideoDecoderContext* p;
     int ret;
     DecoderInterface *pDecoderInterface;
     p = (VideoDecoderContext*)pDecoder;

     if(p == NULL ||p->pVideoEngine == NULL ||
          p->pVideoEngine->pDecoderInterface == NULL)
     {
            loge("GetDecodePerformInfo fail\n");
            return VDECODE_RESULT_UNSUPPORTED;
     }
     pDecoderInterface = p->pVideoEngine->pDecoderInterface;
     ret = 0;
     if(pDecoderInterface->SetExtraScaleInfo != NULL)
     {
         ret = pDecoderInterface->GetPerformInfo(pDecoderInterface,performCmd, performInfo);
     }
     return ret;
}

void *VideoDecoderPallocIonBuf(VideoDecoder* pDecoder, int nSize)
{
    VideoDecoderContext* p= (VideoDecoderContext *)pDecoder;
    return CdcMemPalloc(p->vconfig.memops, nSize, (void *)p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);
}
void VideoDecoderFreeIonBuf(VideoDecoder* pDecoder, void *pIonBuf)
{
    VideoDecoderContext* p= (VideoDecoderContext *)pDecoder;
    CdcMemPfree(p->vconfig.memops, pIonBuf, (void *)p->vconfig.veOpsS, p->vconfig.pVeOpsSelf);
    return;
}

int VideoDecoderSetFreq(VideoDecoder* pDecoder, int nVeFreq)
{
    VideoDecoderContext* p = (VideoDecoderContext*)pDecoder;
    int ret;
    ret = CdcVeSetSpeed(p->vconfig.veOpsS, p->vconfig.pVeOpsSelf, nVeFreq);
    if(ret < 0)
        loge("VideoDecoderSetFreq %d error, ret is %d\n", nVeFreq, ret);

    return ret;
}

int CheckSbmDataFirstLastFlag(VideoDecoder*        pDecoder,
                          VideoStreamDataInfo* pDataInfo,
                          int                  nStreamBufIndex)
{
    SbmInterface*                 pSbm;
    VideoDecoderContext* p;
    struct ScMemOpsS *_memops;

    int checkLen = 512;
    char  dataBuf[512];
    char* pSbmBufferStartAddr;
    int   nSbmBufferSize;
    char* pSbmBufferEndAddr;
    char* pData = NULL;
    u16 startCode = 0;
    int remainDataLen = 0;
    //int length = 0;
    int i = 0;
    u16 nextStartCode = 0;

    p = (VideoDecoderContext*)pDecoder;

    _memops = p->memops;

    pSbm = p->pSbm[nStreamBufIndex];

    if(pSbm == NULL)
    {
        logw("pSbm of video stream %d is NULL, SubmitVideoStreamData fail.", nStreamBufIndex);
        return -1;
    }

    pSbmBufferStartAddr = SbmBufferAddress(pSbm);
    nSbmBufferSize      = SbmBufferSize(pSbm);
    pSbmBufferEndAddr   = pSbmBufferStartAddr + nSbmBufferSize - 1;
    pDataInfo->bIsFirstPart = 0;
    pDataInfo->bIsLastPart = 0;

    if(pDataInfo->nLength <= checkLen)
    {
        checkLen = pDataInfo->nLength;
    }
    pData = pDataInfo->pData;
    if((pDataInfo->pData+checkLen) > pSbmBufferEndAddr)
    {
        remainDataLen = pSbmBufferEndAddr+1-pDataInfo->pData;
        memcpy(dataBuf,pDataInfo->pData, remainDataLen);
        memcpy(dataBuf+remainDataLen,pSbmBufferStartAddr, checkLen-remainDataLen);
        pData = dataBuf;
    }

    startCode = 0;

    for(i=0; i<checkLen; i++)
    {
        startCode <<= 8;
        startCode |= pData[i];
        if(startCode == 0xFFD8)  //SOI
        {
            pDataInfo->bIsFirstPart = 1;
            break;
        }
        else if((startCode!=0) && (startCode!=0xFFFF)&&(startCode!=0x00FF))
        {
            break;
        }
    }

   pData = pDataInfo->pData+pDataInfo->nLength-checkLen;
   if((pDataInfo->pData+pDataInfo->nLength) > pSbmBufferEndAddr)
   {
        remainDataLen = pSbmBufferEndAddr+1-pDataInfo->pData;
        if((pDataInfo->nLength-remainDataLen) >= checkLen)
        {
            pData = pDataInfo->pData+pDataInfo->nLength-nSbmBufferSize-checkLen;
        }
        else
        {
            int length = (checkLen+remainDataLen- pDataInfo->nLength);
            memcpy(dataBuf,pSbmBufferEndAddr-length+1,length);
            memcpy(dataBuf+length, pDataInfo->pData, pDataInfo->nLength-remainDataLen);
            pData = dataBuf;
        }
    }

    startCode = 0;
    for(i=checkLen-1; i>=0; i--)
    {
        startCode >>= 8;
        startCode |= (pData[i]<<8);
        if(startCode == 0xFFD9)  //EOI
        {
            pDataInfo->bIsLastPart = 1;
            break;
        }
        else if((startCode!=0) && (startCode!=0xFFFF)&&(startCode!=0x00FF))
        {
            if(i>0)
            {
                nextStartCode = (pData[i-1]<<8) | (pData[i]);
                if(nextStartCode == 0xFFD9)
                {
                    pDataInfo->bIsLastPart = 1;
                    break;
                }
            }
        }
    }
    return 0;

}

#if 0
//*just for remove warning of cppcheck
static void vdecoderUseFunction()
{
    CEDARC_UNUSE(vdecoderUseFunction);
    CEDARC_UNUSE(CreateVideoDecoder);
    CEDARC_UNUSE(DestroyVideoDecoder);
    CEDARC_UNUSE(InitializeVideoDecoder);
    CEDARC_UNUSE(ResetVideoDecoder);
    CEDARC_UNUSE(DecodeVideoStream);
    CEDARC_UNUSE(DecoderSetSpecialData);
    CEDARC_UNUSE(GetVideoStreamInfo);
    CEDARC_UNUSE(RequestVideoStreamBuffer);
    CEDARC_UNUSE(SubmitVideoStreamData);
    CEDARC_UNUSE(VideoStreamBufferSize);
    CEDARC_UNUSE(VideoStreamDataSize);
    CEDARC_UNUSE(VideoStreamFrameNum);
    CEDARC_UNUSE(VideoStreamFrameNum);
    CEDARC_UNUSE(RequestPicture);
    CEDARC_UNUSE(ReturnPicture);
    CEDARC_UNUSE(NextPictureInfo);
    CEDARC_UNUSE(TotalPictureBufferNum);
    CEDARC_UNUSE(EmptyPictureBufferNum);
    CEDARC_UNUSE(ValidPictureNum);
    CEDARC_UNUSE(ReopenVideoEngine);
    CEDARC_UNUSE(RotatePicture);
    CEDARC_UNUSE(RotatePictureHw);
    CEDARC_UNUSE(AllocatePictureBuffer);
    CEDARC_UNUSE(FreePictureBuffer);
    CEDARC_UNUSE(ReturnRelasePicture);
    CEDARC_UNUSE(RequestReleasePicture);
    CEDARC_UNUSE(SetVideoFbmBufRelease);
    CEDARC_UNUSE(SetVideoFbmBufAddress);
    CEDARC_UNUSE(GetVideoFbmBufInfo);
    CEDARC_UNUSE(ConfigExtraScaleInfo);
    CEDARC_UNUSE(SetDecodePerformCmd);
    CEDARC_UNUSE(GetDecodePerformInfo);
    CEDARC_UNUSE(VideoStreamDataInfoPointer);
    CEDARC_UNUSE(VideoDecoderPallocIonBuf);
    CEDARC_UNUSE(VideoDecoderFreeIonBuf);
    CEDARC_UNUSE(CheckSbmDataFirstLastFlag);
}
#endif
