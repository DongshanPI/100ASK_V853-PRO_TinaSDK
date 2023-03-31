
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : fbm.c
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

//#define CONFIG_LOG_LEVEL    OPTION_LOG_LEVEL_DETAIL
#define LOG_TAG "fbm.c"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include "fbm.h"
#include "cdc_log.h"

#include "CdcIniparserapi.h"
#include "CdcIonUtil.h"
//* should include other  '*.h'  before CdcMalloc.h
#include "CdcMalloc.h"

static int bDynamicShowLogFlag;
extern const char* strPixelFormat[];
unsigned int FbmComputeAfbcBufferSize(VideoPicture* pPicture, int b10BitPicFlag);
extern int GetBufferSize(int ePixelFormat, int nWidth, int nHeight,int*nYBufferSize,
                          int *nCBufferSize, int* nYLineStride, int* nCLineStride, int nAlignValue);

#define FBM_SHOW_DETAIL_LOG (0)

#define FRM_FORBID_USE_TAG        0xff
#define FRM_RELEASE_TAG           0xee
#define FBM_NEW_DISP_INVALID_TAG  0xdd
#define FBM_NEED_FLUSH_CACHE_SIZE 480

#define MAJOR_DECODER_USE_FLAG 1
#define MINOR_DECODER_USE_FLAG 2
#define MAJOR_DISP_USE_FLAG    4
#define MINOR_DISP_USE_FLAG    8
#define MINOR_NEED_USE_FLAG    16

#define dy_log(fmt, arg...) do{ \
    if(pFbm->bShowLogFlag == 1) \
        logd(fmt, arg); \
    else \
        logi(fmt, arg); \
}while(0)

void FbmDebugPrintStatus(Fbm* pFbm)
{
    int i = 0;
    if(pFbm == NULL)
        return;
    for(i=0; i<pFbm->nMaxFrameNum; i++)
    {
        logd("i=%d, picture=%p, displed=%d, valid=%d, decode=%d, render=%d\n", \
            i, &pFbm->pFrames[i].vpicture, pFbm->pFrames[i].Flag.bAlreadyDisplayed, \
            pFbm->pFrames[i].Flag.bInValidPictureQueue,  pFbm->pFrames[i].Flag.bUsedByDecoder, \
            pFbm->pFrames[i].Flag.bUsedByRender);
    }
}

static void FbmEnqueue(FrameNode** ppHead, FrameNode* p)
{
    FrameNode* pCur;

    pCur = *ppHead;

    if(pCur == NULL)
    {
        *ppHead  = p;
        p->pNext = NULL;
        return;
    }
    else
    {
        while(pCur->pNext != NULL)
            pCur = pCur->pNext;

        pCur->pNext = p;
        p->pNext    = NULL;

        return;
    }
}

#if 0
static void FbmEnqueueToHead(FrameNode ** ppHead, FrameNode* p)
{
    CEDARC_UNUSE(FbmEnqueueToHead);

    FrameNode* pCur;

    pCur     = *ppHead;
    *ppHead  = p;
    p->pNext = pCur;
    return;
}
#endif

static FrameNode* FbmDequeue(FrameNode** ppHead)
{
    FrameNode* pHead;

    pHead = *ppHead;

    if(pHead == NULL)
        return NULL;
    else
    {
        *ppHead = pHead->pNext;
        pHead->pNext = NULL;
        return pHead;
    }
}

#if 0
static void debugCreateBuffer(FbmCreateInfo* pFbmCreateInfo, VideoFbmInfo* pFbmInfo)
{
    logv("FbmCreateBuffer CreateInfo:");
    logv("nFrameNum: %d",pFbmCreateInfo->nFrameNum);
    logv("nDecoderNeededMiniFrameNum: %d",pFbmCreateInfo->nDecoderNeededMiniFrameNum);
    logv("nWidth: %d",pFbmCreateInfo->nWidth);
    logv("nHeight: %d",pFbmCreateInfo->nHeight);
    logv("ePixelFormat: %d",pFbmCreateInfo->ePixelFormat);
    logv("bThumbnailMode: %d",pFbmCreateInfo->bThumbnailMode);
    logv("bGpuBufValid: %d",pFbmCreateInfo->bGpuBufValid);
    logv("nAlignStride: %d",pFbmCreateInfo->nAlignStride);
    logv("nBufferType: %d",pFbmCreateInfo->nBufferType);
    logv("bProgressiveFlag: %d",pFbmCreateInfo->bProgressiveFlag);
    logv("bIsSoftDecoderFlag: %d",pFbmCreateInfo->bIsSoftDecoderFlag);
    logv("memops: %p",pFbmCreateInfo->memops);
    logv("b10BitStreamFlag: %d",pFbmCreateInfo->b10BitStreamFlag);
    logv("veOpsS: %p",pFbmCreateInfo->veOpsS);
    logv("pVeOpsSelf: %p",pFbmCreateInfo->pVeOpsSelf);

    logv("FbmCreateBuffer FbmInfo:");
    logv("nValidBufNum: %d",pFbmInfo->nValidBufNum);
    logv("pFbmFirst: %p",pFbmInfo->pFbmFirst);
    logv("pFbmSecond: %p",pFbmInfo->pFbmSecond);
    logv("pFbmBufInfo:");
    logv("    nBufNum : %d",pFbmInfo->pFbmBufInfo.nBufNum);
    logv("    nBufWidth : %d",pFbmInfo->pFbmBufInfo.nBufWidth);
    logv("    nBufHeight : %d",pFbmInfo->pFbmBufInfo.nBufHeight);
    logv("    ePixelFormat : %d",pFbmInfo->pFbmBufInfo.ePixelFormat);
    logv("    nAlignValue : %d",pFbmInfo->pFbmBufInfo.nAlignValue);
    logv("    bProgressiveFlag : %d",pFbmInfo->pFbmBufInfo.bProgressiveFlag);
    logv("    bIsSoftDecoderFlag : %d",pFbmInfo->pFbmBufInfo.bIsSoftDecoderFlag);
    logv("    bHdrVideoFlag : %d",pFbmInfo->pFbmBufInfo.bHdrVideoFlag);
    logv("    b10bitVideoFlag : %d",pFbmInfo->pFbmBufInfo.b10bitVideoFlag);
    logv("    bAfbcModeFlag : %d",pFbmInfo->pFbmBufInfo.bAfbcModeFlag);
    logv("    nTopOffset : %d",pFbmInfo->pFbmBufInfo.nTopOffset);
    logv("    nBottomOffset : %d",pFbmInfo->pFbmBufInfo.nBottomOffset);
    logv("    nLeftOffset : %d",pFbmInfo->pFbmBufInfo.nLeftOffset);
    logv("    nRightOffset : %d",pFbmInfo->pFbmBufInfo.nRightOffset);
    logv("bIs3DStream: %d",pFbmInfo->bIs3DStream);
    logv("bTwoStreamShareOneFbm: %d",pFbmInfo->bTwoStreamShareOneFbm);
    logv("pMajorDispFrame: %p",pFbmInfo->pMajorDispFrame);
    logv("pMajorDecoderFrame: %p",pFbmInfo->pMajorDecoderFrame);
    logv("nMinorYBufOffset: %d",pFbmInfo->nMinorYBufOffset);
    logv("nMinorCBufOffset: %d",pFbmInfo->nMinorCBufOffset);
    logv("bIsFrameCtsTestFlag: %d",pFbmInfo->bIsFrameCtsTestFlag);
    logv("nExtraFbmBufferNum: %d",pFbmInfo->nExtraFbmBufferNum);
    logv("nDecoderNeededMiniFbmNum: %d",pFbmInfo->nDecoderNeededMiniFbmNum);
    logv("nDecoderNeededMiniFbmNumSD: %d",pFbmInfo->nDecoderNeededMiniFbmNumSD);
    logv("bIsSoftDecoderFlag: %d",pFbmInfo->bIsSoftDecoderFlag);
}
#endif

Fbm* FbmCreateBuffer(FbmCreateInfo* pFbmCreateInfo, VideoFbmInfo* pFbmInfo)
{
    Fbm*       p;
    int        i;
    //int        nAllocFrameNum;
    FrameNode* pFrameNode;

    int nWidth           = pFbmCreateInfo->nWidth;
    int nHeight          = pFbmCreateInfo->nHeight;
    int nAlignStride     = pFbmCreateInfo->nAlignStride;

    if(nWidth >= 3840 && nHeight >= 2160)
    {
        if(pFbmCreateInfo->nFrameNum > 20)
        {
            pFbmCreateInfo->nFrameNum = 20;
            logw("the nFrameNum excess 20,decrease to 20");
        }
    }

    logd("FbmCreate, total fbm number: %d, decoder needed: %d,  \
nWidth=%d, nHeight=%d, nAlignStride = %d",
           pFbmCreateInfo->nFrameNum,
           pFbmCreateInfo->nDecoderNeededMiniFrameNum,
           nWidth, nHeight, nAlignStride);
    if(nWidth >= 7680 || nHeight >= 4320)
    {
        loge("width or height too large , can not create fbm buffer.");
        return NULL;
    }

    p = (Fbm*)malloc(sizeof(Fbm));
    if(p == NULL)
    {
        loge("memory alloc fail.");
        return NULL;
    }
    memset(p, 0, sizeof(Fbm));

#if FBM_SHOW_DETAIL_LOG
    p->bShowLogFlag = 1;
#endif
    if(p->bShowLogFlag == 0)
       p->bShowLogFlag = CdcGetConfigParamterInt("vdecoder_fbm_printf_log", 0);

    p->memops               = pFbmCreateInfo->memops;
    p->nMaxFrameNum         = pFbmCreateInfo->nFrameNum;
    p->sVeops               = pFbmCreateInfo->veOpsS;
    p->pVeOpsSelf           = pFbmCreateInfo->pVeOpsSelf;
    pFbmInfo->bIsSoftDecoderFlag = pFbmCreateInfo->bIsSoftDecoderFlag;

#if 0   //added by xyliu at 2015-02-12
    if((pFbmCreateInfo->bGpuBufValid==1) && (pFbmCreateInfo->nBufferType!=BUF_TYPE_ONLY_REFERENCE))
    {
         pFbmCreateInfo->nFrameNum -= 3; //
         p->nMaxFrameNum       = pFbmCreateInfo->nFrameNum;

         p->nMaxFrameNum += 5;
         if(pFbmCreateInfo->bProgressiveFlag == 0)
         {
             p->nMaxFrameNum += 2;
         }
    }
#endif

    //* allocate frame nodes.
    pFrameNode = (FrameNode*)malloc(p->nMaxFrameNum*sizeof(FrameNode));
    if(pFrameNode == NULL)
    {
        loge("memory alloc fail, alloc %d bytes.", (int)(p->nMaxFrameNum*sizeof(FrameNode)));
        free(p);
        return NULL;
    }
    memset(pFrameNode, 0, p->nMaxFrameNum*sizeof(FrameNode));

    pthread_mutex_init(&p->mutex, NULL);

    p->pEmptyBufferQueue  = NULL;
    p->pValidPictureQueue = NULL;
    p->bThumbnailMode     = pFbmCreateInfo->bThumbnailMode;
    p->pFrames            = pFrameNode;
    p->bUseGpuBuf         = 0;
    if((pFbmInfo->pFbmBufInfo.nRightOffset==0) || (pFbmInfo->pFbmBufInfo.nBottomOffset==0))
    {
        pFbmInfo->pFbmBufInfo.nRightOffset = nWidth;
        pFbmInfo->pFbmBufInfo.nBottomOffset = nHeight;
        pFbmInfo->pFbmBufInfo.nTopOffset = 0;
        pFbmInfo->pFbmBufInfo.nLeftOffset = 0;
    }

    for(i=0; i<p->nMaxFrameNum; i++)
    {
        memset((void*)(&pFrameNode->vpicture), 0, sizeof(VideoPicture));
        pFrameNode->Flag.bUsedByDecoder       = 0;
        pFrameNode->Flag.bUsedByRender        = 0;
        pFrameNode->Flag.bInValidPictureQueue = 0;
        pFrameNode->Flag.bAlreadyDisplayed    = 0;
        pFrameNode->pNext                = NULL;

        pFrameNode->vpicture.nBufId       = FBM_NEW_DISP_INVALID_TAG;
        pFrameNode->vpicture.nID          = i;
        pFrameNode->vpicture.ePixelFormat = pFbmCreateInfo->ePixelFormat;
        if((nAlignStride != 0) && (pFbmInfo->pFbmBufInfo.nLbcLossyComMod == 0))
        {
            pFrameNode->vpicture.nWidth       = \
                (nWidth+nAlignStride-1) &~ (nAlignStride-1);
            pFrameNode->vpicture.nHeight      = \
                (nHeight+nAlignStride-1) &~ (nAlignStride-1);
        }
        else
        {
            pFrameNode->vpicture.nWidth       = nWidth;
            pFrameNode->vpicture.nHeight      = nHeight;
        }
        pFrameNode->vpicture.nLineStride  = pFrameNode->vpicture.nWidth;
        pFrameNode->vpicture.bIsProgressive = pFbmCreateInfo->bProgressiveFlag;
        pFrameNode->vpicture.nLeftOffset   = 0;
        pFrameNode->vpicture.nTopOffset    = 0;
        pFrameNode->vpicture.nRightOffset  = nWidth;
        pFrameNode->vpicture.nBottomOffset = nHeight;
        pFrameNode->vpicture.b10BitPicFlag = pFbmCreateInfo->b10BitStreamFlag;
        pFrameNode->vpicture.bEnableAfbcFlag = pFbmInfo->pFbmBufInfo.bAfbcModeFlag;
        pFrameNode->vpicture.nLbcLossyComMod = pFbmInfo->pFbmBufInfo.nLbcLossyComMod;
        pFrameNode->vpicture.bIsLossy        = pFbmInfo->pFbmBufInfo.bIsLossy;
        pFrameNode->vpicture.bRcEn           = pFbmInfo->pFbmBufInfo.bRcEn;

        if(pFbmCreateInfo->bGpuBufValid==0 || \
            pFbmCreateInfo->nBufferType==BUF_TYPE_ONLY_REFERENCE)
        {
            if(!pFbmCreateInfo->bThumbnailMode || i==0)
            {
                logd("** call allocate pic buf, i = %d, maxNum = %d",i,p->nMaxFrameNum);
                if(FbmAllocatePictureBuffer(p, \
                    &pFrameNode->vpicture, &nAlignStride, nWidth, nHeight) != 0)
                    break;
            }
            else
            {
                //* thumbnail mode, other pictures share picture buffer with the first one.
                pFrameNode->vpicture.pData0 = p->pFrames[0].vpicture.pData0;
                pFrameNode->vpicture.pData1 = p->pFrames[0].vpicture.pData1;
                pFrameNode->vpicture.pData2 = p->pFrames[0].vpicture.pData2;
                pFrameNode->vpicture.pData3 = p->pFrames[0].vpicture.pData3;
                pFrameNode->vpicture.phyYBufAddr = p->pFrames[0].vpicture.phyYBufAddr;
                pFrameNode->vpicture.phyCBufAddr = p->pFrames[0].vpicture.phyCBufAddr;
            }
            pFrameNode->vpicture.nColorPrimary = 0xffffffff;
        }

        if(pFbmCreateInfo->bGpuBufValid==1)
        {
            int nLower2BitBufOffset = 0;
            int nLower2BitBufStride = 0;
            int nAfbcBufSize  = 0;
            int nAlignWidht   = pFrameNode->vpicture.nWidth;
            int nAlignHeight  = pFrameNode->vpicture.nHeight;
            if(pFrameNode->vpicture.b10BitPicFlag)
            {
                //int frmbuf_c_size = 0;
                //frmbuf_c_size = 2 * (PriChromaStride * (((nHeight/2)+15)&0xfffffff0)/4);
                //int PriChromaStride = ((nWidth/2) + 31)&0xffffffe0;
                nAfbcBufSize = ((nWidth+15)>>4) * ((nHeight+4+15)>>4) * (512 + 16) + 32 + 1024;
                nLower2BitBufStride = ((((nWidth+3)>>2) + 31) & 0xffffffe0);

                if(pFrameNode->vpicture.bEnableAfbcFlag == 1)
                {
                    nLower2BitBufOffset = nAfbcBufSize/* + frmbuf_c_size*/;
                    pFrameNode->vpicture.nAfbcSize = nAfbcBufSize;
                }
                else
                {
                    int nNormalYuvBufSize = nAlignWidht*nAlignHeight*3/2;
                    nLower2BitBufOffset = nNormalYuvBufSize;
                }
            }
            else
            {
                if(pFrameNode->vpicture.bEnableAfbcFlag == 1)
                {
                    nAfbcBufSize = ((nWidth+15)>>4) * ((nHeight+4+15)>>4) * (384 + 16) + 1024 + 32;
                    //nAfbcBufSize = ((nWidth+15)>>4) * ((nHeight+4+15)>>4) * (384 + 16) + 1024 ;
                    pFrameNode->vpicture.nAfbcSize = nAfbcBufSize;
                }
            }
            logd("*** calcute nLower2BitBufOffset = %d(%0.2f), stride = %d",
                 nLower2BitBufOffset, (float)nLower2BitBufOffset/1024/1024, nLower2BitBufStride);
            pFrameNode->vpicture.nLower2BitBufOffset = nLower2BitBufOffset;
            pFrameNode->vpicture.nLower2BitBufStride = nLower2BitBufStride;
        }
        pFrameNode++;
    }
    p->nAlignValue = nAlignStride;

    //* check whether all picture buffer allocated.
    if(i < p->nMaxFrameNum)
    {
        //* not all picture buffer allocated, abort the Fbm creating.
        int j;
        loge("memory alloc fail, only %d frames allocated, \
            we need %d frames.", i, pFbmCreateInfo->nFrameNum);
        for(j = 0; j < i; j++)
            FbmFreePictureBuffer(p, &p->pFrames[j].vpicture);
        pthread_mutex_destroy(&p->mutex);
        free(p->pFrames);
        free(p);
        return NULL;
    }

    if((pFbmCreateInfo->bGpuBufValid==1) && \
        (pFbmCreateInfo->nBufferType!=BUF_TYPE_ONLY_REFERENCE))
    {
        //****************************TO DO*******************************************//
        if((pFbmInfo->pFbmFirst != NULL)&&(pFbmInfo->bIs3DStream==1))
        {
            //pFbmInfo->pFbmSecond = (void*)p;
            p->bUseGpuBuf = 1;
            p->pFbmInfo = (void*)pFbmInfo;
        }
        else
        {
            pFbmInfo->pFbmBufInfo.nBufNum = pFbmCreateInfo->nFrameNum;
            if(pFbmCreateInfo->bThumbnailMode == 1)
            {
                pFbmInfo->pFbmBufInfo.nBufNum  = 1;
            }
            //* not align w and h on the afbc mode(gpu require)
            if(pFbmInfo->pFbmBufInfo.bAfbcModeFlag == 0 && nAlignStride != 0)
            {
                pFbmInfo->pFbmBufInfo.nBufWidth = \
                    (nWidth+nAlignStride-1) &~ (nAlignStride-1);
                pFbmInfo->pFbmBufInfo.nBufHeight = \
                    (nHeight+nAlignStride-1) &~ (nAlignStride-1);
            }
            else
            {
                pFbmInfo->pFbmBufInfo.nBufWidth = nWidth;
                pFbmInfo->pFbmBufInfo.nBufHeight = nHeight;
            }
            #ifdef CONFIG_VE_MULTIPLE_OUTPUT
            if(pFbmInfo->pFbmBufInfo.bAfbcModeFlag != 0)
            {
                //pFbmInfo->pFbmBufInfo.nBufWidth  = 	pFbmInfo->pFbmBufInfo.nBufWidth;
                pFbmInfo->pFbmBufInfo.nBufHeight =  (pFbmInfo->pFbmBufInfo.nBufHeight*3)/2;
            }
            #endif
            pFbmInfo->pFbmBufInfo.nAlignValue        = nAlignStride;
            pFbmInfo->pFbmBufInfo.ePixelFormat       = pFbmCreateInfo->ePixelFormat;
            pFbmInfo->pFbmBufInfo.bProgressiveFlag   = pFbmCreateInfo->bProgressiveFlag;
            pFbmInfo->pFbmBufInfo.bIsSoftDecoderFlag = pFbmCreateInfo->bIsSoftDecoderFlag;
            pFbmInfo->pFbmBufInfo.b10bitVideoFlag    = pFbmCreateInfo->b10BitStreamFlag;
            //pFbmInfo->pFbmFirst = (void*)p;
            p->pFbmInfo = (void*)pFbmInfo;
            p->bUseGpuBuf = 1;
        }
    }
    else
    {
        pFbmInfo->pFbmBufInfo.nBufNum = pFbmCreateInfo->nFrameNum;
        if(pFbmCreateInfo->bThumbnailMode == 1)
        {
            pFbmInfo->pFbmBufInfo.nBufNum  = 1;
        }
        //* not align w and h on the afbc mode(gpu require)
        if(pFbmInfo->pFbmBufInfo.bAfbcModeFlag == 0 && nAlignStride != 0)
        {
            pFbmInfo->pFbmBufInfo.nBufWidth = \
                (nWidth+nAlignStride-1) &~ (nAlignStride-1);
            pFbmInfo->pFbmBufInfo.nBufHeight = \
                (nHeight+nAlignStride-1) &~ (nAlignStride-1);
        }
        else
        {
            pFbmInfo->pFbmBufInfo.nBufWidth = nWidth;
            pFbmInfo->pFbmBufInfo.nBufHeight = nHeight;
        }
        pFbmInfo->pFbmBufInfo.nAlignValue        = nAlignStride;
        pFbmInfo->pFbmBufInfo.ePixelFormat       = pFbmCreateInfo->ePixelFormat;
        pFbmInfo->pFbmBufInfo.bProgressiveFlag   = pFbmCreateInfo->bProgressiveFlag;
        pFbmInfo->pFbmBufInfo.bIsSoftDecoderFlag = pFbmCreateInfo->bIsSoftDecoderFlag;
        pFbmInfo->pFbmBufInfo.b10bitVideoFlag    = pFbmCreateInfo->b10BitStreamFlag;
        //* put all frame to the empty frame queue.
        for(i=0; i<pFbmCreateInfo->nFrameNum; i++)
        {
            pFrameNode = &p->pFrames[i];
            FbmEnqueue(&p->pEmptyBufferQueue, pFrameNode);
        }
        p->nEmptyBufferNum = pFbmCreateInfo->nFrameNum;

        p->pFbmInfo = (void*)pFbmInfo;
    }
    logd("*** finish fbmCreateBuffer");
    return p;
}

Fbm* FbmCreate(FbmCreateInfo* pFbmCreateInfo, VideoFbmInfo* pFbmInfo)
{
    int nYBufferSize = 0;
    int nCBufSize = 0;
    int nYLineStride = 0;
    int nCLineStride = 0;
    //debugCreateBuffer(pFbmCreateInfo, pFbmInfo);
    if((pFbmInfo->bIs3DStream==1)&&(pFbmCreateInfo->bGpuBufValid==1))
    {
        pFbmInfo->bTwoStreamShareOneFbm = 1;
        if(pFbmInfo->pFbmFirst == NULL)
        {
            pFbmInfo->pFbmFirst = (void*)FbmCreateBuffer(pFbmCreateInfo, pFbmInfo);
            if(pFbmInfo->pFbmFirst != NULL)
            {
                pFbmInfo->pFbmSecond = (void*)FbmCreateBuffer(pFbmCreateInfo, pFbmInfo);
                if(pFbmInfo->pFbmSecond == NULL)
                {
                    loge("pFbmInfo->pFbmSecond=NULL\n");
                }
                GetBufferSize(pFbmInfo->pFbmBufInfo.ePixelFormat, \
                        pFbmInfo->pFbmBufInfo.nBufWidth, pFbmInfo->pFbmBufInfo.nBufHeight, \
                        &nYBufferSize, &nCBufSize, &nYLineStride, &nCLineStride, \
                        pFbmInfo->pFbmBufInfo.nAlignValue);
                pFbmInfo->nMinorYBufOffset = nYBufferSize;
                pFbmInfo->nMinorCBufOffset = 2*nCBufSize;
            }
            //debugCreateBuffer(pFbmCreateInfo, pFbmInfo);
            return (Fbm*)pFbmInfo->pFbmFirst;
        }
        else
        {
            //debugCreateBuffer(pFbmCreateInfo, pFbmInfo);
            return (Fbm*)pFbmInfo->pFbmSecond;
        }
    }
    else
    {
        if(pFbmInfo->pFbmFirst == NULL)
        {
            pFbmInfo->pFbmFirst = FbmCreateBuffer(pFbmCreateInfo, pFbmInfo);
            //debugCreateBuffer(pFbmCreateInfo, pFbmInfo);
            return pFbmInfo->pFbmFirst;
        }
        else if(pFbmInfo->pFbmSecond == NULL)
        {
            pFbmInfo->pFbmSecond = FbmCreateBuffer(pFbmCreateInfo, pFbmInfo);
            pFbmInfo->nMinorYBufOffset = 0;
            pFbmInfo->nMinorCBufOffset = 0;
            //debugCreateBuffer(pFbmCreateInfo, pFbmInfo);
            return pFbmInfo->pFbmSecond;
        }
    }
    return NULL;
}

void FbmDestroy(Fbm* pFbm)
{
    logv("FbmDestroy");
    if(pFbm == NULL)
        return;
    pthread_mutex_destroy(&pFbm->mutex);
    if(pFbm->bUseGpuBuf == 0)
    {
         //* in thumbnail mode, we only allocate one picture buffer.
        if(pFbm->bThumbnailMode)
            FbmFreePictureBuffer(pFbm, &pFbm->pFrames[0].vpicture);
        else
        {
            int i;
            for(i=0; i<pFbm->nMaxFrameNum; i++)
                FbmFreePictureBuffer(pFbm, &pFbm->pFrames[i].vpicture);
        }
    }
    else
    {
        int i;
        for(i=0; i<pFbm->nMaxFrameNum; i++)
        {
            if(CdcIonGetMemType() == MEMORY_IOMMU)
            {
                struct user_iommu_param sUserIommuBuf;
                sUserIommuBuf.fd = pFbm->pFrames[i].vpicture.nBufFd;
                VideoFbmInfo* pFbmInfo = (VideoFbmInfo*)(pFbm->pFbmInfo);
                if(pFbmInfo->bIsSoftDecoderFlag==0)
                {
                    CdcVeFreeIommuAddr(pFbm->sVeops, pFbm->pVeOpsSelf, &sUserIommuBuf);
                }
             }
        }
    }
    free(pFbm->pFrames);
    free(pFbm);

    return;
}

VideoPicture* FbmRequestBuffer(Fbm* pFbm)
{
    //int        i;
    //int ret = 0;
    FrameNode* pFrameNode;
    VideoFbmInfo*  pFbmInfo = NULL;
    struct ScMemOpsS *_memops = NULL;

    logv("FbmRequestBuffer");
    if(pFbm == NULL)
        return NULL;

    _memops = pFbm->memops;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    pFrameNode = NULL;
    if(pFbmInfo == NULL)
        return NULL;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) &&(pFbm == pFbmInfo->pFbmSecond))
    {
        if(pFbmInfo->pMajorDecoderFrame == NULL)
        {
            return NULL;
        }
        pFbmInfo->pMajorDecoderFrame->nBufStatus |= MINOR_DECODER_USE_FLAG;
        pFbmInfo->pMajorDecoderFrame->nBufStatus &= ~MINOR_NEED_USE_FLAG;
        return pFbmInfo->pMajorDecoderFrame;
    }

    pFbmInfo->pMajorDecoderFrame = NULL;
    pthread_mutex_lock(&pFbm->mutex);
    pFrameNode = FbmDequeue(&pFbm->pEmptyBufferQueue);

    if(pFrameNode != NULL)
    {
        //* check the picture status.
        if(pFrameNode->Flag.bUsedByDecoder ||
            pFrameNode->Flag.bInValidPictureQueue ||
            pFrameNode->Flag.bUsedByRender ||
            pFrameNode->Flag.bAlreadyDisplayed)
        {
            //* the picture is in the pEmptyBufferQueue, these four flags
            //* shouldn't be set.
            loge("invalid frame status, a picture is just pick out from the pEmptyBufferQueue, \
                    but bUsedByDecoder=%d, bInValidPictureQueue=%d, bUsedByRender=%d,\
                    bAlreadyDisplayed=%d.",
                    pFrameNode->Flag.bUsedByDecoder, pFrameNode->Flag.bInValidPictureQueue,
                    pFrameNode->Flag.bUsedByRender, pFrameNode->Flag.bAlreadyDisplayed);
//            abort();
            pthread_mutex_unlock(&pFbm->mutex);
            return NULL;
        }
        pFbm->nEmptyBufferNum--;
        pFbm->nDecoderHoldingNum++;
        pFrameNode->Flag.bUsedByDecoder = 1;
        if(pFbmInfo->bTwoStreamShareOneFbm == 1)
        {
            pFbmInfo->pMajorDecoderFrame = &pFrameNode->vpicture;
            pFbmInfo->pMajorDecoderFrame->nBufStatus |= MAJOR_DECODER_USE_FLAG;
            pFbmInfo->pMajorDecoderFrame->nBufStatus |= MINOR_NEED_USE_FLAG;
        }

         //CdcMemSet(_memops,(void*)pFrameNode->vpicture.pData0,0,pFrameNode->vpicture.nBufSize);
         //CdcMemFlushCache(_memops, (void*)pFrameNode->vpicture.pData0,
         //pFrameNode->vpicture.nBufSize);
    }

    pthread_mutex_unlock(&pFbm->mutex);
    if((pFbmInfo->bIsFrameCtsTestFlag)&&(pFrameNode != NULL)
        && (pFrameNode->vpicture.nWidth < FBM_NEED_FLUSH_CACHE_SIZE))
    {
           CdcMemFlushCache(_memops, (void*)pFrameNode->vpicture.pData0,
        pFrameNode->vpicture.nWidth*pFrameNode->vpicture.nHeight*3/2);
    }
    return (pFrameNode==NULL)? NULL: &pFrameNode->vpicture;
}

void FbmReturnBuffer(Fbm* pFbm, VideoPicture* pVPicture, int bValidPicture)
{
    int        index;
    FrameNode* pFrameNode;
    //VideoPicture fbmPicBufInfo;
    VideoFbmInfo*  pFbmInfo = NULL;
    if(pFbm == NULL)
        return;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return;

    if(pVPicture == NULL)
    {
        return;
    }
    dy_log("FbmReturnBuffer pVPicture=%p, bValidPicture=%d, id=%d", \
        pVPicture, bValidPicture, pVPicture->nID);

    //* 3d -- newDisplay case
    if(pFbmInfo->bTwoStreamShareOneFbm == 1)
    {
        if(pFbm == pFbmInfo->pFbmFirst)
        {
            pVPicture->nBufStatus &= ~MAJOR_DECODER_USE_FLAG;
            if(pVPicture->nBufStatus & MINOR_DECODER_USE_FLAG)
            {
                return;
            }
            else if((bValidPicture==1) &&(pVPicture->nBufStatus & MINOR_NEED_USE_FLAG))
            {
                return;
            }
        }
        else if(pFbm == pFbmInfo->pFbmSecond)
        {
            pVPicture->nBufStatus &= ~MINOR_DECODER_USE_FLAG;
            if(pVPicture->nBufStatus & MAJOR_DECODER_USE_FLAG)
            {
                return;
            }
            pFbm = pFbmInfo->pFbmFirst;
        }
    }

    index = pVPicture->nID;

    //* check the index is valid.
    if(index < 0 || index >= pFbm->nMaxFrameNum)
    {
        loge("FbmReturnBuffer: the picture id is invalid, pVPicture=%p, pVPicture->nID=%d",
                pVPicture, pVPicture->nID);
        return;
    }
    else
    {
        if(pVPicture != &pFbm->pFrames[index].vpicture)
        {
            loge("FbmReturnBuffer: the picture id is invalid, pVPicture=%p, pVPicture->nID=%d",
                    pVPicture, pVPicture->nID);
            return;
        }
    }

    pFrameNode = &pFbm->pFrames[index];

    pthread_mutex_lock(&pFbm->mutex);

    if(pFrameNode->Flag.bUsedByDecoder == 0)
    {
        loge("invalid picture status, bUsedByDecoder=0 when picture buffer is returned.");
        //abort();
        pthread_mutex_unlock(&pFbm->mutex);
        return;
    }
    pFrameNode->Flag.bUsedByDecoder = 0;

    if((pFrameNode->Flag.bNeedRelease==1) &&(pFrameNode->Flag.bInValidPictureQueue==0))
    {
        FbmEnqueue(&pFbm->pReleaseBufferQueue, pFrameNode);
        pFbm->nReleaseBufferNum++;
        pFrameNode->Flag.bAlreadyDisplayed = 0;
        pFrameNode->Flag.bInValidPictureQueue = 0;
        pthread_mutex_unlock(&pFbm->mutex);
        logv("return buffer end\n");
        return;
    }

    //*the buffer was share before and in valid pic queue
    if(pFrameNode->Flag.bInValidPictureQueue)
    {
        //* check status.
        if(pFrameNode->Flag.bUsedByRender || pFrameNode->Flag.bAlreadyDisplayed)
        {
            loge("invalid frame status, a picture in pValidPictureQueue, \
                    but bUsedByRender=%d and Flag.bAlreadyDisplayed=%d",
                    pFrameNode->Flag.bUsedByRender, pFrameNode->Flag.bAlreadyDisplayed);
            //abort();
            pthread_mutex_unlock(&pFbm->mutex);
            return;
        }
    }
    //*the buffer was share before and used By Render
    else if(pFrameNode->Flag.bInValidPictureQueue == 0 && pFrameNode->Flag.bUsedByRender == 1)
    {
        //*do nothing
    }
    //*the buffer was share before and had already displayed
    else if(pFrameNode->Flag.bInValidPictureQueue == 0 && pFrameNode->Flag.bUsedByRender == 0
          && pFrameNode->Flag.bAlreadyDisplayed == 1)
    {
        pFrameNode->Flag.bAlreadyDisplayed = 0;
        FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
        pFbm->nEmptyBufferNum++;
    }
    //*the buffer was not share before
    else if(pFrameNode->Flag.bInValidPictureQueue == 0 && pFrameNode->Flag.bUsedByRender == 0
          && pFrameNode->Flag.bAlreadyDisplayed == 0)
    {
        if(bValidPicture)
        {
            FbmEnqueue(&pFbm->pValidPictureQueue, pFrameNode);
            pFbm->nValidPictureNum++;
            pFbm->nWaitForDispNum++;
            pFrameNode->Flag.bInValidPictureQueue = 1;
        }
        else
        {
            FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
            pFbm->nEmptyBufferNum++;
        }
    }

    pFbm->nDecoderHoldingNum--;
    pthread_mutex_unlock(&pFbm->mutex);
    logv("return buffer end\n");
    return;
}

void FbmShareBuffer(Fbm* pFbm, VideoPicture* pVPicture)
{
    int        index;
    FrameNode* pFrameNode;
    //VideoPicture* pDispVPicture = NULL;
    //VideoPicture fbmPicBufInfo;
    //int   bNeedDispBuffer = 0;
    VideoFbmInfo*  pFbmInfo = NULL;
    if(pFbm == NULL)
        return;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return;

    if(pVPicture == NULL)
    {
        return;
    }
    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm == pFbmInfo->pFbmSecond))
    {
        return;
    }

    dy_log("FbmShareBuffer pVPicture=%p", pVPicture);

    index = pVPicture->nID;

    //* check the index is valid.
    if(index < 0 || index >= pFbm->nMaxFrameNum)
    {
        loge("FbmShareBuffer: the picture id is invalid, \
            pVPicture=%p, pVPicture->nID=%d, pFbm->nMaxFrameNum=%d",
                pVPicture, pVPicture->nID, pFbm->nMaxFrameNum);
        return;
    }
    else
    {
        if(pVPicture != &pFbm->pFrames[index].vpicture)
        {
            loge("FbmShareBuffer: the picture id is invalid, pVPicture=%p, pVPicture->nID=%d",
                    pVPicture, pVPicture->nID);
            return;
        }
    }

    pFrameNode = &pFbm->pFrames[index];
    if(pFrameNode->Flag.bNeedRelease == 1)
    {
        logv("the buffer need release\n");
        return;
    }

    pthread_mutex_lock(&pFbm->mutex);

    //* check status.
    if(pFrameNode->Flag.bUsedByDecoder == 0 ||
        pFrameNode->Flag.bInValidPictureQueue == 1 ||
        pFrameNode->Flag.bAlreadyDisplayed == 1)
    {
        loge("invalid frame status, a picture is shared but bUsedByDecoder=%d, \
                bInValidPictureQueue=%d, bUsedByDecoder=%d, bAlreadyDisplayed=%d.",
                pFrameNode->Flag.bUsedByDecoder, pFrameNode->Flag.bInValidPictureQueue,
                pFrameNode->Flag.bUsedByDecoder, pFrameNode->Flag.bAlreadyDisplayed);
        //abort();
        pthread_mutex_unlock(&pFbm->mutex);
        return;
    }

    //* put the picture to pValidPictureQueue.
    pFrameNode->Flag.bInValidPictureQueue = 1;
    FbmEnqueue(&pFbm->pValidPictureQueue, pFrameNode);
    pFbm->nValidPictureNum++;
    pFbm->nWaitForDispNum++;

    pthread_mutex_unlock(&pFbm->mutex);
    logv("end sharebuffer\n");
    return;
}

VideoPicture* FbmRequestPicture(Fbm* pFbm)
{
    VideoPicture* pVPicture;
    FrameNode*    pFrameNode;
    VideoFbmInfo*  pFbmInfo = NULL;

    logv("FbmRequestPicture, bDynamicShowLogFlag = %d",bDynamicShowLogFlag);
    if(bDynamicShowLogFlag == 1 || pFbm->bShowLogFlag == 1)
        FbmDebugPrintStatus(pFbm);

    if(pFbm == NULL)
        return NULL;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return NULL;

    if((pFbmInfo->bTwoStreamShareOneFbm  == 1) && (pFbm ==pFbmInfo->pFbmSecond))
    {
        pFbmInfo->pMajorDispFrame->nBufStatus |= MINOR_DISP_USE_FLAG;
        logv("**************here7:  pVPicture->nBufStatus =%x\n",
            pFbmInfo->pMajorDispFrame->nBufStatus);
        return pFbmInfo->pMajorDispFrame;
    }

    pthread_mutex_lock(&pFbm->mutex);

   // logi("FbmRequestPicture");

    pVPicture  = NULL;
    pFbmInfo->pMajorDispFrame = NULL;
    pFrameNode = FbmDequeue(&pFbm->pValidPictureQueue);

    if(pFrameNode != NULL)
    {
        if(pFrameNode->Flag.bInValidPictureQueue == 0 ||
            pFrameNode->Flag.bUsedByRender == 1 ||
            pFrameNode->Flag.bAlreadyDisplayed == 1)
        {
            //* the picture is in the pValidPictureQueue, one of these three flags is invalid.
            loge("invalid frame status, a picture is just pick out from the pValidPictureQueue, \
                    but bInValidPictureQueue=%d, bUsedByRender=%d, bAlreadyDisplayed=%d.",
                    pFrameNode->Flag.bInValidPictureQueue,
                    pFrameNode->Flag.bUsedByRender,
                    pFrameNode->Flag.bAlreadyDisplayed);
            //abort();
            pthread_mutex_unlock(&pFbm->mutex);
            return NULL;
        }

        pFbm->nValidPictureNum--;
        pFbm->nWaitForDispNum--;
        pFbm->nRenderHoldingNum++;
        pFrameNode->Flag.bInValidPictureQueue = 0;
        pVPicture = &pFrameNode->vpicture;
        pFrameNode->Flag.bUsedByRender = 1;
        pFbmInfo->pMajorDispFrame = pVPicture;
        pVPicture->nBufStatus |= MAJOR_DISP_USE_FLAG;
        logv("**************here8:  pVPicture->nBufStatus =%x\n",   pVPicture->nBufStatus);
    }

    pthread_mutex_unlock(&pFbm->mutex);

    return (pFrameNode==NULL)? NULL: pVPicture;
}

int FbmReturnPicture(Fbm* pFbm, VideoPicture* pVPicture)
{
    int        index;
    FrameNode* pFrameNode;
    VideoFbmInfo*  pFbmInfo = NULL;
    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if(pVPicture == NULL)
    {
        return 0;
    }
    dy_log("****FbmReturnPicture pVPicture=%p, id = %d", pVPicture,pVPicture->nID);
    if(pFbmInfo->bTwoStreamShareOneFbm == 1)
    {
        if(pFbm == pFbmInfo->pFbmFirst)
        {
              pVPicture->nBufStatus &= ~MAJOR_DISP_USE_FLAG;
              if(pVPicture->nBufStatus & MINOR_DISP_USE_FLAG)
              {
                  return 0;
              }
        }
        else if(pFbm == pFbmInfo->pFbmSecond)
        {
              pVPicture->nBufStatus &= ~MINOR_DISP_USE_FLAG;
              if(pVPicture->nBufStatus & MAJOR_DISP_USE_FLAG)
              {
                  return 0;
              }
        }
    }

    int i;

    if(pFbm->bUseGpuBuf == 1)
    {
        for(i=0; i<pFbm->nMaxFrameNum; i++)
        {
            if((pVPicture->nBufFd == pFbm->pFrames[i].vpicture.nBufFd)
                        && !pFbm->pFrames[i].Flag.bNeedRelease)
                        break;
        }
    }
    else
    {
        for(i=0; i<pFbm->nMaxFrameNum; i++)
        {
            if(pVPicture->phyYBufAddr == pFbm->pFrames[i].vpicture.phyYBufAddr)
                break;
        }
    }

    if(i == pFbm->nMaxFrameNum)
    {
        loge("FbmReturnPicture: cannot find this picture");
        return -1;
    }
    index = pFbm->pFrames[i].vpicture.nID;

    //* check the index is valid.
    if(index < 0 || index >= pFbm->nMaxFrameNum)
    {
        if(index >= FRM_FORBID_USE_TAG)
        {
             pFrameNode = &pFbm->pFrames[index-FRM_FORBID_USE_TAG];
             pthread_mutex_lock(&pFbm->mutex);
             if(pFrameNode->Flag.bUsedByRender!=0 ||
                pFrameNode->Flag.bInValidPictureQueue != 0 ||
                pFrameNode->Flag.bAlreadyDisplayed != 0)
             {
                 loge(" check index valid error. bUsedByRender: \
                         %d, bInValidPictureQueue: %d, bAlreadyDisplayed: %d ", \
                         pFrameNode->Flag.bUsedByRender, pFrameNode->Flag.bInValidPictureQueue, \
                         pFrameNode->Flag.bAlreadyDisplayed);
                 //abort();
                 pthread_mutex_unlock(&pFbm->mutex);
                 return -1;
             }
             pFrameNode->vpicture.nID -= FRM_FORBID_USE_TAG;
             FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
             pFbm->nEmptyBufferNum++;
             if(pVPicture->nID >= FRM_FORBID_USE_TAG)
             {
                pVPicture->nID -= FRM_FORBID_USE_TAG;
             }
             pthread_mutex_unlock(&pFbm->mutex);
             return 0;
        }
        logw("FbmReturnPicture: the picture id is invalid, pVPicture=%p, pVPicture->nID=%d",\
                pVPicture, pVPicture->nID);
        return -1;
    }

    pFrameNode = &pFbm->pFrames[index];

    if(pFbm->bThumbnailMode == 1)
    {
         pFrameNode = &pFbm->pFrames[pVPicture->nID];
    }

    pthread_mutex_lock(&pFbm->mutex);

    if(pFrameNode->Flag.bUsedByRender == 0 ||
        pFrameNode->Flag.bInValidPictureQueue == 1 ||
        pFrameNode->Flag.bAlreadyDisplayed == 1)
    {
        //* the picture is being returned, but one of these three flags is invalid.
        loge("invalid frame status, a picture being returned, \
                but bUsedByRender=%d, bInValidPictureQueue=%d, bAlreadyDisplayed=%d.",
                    pFrameNode->Flag.bUsedByRender,
                    pFrameNode->Flag.bInValidPictureQueue,
                    pFrameNode->Flag.bAlreadyDisplayed);
        loge("**picture[%p],id[%d]",&pFrameNode->vpicture,pFrameNode->vpicture.nID);
        //abort();
        pthread_mutex_unlock(&pFbm->mutex);
        return -1;
    }

    pFrameNode->Flag.bUsedByRender = 0;
    pFbm->nRenderHoldingNum--;
    if(pFrameNode->Flag.bUsedByDecoder)
        pFrameNode->Flag.bAlreadyDisplayed = 1;
    else
    {
          if(pFrameNode->Flag.bNeedRelease == 0)
        {
              FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
              pFbm->nEmptyBufferNum++;
        }
          else
        {
              FbmEnqueue(&pFbm->pReleaseBufferQueue, pFrameNode);
              pFbm->nReleaseBufferNum++;
        }
    }

    pthread_mutex_unlock(&pFbm->mutex);

    return 0;
}

VideoPicture* FbmNextPictureInfo(Fbm* pFbm)
{
    FrameNode* pFrameNode;

   // logi("FbmNextPictureInfo");
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;

    if(pFbm == NULL)
    {
        return NULL;
    }

    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return NULL;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }

    if(ppFbm->pValidPictureQueue != NULL)
    {
        pFrameNode = ppFbm->pValidPictureQueue;
        return &pFrameNode->vpicture;
    }
    else
        return NULL;
}

void FbmFlush(Fbm* pFbm)
{
    FrameNode* pFrameNode;
    //VideoPicture fbmPicBufInfo;

    VideoFbmInfo*  pFbmInfo = NULL;

    if(pFbm == NULL)
        return;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        return;
    }

    pthread_mutex_lock(&pFbm->mutex);

    logv("FbmFlush");

    while((pFrameNode = FbmDequeue(&pFbm->pValidPictureQueue)) != NULL)
    {
        pFrameNode->vpicture.nBufStatus = 0;
        pFbm->nValidPictureNum--;

        //* check the picture status.
        if(pFrameNode->Flag.bUsedByRender || pFrameNode->Flag.bAlreadyDisplayed)
        {
            //* the picture is in the pValidPictureQueue, these two flags
            //* shouldn't be set.
            loge("invalid frame status, a picture is just pick out from the pValidPictureQueue, \
                    but bUsedByRender=%d and bAlreadyDisplayed=%d.",
                    pFrameNode->Flag.bUsedByRender, pFrameNode->Flag.bAlreadyDisplayed);
            //abort();
            pthread_mutex_unlock(&pFbm->mutex);
            return;
        }

        pFrameNode->Flag.bInValidPictureQueue = 0;
        if(pFrameNode->Flag.bUsedByDecoder == 0)
        {
            //* the picture is not used, put it into pEmptyBufferQueue.
            FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
            pFbm->nEmptyBufferNum++;
        }
        else
        {
            //* the picture was shared by the video engine,
            //* set the bAlreadyDisplayed so the picture won't be put into
            //* the pValidPictureQueue when it is returned by the video engine.
            pFrameNode->Flag.bAlreadyDisplayed = 1;
        }
    }

    pthread_mutex_unlock(&pFbm->mutex);

    return;
}

int FbmGetBufferInfo(Fbm* pFbm, VideoPicture* pVPicture)
{
    CEDARC_UNUSE(FbmGetBufferInfo);

    FrameNode* pFrameNode;

    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;

    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }

    logi("FbmGetBufferInfo");

    //* give the general information of the video pictures.
    pFrameNode = &ppFbm->pFrames[0];
    pVPicture->ePixelFormat   = pFrameNode->vpicture.ePixelFormat;
    pVPicture->nWidth         = pFrameNode->vpicture.nWidth;
    pVPicture->nHeight        = pFrameNode->vpicture.nHeight;
    pVPicture->nLineStride    = pFrameNode->vpicture.nLineStride;
    pVPicture->nTopOffset     = pFrameNode->vpicture.nTopOffset;
    pVPicture->nLeftOffset    = pFrameNode->vpicture.nLeftOffset;
    pVPicture->nFrameRate     = pFrameNode->vpicture.nFrameRate;
    pVPicture->nAspectRatio   = pFrameNode->vpicture.nAspectRatio;
    pVPicture->bIsProgressive = pFrameNode->vpicture.bIsProgressive;

    return 0;
}

int FbmTotalBufferNum(Fbm* pFbm)
{
    logi("FbmTotalBufferNum");
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;

    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    return ppFbm->nMaxFrameNum;
}

int FbmEmptyBufferNum(Fbm* pFbm)
{
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;
    int nEmptyBufferNum = 0;

    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    nEmptyBufferNum = ppFbm->nEmptyBufferNum;
    return nEmptyBufferNum;
}

int FbmGetDecoderHoldingBufferNum(Fbm* pFbm)
{
    CEDARC_UNUSE(FbmGetDecoderHoldingBufferNum);

    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;
    int Num = 0;

    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    Num = ppFbm->nDecoderHoldingNum;
    return Num;
}
int FbmGetRenderHoldingBufferNum(Fbm* pFbm)
{
    CEDARC_UNUSE(FbmGetRenderHoldingBufferNum);

    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;
    int Num = 0;
    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;
    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    Num = ppFbm->nRenderHoldingNum;
    return Num;
}
int FbmGetDisplayBufferNum(Fbm* pFbm)
{
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;
    int Num = 0;
    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;
    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    Num = ppFbm->nRenderHoldingNum + ppFbm->nValidPictureNum;
    return Num;
}
int FbmValidPictureNum(Fbm* pFbm)
{
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;
    int nValidPictureNum = 0;
    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;
    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    nValidPictureNum = ppFbm->nValidPictureNum;
    return nValidPictureNum;
}

int FbmGetAlignValue(Fbm* pFbm)
{
    VideoFbmInfo*  pFbmInfo = NULL;
    Fbm* ppFbm = NULL;

    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bTwoStreamShareOneFbm == 1) && (pFbm==pFbmInfo->pFbmSecond))
    {
        ppFbm = pFbmInfo->pFbmFirst;
    }
    else
    {
        ppFbm = pFbm;
    }
    return ppFbm->nAlignValue;
}

int FbmAllocatePictureBuffer(Fbm* pFbm, VideoPicture* pPicture,
            int* nAlignValue, int nWidth, int nHeight)
{
    int   ePixelFormat;
    int   nHeight16Align;
    int   nHeight32Align;
    char* pMem;
    int   nMemSizeY;
    int   nMemSizeC;
    int   nLineStride;
    int   nHeight64Align;

    //void *veOps = NULL;
    //void *pVeopsSelf = NULL;

    ePixelFormat   = pPicture->ePixelFormat;
    nHeight16Align = (nHeight+15) & ~15;
    nHeight32Align = (nHeight+31) & ~31;
    nHeight64Align = (nHeight+63) & ~63;

    pPicture->pData0 = NULL;
    pPicture->pData1 = NULL;
    pPicture->pData2 = NULL;
    pPicture->pData3 = NULL;

    switch(ePixelFormat)
    {
        case PIXEL_FORMAT_P010_UV:
        case PIXEL_FORMAT_P010_VU:
        {

            if(*nAlignValue == 0)
           {
                nLineStride = (nWidth+15) &~ 15;
                pPicture->nLineStride = nLineStride;
                pPicture->nWidth = pPicture->nLineStride;
                pPicture->nHeight = nHeight16Align;
                nMemSizeY = pPicture->nWidth*pPicture->nHeight*2;
                *nAlignValue  = 16;
            }
            else
            {
                nLineStride = (nWidth+*nAlignValue-1) &~ (*nAlignValue-1);
                pPicture->nLineStride = nLineStride;
                pPicture->nWidth = pPicture->nLineStride;
                pPicture->nHeight = (nHeight+*nAlignValue-1) &~ (*nAlignValue-1);;
                nMemSizeY = pPicture->nWidth*pPicture->nHeight*2;
            }
            nMemSizeC = nMemSizeY>>2;
            pMem = (char*)CdcMemPalloc(pFbm->memops, nMemSizeY + nMemSizeC*2,
                   pFbm->sVeops, pFbm->pVeOpsSelf);
            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture buffer.", \
                    nMemSizeY + nMemSizeC*2);
                return -1;
            }

            memset(pMem, 0, nMemSizeY + nMemSizeC*2);
            CdcMemFlushCache(pFbm->memops,pMem, nMemSizeY + nMemSizeC*2);

            pPicture->pData0 = pMem;

            pPicture->pData1 = pMem+nMemSizeY;
            pPicture->pData2 = NULL;
            pPicture->nBufSize = nMemSizeY+2*nMemSizeC;
            break;
        }
        case PIXEL_FORMAT_YUV_PLANER_420:
        case PIXEL_FORMAT_YUV_PLANER_422:
        case PIXEL_FORMAT_YUV_PLANER_444:
        case PIXEL_FORMAT_YV12:
        case PIXEL_FORMAT_NV21:
        case PIXEL_FORMAT_NV12:
        {
            //* for decoder,
            //* height of Y component is required to be 16 aligned, for example,
            //* 1080 becomes to 1088.
            //* width and height of U or V component are both required to be 8 aligned.
            //* nLineStride should be 16 aligned.
            if(*nAlignValue == 0)
            {
                nLineStride = (nWidth+15) &~ 15;
                pPicture->nLineStride = nLineStride;
                pPicture->nWidth = pPicture->nLineStride;
                pPicture->nHeight = nHeight16Align;
                nMemSizeY = pPicture->nWidth*pPicture->nHeight;
                *nAlignValue  = 16;
            }
            else
            {
                nLineStride = (nWidth+*nAlignValue-1) &~ (*nAlignValue-1);
                pPicture->nLineStride = nLineStride;
                pPicture->nWidth = pPicture->nLineStride;
                pPicture->nHeight = (nHeight+*nAlignValue-1) &~ (*nAlignValue-1);;
                nMemSizeY = pPicture->nWidth*pPicture->nHeight;
            }

            if(ePixelFormat == PIXEL_FORMAT_YUV_PLANER_420 ||
               ePixelFormat == PIXEL_FORMAT_YV12 ||
               ePixelFormat == PIXEL_FORMAT_NV21 ||
               ePixelFormat == PIXEL_FORMAT_NV12)
                nMemSizeC = nMemSizeY>>2;
            else if(ePixelFormat == PIXEL_FORMAT_YUV_PLANER_422)
                nMemSizeC = nMemSizeY>>1;
            else
                nMemSizeC = nMemSizeY;  //* PIXEL_FORMAT_YUV_PLANER_444

            int nLower2BitBufSize = 0;
            int nAfbcBufSize = 0;
            int nNormalYuvBufSize = nMemSizeY + nMemSizeC*2;
            int nTotalPicPhyBufSize = 0;
            //int frmbuf_c_size = 0;

            nLower2BitBufSize = ((((nWidth+3)>>2) + 31) & 0xffffffe0) * nHeight * 3/2;
            //int PriChromaStride = ((nWidth/2) + 31)&0xffffffe0;
            //frmbuf_c_size = 2 * (PriChromaStride * (((nHeight/2)+15)&0xfffffff0)/4);
            //logd("frmbuf_c_size : %d(%0.2f)",frmbuf_c_size,(float)frmbuf_c_size/1024/1024);
            if(pPicture->b10BitPicFlag)
            {
                if(pPicture->bEnableAfbcFlag == 1)
                {
                    nAfbcBufSize = ((nWidth+15)>>4) * ((nHeight+4+15)>>4) * (512 + 16) + 32 + 1024;
                    nTotalPicPhyBufSize = nAfbcBufSize + nLower2BitBufSize /*+ frmbuf_c_size*/;
                    logd("TotalSize: %d(%0.2f MB),AfbcSize: %d(%0.2f MB),Low2Size: %d(%0.2f MB)",
                           nTotalPicPhyBufSize,(float)nTotalPicPhyBufSize/1024/1024,
                           nAfbcBufSize,(float)nAfbcBufSize/1024/1024,
                           nLower2BitBufSize,(float)nLower2BitBufSize/1024/1024);
                }
                else
                {
                    nTotalPicPhyBufSize = nNormalYuvBufSize + nLower2BitBufSize;
                }
            }
            else
            {
                if(pPicture->bEnableAfbcFlag == 1)
                {
                    nAfbcBufSize = ((nWidth+15)>>4) * ((nHeight+4+15)>>4) * (384 + 16) + 32 + 1024;
                    nTotalPicPhyBufSize = nAfbcBufSize/* + frmbuf_c_size*/;
                }
                else
                {
                    nTotalPicPhyBufSize = nNormalYuvBufSize;
                }
            }

            if(pPicture->nLbcLossyComMod > 0)
            {
                int nLbcBufferSize = 0;
                int cmp_ratio = 0;
                int seg_wth = 16;
                int seg_hgt = 4;
                int bit_depth = 8;
                //int ALIGN = 128;
                int seg_tar_bits;
                int segline_tar_bits;
                int y_mode_bits, c_mode_bits, y_data_bits, c_data_bits;
                int info_buffer_size;
                int frm_wth = nWidth;
                int frm_hgt = (nHeight+3) &~ 3;

                if(pPicture->b10BitPicFlag == 1)
                {
                    bit_depth = 10;
                }

                if(pPicture->nLbcLossyComMod == 1)//1.5x
                {
                    cmp_ratio = 666;
                }
                else if(pPicture->nLbcLossyComMod == 2)//2x
                {
                    cmp_ratio = 500;
                }
                else if(pPicture->nLbcLossyComMod == 3)//2.5x
                {
                    cmp_ratio = 400;
                }

                if(pPicture->bIsLossy)
                {
                    seg_tar_bits = ((seg_wth * seg_hgt * bit_depth * cmp_ratio * 3 / 2000) / ALIGN) * ALIGN;
                    if(pPicture->bRcEn)
                    {
                        segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth * seg_wth * seg_hgt * bit_depth * cmp_ratio * 3 / 2000 + ALIGN - 1) / ALIGN * ALIGN;
                    }
                    else
                    {
                        segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth) * seg_tar_bits;
                    }
                }
                else
                {
                    y_mode_bits = seg_wth / MB_WTH * (CODE_MODE_Y_BITS * 2 + BLK_MODE_BITS);
                    c_mode_bits = 2 * (seg_wth / 2 / MB_WTH * CODE_MODE_C_BITS);
                    y_data_bits = seg_wth * seg_hgt * bit_depth;
                    c_data_bits = seg_wth * seg_hgt * bit_depth / 2 + 2 * (seg_wth / 2 / MB_WTH) * C_DTS_BITS;
                    seg_tar_bits= (y_data_bits + c_data_bits + y_mode_bits + c_mode_bits + ALIGN - 1) / ALIGN * ALIGN;
                    segline_tar_bits = ((frm_wth + seg_wth - 1) / seg_wth * seg_wth / seg_wth) * seg_tar_bits;
                }
                info_buffer_size = ((((frm_wth + seg_wth - 1) / seg_wth + 7) / 8) * 8 * 16 * (frm_hgt /seg_hgt) + 7) / 8;
                nTotalPicPhyBufSize = (segline_tar_bits * frm_hgt / seg_hgt + 7) / 8 + info_buffer_size;

                pPicture->nLbcSize = nTotalPicPhyBufSize;
                pPicture->nWidth = nWidth;
                pPicture->nHeight = nHeight;

                logd("the_lbc:cmp_ratio=%d, buffer size=%d", cmp_ratio, nTotalPicPhyBufSize);
            }

            pMem = (char*)CdcMemPalloc(pFbm->memops, nTotalPicPhyBufSize/*nMemSizeY + nMemSizeC*2*/,
                                       pFbm->sVeops, pFbm->pVeOpsSelf);
            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture buffer.", \
                    nMemSizeY + nMemSizeC*2);
                return -1;
            }
            logd("pPicture->bEnableAfbcFlag = %d",pPicture->bEnableAfbcFlag);
            if(pPicture->bEnableAfbcFlag == 1)
            {
                memset(pMem, 0, nTotalPicPhyBufSize);
                CdcMemFlushCache(pFbm->memops,pMem, nTotalPicPhyBufSize);
            }

            if(pPicture->b10BitPicFlag)
            {
                if(pPicture->bEnableAfbcFlag == 1)
                {
                    pPicture->pData0 = pMem;
                    //pPicture->pLower2BitVirtAddr = pMem + nAfbcBufSize;
                    pPicture->nLower2BitBufSize  = nLower2BitBufSize;
                    pPicture->nLower2BitBufOffset = nAfbcBufSize/* + frmbuf_c_size*/;
                    pPicture->nLower2BitBufStride = ((((nWidth+3)>>2) + 31) & 0xffffffe0);
                    pPicture->nAfbcSize = nAfbcBufSize;
                }
                else
                {
                    pPicture->pData0 = pMem;
                    pPicture->pData1 = pMem + nMemSizeY;
                    pPicture->nLower2BitBufSize  = nLower2BitBufSize;
                    pPicture->nLower2BitBufOffset = nNormalYuvBufSize;
                    pPicture->nLower2BitBufStride = ((((nWidth+3)>>2) + 31) & 0xffffffe0);
                }
            }
            else
            {
                if(pPicture->bEnableAfbcFlag == 1)
                {
                    pPicture->pData0 = pMem;
                    pPicture->nAfbcSize = nAfbcBufSize;
                }
                else
                {
                    pPicture->pData0 = pMem;
                    pPicture->pData1 = pMem + nMemSizeY;
                    if((ePixelFormat != PIXEL_FORMAT_NV21) && (ePixelFormat != PIXEL_FORMAT_NV12))
                    pPicture->pData2 = pMem + nMemSizeY + nMemSizeC;
                    else
                    pPicture->pData2 = NULL;    //* U and V component are combined at pData1.
                }
            }

            if(pPicture->nLbcLossyComMod > 0)
            {
                pPicture->pData0 = pMem;
                pPicture->pData1 = pMem;
                pPicture->pData2 = NULL;
            }
            pPicture->pData3 = NULL;
            pPicture->nBufSize = nTotalPicPhyBufSize;

            pMem = (char*)malloc(4*1024);
            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture metadata buffer", 4*1024);
                return -1;
            }
            memset(pMem, 0, 4*1024);
            CdcMemFlushCache(pFbm->memops, pMem, 4*1024);
            pPicture->pMetaData = (void*)pMem;

            break;
        }
        case PIXEL_FORMAT_YUV_MB32_420:
        case PIXEL_FORMAT_YUV_MB32_422:
        case PIXEL_FORMAT_YUV_MB32_444:
        {
            //* for decoder,
            //* height of Y component is required to be 32 aligned.
            //* height of UV component are both required to be 32 aligned.
            //* nLineStride should be 32 aligned.
            #if 0
            *nAlignValue = 32;

            pPicture->nLineStride = (nWidth+31)&~31;
            nLineStride = (pPicture->nWidth+63) &~ 63;
            pPicture->nWidth = nLineStride;
            pPicture->nHeight = nHeight64Align;

            nMemSizeY = pPicture->nWidth*pPicture->nHeight;

            if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_420)
                nMemSizeC = nLineStride*nHeight64Align/4;
            else if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_422)
                nMemSizeC = nLineStride*nHeight64Align/2;
            else
                nMemSizeC = nLineStride*nHeight64Align;

            pMem = (char*)CdcMemPalloc(memops, nMemSizeY);
            #else
            *nAlignValue = 32;

            pPicture->nLineStride = (nWidth+31)&~31;
            pPicture->nWidth = pPicture->nLineStride;
            pPicture->nHeight = nHeight32Align;
            nLineStride = (pPicture->nWidth+63) &~ 63;
            nMemSizeY = nLineStride*nHeight64Align;

            if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_420)
                nMemSizeC = nLineStride*nHeight64Align/4;
            else if(ePixelFormat == PIXEL_FORMAT_YUV_MB32_422)
                nMemSizeC = nLineStride*nHeight64Align/2;
            else
                nMemSizeC = nLineStride*nHeight64Align;

            pMem = (char*)CdcMemPalloc(pFbm->memops, nMemSizeY,
                pFbm->sVeops, pFbm->pVeOpsSelf);
            #endif

            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture buffer.", nMemSizeY);
                return -1;
            }
            memset(pMem, 0, nMemSizeY);
            CdcMemFlushCache(pFbm->memops,pMem, nMemSizeY);
            pPicture->pData0 = pMem;

            pMem = (char*)CdcMemPalloc(pFbm->memops, nMemSizeC*2,
                     pFbm->sVeops, pFbm->pVeOpsSelf);    //* UV is combined.
            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture buffer.", nMemSizeC*2);
                CdcMemPfree(pFbm->memops, pPicture->pData0,
                            pFbm->sVeops, pFbm->pVeOpsSelf);
                pPicture->pData0 = NULL;
                return -1;
            }
            memset(pMem, 0, nMemSizeC*2);
            CdcMemFlushCache(pFbm->memops,pMem, nMemSizeC*2);
            pPicture->pData1 = pMem;
            pPicture->pData2 = NULL;
            pPicture->pData3 = NULL;
            pPicture->nBufSize = nMemSizeY+2*nMemSizeC;

            break;
        }
        case PIXEL_FORMAT_RGBA:
        case PIXEL_FORMAT_ARGB:
        case PIXEL_FORMAT_ABGR:
        case PIXEL_FORMAT_BGRA:
        {
            nLineStride = pPicture->nWidth*4;
            pPicture->nLineStride = nLineStride;

            nMemSizeY = nLineStride*nHeight;
            pMem = (char *)CdcMemPalloc(pFbm->memops,nMemSizeY,
                           pFbm->sVeops, pFbm->pVeOpsSelf);
            if(pMem == NULL)
            {
                loge("memory alloc fail, require %d bytes for picture buffer.", nMemSizeY);
                return -1;
            }
            // must memset pixels to oxFF, or jpeg_test in skia will failed
            memset(pMem, 0xff, nMemSizeY);
            CdcMemFlushCache(pFbm->memops,pMem, nMemSizeY);

            pPicture->pData0 = pMem;
            pPicture->pData1 = NULL;
            pPicture->pData2 = NULL;
            pPicture->pData3 = NULL;
            pPicture->nBufSize = nMemSizeY;
            break;
        }
        default:
            loge("pixel format incorrect, ePixelFormat=%d", ePixelFormat);
            return -1;
    }

    if(pPicture->pData0 != NULL)
    {
        pPicture->phyYBufAddr = (size_addr)CdcMemGetPhysicAddress(pFbm->memops, pPicture->pData0);
    }
    if(pPicture->pData1 != NULL)
    {
        pPicture->phyCBufAddr = (size_addr)CdcMemGetPhysicAddress(pFbm->memops, pPicture->pData1);
    }
    return 0;
}

int FbmFreePictureBuffer(Fbm* pFbm, VideoPicture* pPicture)
{
    if(pPicture->pData0 != NULL)
    {
        CdcMemPfree(pFbm->memops, pPicture->pData0, pFbm->sVeops,
            pFbm->pVeOpsSelf);
        pPicture->pData0 = NULL;
    }

    if(pPicture->pMetaData != NULL && pFbm->bUseGpuBuf == 0)
    {
        free(pPicture->pMetaData);
        pPicture->pMetaData = NULL;
    }

    if(pPicture->ePixelFormat < PIXEL_FORMAT_YUV_MB32_420)
    {
        return 0;
    }
    if(pPicture->pData1 != NULL)
    {
        CdcMemPfree(pFbm->memops,pPicture->pData1,
                  pFbm->sVeops, pFbm->pVeOpsSelf);
        pPicture->pData1 = NULL;
    }

    if(pPicture->pData2 != NULL)
    {
        CdcMemPfree(pFbm->memops,pPicture->pData2,
                    pFbm->sVeops, pFbm->pVeOpsSelf);
        pPicture->pData2 = NULL;
    }

    if(pPicture->pData3 != NULL)
    {
        CdcMemPfree(pFbm->memops,pPicture->pData3,
                    pFbm->sVeops, pFbm->pVeOpsSelf);
        pPicture->pData3 = NULL;
    }
    return 0;
}

int FbmUpdateProcInfo(Fbm* pFbm)
{

    int i = 0;
    int nValidNum = 0 ;
    int nDecoderHoldNum = 0;
    int nRenderHoldNum = 0;

    if(pFbm == NULL)
        return -1;

    VideoFbmInfo* pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;

    for(i=0; i<pFbm->nMaxFrameNum; i++)
    {
        if(pFbm->pFrames[i].Flag.bInValidPictureQueue == 1)
            nValidNum++;

        if(pFbm->pFrames[i].Flag.bUsedByDecoder == 1)
            nDecoderHoldNum++;

        if(pFbm->pFrames[i].Flag.bUsedByRender == 1)
            nRenderHoldNum++;


        logv("i=%d, picture=%p, displed=%d, valid=%d, decode=%d, render=%d\n", \
            i, &pFbm->pFrames[i].vpicture, pFbm->pFrames[i].Flag.bAlreadyDisplayed, \
            pFbm->pFrames[i].Flag.bInValidPictureQueue,  pFbm->pFrames[i].Flag.bUsedByDecoder, \
            pFbm->pFrames[i].Flag.bUsedByRender);
    }

    char* pDebugInfo = NULL;
    int count = 0;

    pDebugInfo = (char*)calloc(1, 10*1024);
    if(pDebugInfo == NULL)
    {
        loge("malloc for pDebugInfo falied");
        return -1;
    }

    count += sprintf(pDebugInfo + count, "fbm: totalNum[%d], validNum[%d], decHoldNum[%d], \
renderHoldNum[%d]\n",
    pFbm->nMaxFrameNum,nValidNum, nDecoderHoldNum, nRenderHoldNum);

    count += sprintf(pDebugInfo + count, "fbm: align[%d], bGpuBuf[%d], w&h[%d,%d], \
offset[%d,%d,%d,%d], bHdr[%d], b10bit[%d], bAfbc[%d]\n",
                     pFbm->nAlignValue, pFbm->bUseGpuBuf,
                     pFbmInfo->pFbmBufInfo.nBufWidth,
                     pFbmInfo->pFbmBufInfo.nBufHeight,
                     pFbmInfo->pFbmBufInfo.nLeftOffset,
                     pFbmInfo->pFbmBufInfo.nRightOffset,
                     pFbmInfo->pFbmBufInfo.nTopOffset,
                     pFbmInfo->pFbmBufInfo.nBottomOffset,
                     pFbmInfo->pFbmBufInfo.bHdrVideoFlag,
                     pFbmInfo->pFbmBufInfo.b10bitVideoFlag,
                     pFbmInfo->pFbmBufInfo.bAfbcModeFlag);

    logv("count = %d",count);
    CdcVeProcInfoUpdate(pFbm->sVeops, pFbm->pVeOpsSelf,
                         pDebugInfo, count, 0);

    free(pDebugInfo);

    return 0;
}


#if 1
/**************************************************************
 * ******these functions only be used in new display structure.
***************************************************************/
FbmBufInfo* FbmGetVideoBufferInfo(VideoFbmInfo* pFbmInf)
{
    if(pFbmInf == NULL)
    {
        return NULL;
    }
    if(pFbmInf->bIs3DStream==1)
    {
        if((pFbmInf->pFbmFirst!=NULL) &&(pFbmInf->pFbmSecond!=NULL))
        {
            return &(pFbmInf->pFbmBufInfo);
        }
    }
    //else if(pFbmInf->pFbmFirst != NULL)
    else if(pFbmInf->pFbmBufInfo.nBufNum == 0)
    {
        return NULL;
    }
    else
    {
        return &(pFbmInf->pFbmBufInfo);
    }
    return NULL;
}

VideoPicture* FbmSetFbmBufAddress(VideoFbmInfo* pFbmInfo,
            VideoPicture* pPicture, int bForbidUseFlag)
{
    Fbm* pFbm = NULL;

    FrameNode* pFrameNode = NULL;
    //int nYBufferSize = 0;
    if(pFbmInfo == NULL)
        return NULL;

    if((pFbmInfo->bIs3DStream==1)||(pFbmInfo->pFbmSecond==NULL))
    {
        pFbm = (Fbm*)pFbmInfo->pFbmFirst;
    }
    else if(pFbmInfo->pFbmSecond != NULL)
    {
        pFbm = (Fbm*)pFbmInfo->pFbmSecond;
    }

    if(pFbm == NULL)
    {
        loge("the handle is error:pFbm=%p\n",pFbm);
        return 0;
    }
    pthread_mutex_lock(&pFbm->mutex);
    pFrameNode = &pFbm->pFrames[pFbmInfo->nValidBufNum];

    pFrameNode->vpicture.pData0      = pPicture->pData0;
    pFrameNode->vpicture.pData1      = pPicture->pData1;
    pFrameNode->vpicture.pData2      = pPicture->pData2;
    pFrameNode->vpicture.phyYBufAddr = pPicture->phyYBufAddr;
    pFrameNode->vpicture.phyCBufAddr = pPicture->phyCBufAddr;
    pFrameNode->vpicture.nBufId         = pPicture->nBufId;
    pFrameNode->vpicture.nBufFd      = pPicture->nBufFd;
    pFrameNode->vpicture.pPrivate    = pPicture->pPrivate;
    pFrameNode->vpicture.pMetaData   = pPicture->pMetaData;
    pFrameNode->vpicture.nLbcSize    = pPicture->nLbcSize;
    pFrameNode->vpicture.nID = pFbmInfo->nValidBufNum;
    pFrameNode->vpicture.nColorPrimary = 0xffffffff;
    #ifdef CONFIG_VE_MULTIPLE_OUTPUT
    if(pFbmInfo->pFbmBufInfo.bAfbcModeFlag==1)
    {
        pFrameNode->vpicture.pData2 = (char*)(pPicture->phyYBufAddr+ \
			  FbmComputeAfbcBufferSize(&(pFrameNode->vpicture), pFrameNode->vpicture.b10BitPicFlag));
    }
    else
    {
        pFrameNode->vpicture.pData2 = NULL;
    }
    #endif
    if(bForbidUseFlag == 0)
    {
        FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
        pFbm->nEmptyBufferNum++;
    }
    else
    {
        pFrameNode->vpicture.nID += FRM_FORBID_USE_TAG;
    }

    if(pFbmInfo->bIs3DStream==1)
    {
          pFrameNode->vpicture.phyCBufAddr = \
            pFrameNode->vpicture.phyYBufAddr+2*pFbmInfo->nMinorYBufOffset;
          pFrameNode->vpicture.pData1 = \
            pFrameNode->vpicture.pData0+2*pFbmInfo->nMinorYBufOffset;
    }
    pFbmInfo->nValidBufNum++;
    pthread_mutex_unlock(&pFbm->mutex);
    return &pFrameNode->vpicture;
}

int FbmNewDispRelease(Fbm* pFbm)
{
    if(pFbm == NULL)
        return 0;
    pthread_mutex_lock(&pFbm->mutex);
    while(1)
    {
        FrameNode* pFrameNode = FbmDequeue(&pFbm->pEmptyBufferQueue);
        if(pFrameNode == NULL)
        {
            break;
        }
        else
        {
            //* check the picture status.
            if(pFrameNode->Flag.bUsedByDecoder ||
                    pFrameNode->Flag.bInValidPictureQueue ||
                    pFrameNode->Flag.bUsedByRender ||
                    pFrameNode->Flag.bAlreadyDisplayed)
            {
                //* the picture is in the pEmptyBufferQueue, these four flags
                //* shouldn't be set.
                loge("invalid frame status, \
                       a picture is just pick out from the pEmptyBufferQueue, \
                       but bUsedByDecoder=%d, bInValidPictureQueue=%d, bUsedByRender=%d,\
                       bAlreadyDisplayed=%d.",
                       pFrameNode->Flag.bUsedByDecoder, pFrameNode->Flag.bInValidPictureQueue,
                       pFrameNode->Flag.bUsedByRender, pFrameNode->Flag.bAlreadyDisplayed);
                //abort();
                pthread_mutex_unlock(&pFbm->mutex);
                return 0;
            }
            pFbm->nEmptyBufferNum--;
            FbmEnqueue(&pFbm->pReleaseBufferQueue, pFrameNode);
            pFbm->nReleaseBufferNum++;
        }
    }
    pthread_mutex_unlock(&pFbm->mutex);
    return 0;
}

int FbmSetNewDispRelease(VideoFbmInfo* pFbmInfo)
{
    Fbm* pFbm = NULL;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bIs3DStream==1) || (pFbmInfo->pFbmSecond==NULL))
    {
        pFbm = (Fbm*)pFbmInfo->pFbmFirst;
    }
    else if(pFbmInfo->pFbmSecond != NULL)
    {
        pFbm = (Fbm*)pFbmInfo->pFbmSecond;
    }

    if(pFbm != NULL)
    {
        int index;
        pthread_mutex_lock(&pFbm->mutex);
        for(index=0; index<pFbm->nMaxFrameNum; index++)
        {
            FrameNode* pFrameNode = &pFbm->pFrames[index];
            if(pFrameNode->vpicture.nBufId != FBM_NEW_DISP_INVALID_TAG)
            {
                pFrameNode->Flag.bNeedRelease = 1;
            }
        }
        pthread_mutex_unlock(&pFbm->mutex);
        FbmFlush(pFbm);
        FbmNewDispRelease(pFbm);
    }
    return 0;
}

VideoPicture* FbmRequestReleasePicture(VideoFbmInfo* pFbmInfo)
{
    Fbm* pFbm = NULL;
    FrameNode* pFrameNode = NULL;
    //int index = 0;

    if(pFbmInfo == NULL)
        return 0;
    if((pFbmInfo->bIs3DStream==1) || (pFbmInfo->pFbmSecond==NULL))
    {
        pFbm = (Fbm*)pFbmInfo->pFbmFirst;
    }
    else if(pFbmInfo->pFbmSecond != NULL)
    {
        pFbm = (Fbm*)pFbmInfo->pFbmSecond;
    }
    if(pFbm == NULL)
    {
        loge("the handle is error:pFbm=%p\n", pFbm);
        return NULL;
    }

    pthread_mutex_lock(&pFbm->mutex);
    pFrameNode = FbmDequeue(&pFbm->pReleaseBufferQueue);
    if(pFrameNode != NULL)
    {
        pFbm->nReleaseBufferNum--;
        pFrameNode->vpicture.nID = FRM_RELEASE_TAG;
    }
    pthread_mutex_unlock(&pFbm->mutex);
    return (pFrameNode==NULL)? NULL: &(pFrameNode->vpicture);
}

VideoPicture* FbmReturnReleasePicture(VideoFbmInfo* pFbmInfo,
            VideoPicture* pVpicture, int bForbidUseFlag)
{
    Fbm* pFbm = NULL;
    FrameNode* pFrameNode = NULL;
    int index = 0;
    if(pFbmInfo == NULL)
        return 0;

    if((pFbmInfo->bIs3DStream==1) || (pFbmInfo->pFbmSecond==NULL))
    {
        pFbm = (Fbm*)pFbmInfo->pFbmFirst;
    }
    else if(pFbmInfo->pFbmSecond != NULL)
    {
        pFbm = (Fbm*)pFbmInfo->pFbmSecond;
    }
    if(pFbm == NULL)
    {
        loge("the handle is error:pFbm=%p\n", pFbm);
        return NULL;
    }
    pthread_mutex_lock(&pFbm->mutex);

    for(index=0; index<pFbm->nMaxFrameNum; index++)
    {
        pFrameNode = &pFbm->pFrames[index];
        if((pFrameNode->Flag.bNeedRelease==1)&&(pFrameNode->vpicture.nID == FRM_RELEASE_TAG))
        {
            //memcpy(&(pFrameNode->vpicture), pVpicture, sizeof(VideoPicture));
            pFrameNode->vpicture.pData0      = pVpicture->pData0;
            pFrameNode->vpicture.pData1      = pVpicture->pData1;
            pFrameNode->vpicture.pData2      = pVpicture->pData2;
            pFrameNode->vpicture.phyYBufAddr = pVpicture->phyYBufAddr;
            pFrameNode->vpicture.phyCBufAddr = pVpicture->phyCBufAddr;
            pFrameNode->vpicture.nBufId      = pVpicture->nBufId;
            pFrameNode->vpicture.nBufFd      = pVpicture->nBufFd;
            pFrameNode->vpicture.pPrivate    = pVpicture->pPrivate;
            pFrameNode->vpicture.pMetaData   = pVpicture->pMetaData;
            pFrameNode->Flag.bNeedRelease = 0;
            pFrameNode->vpicture.nID = index;
            pFrameNode->vpicture.nColorPrimary = 0xffffffff;
#ifdef CONFIG_VE_MULTIPLE_OUTPUT
            if((pFbmInfo->pFbmBufInfo.bAfbcModeFlag==1) && (pFrameNode->vpicture.pData2==NULL))
            {
                pFrameNode->vpicture.pData2 = (char*)(pFrameNode->vpicture.phyYBufAddr+ \
                          FbmComputeAfbcBufferSize(&(pFrameNode->vpicture), pFrameNode->vpicture.b10BitPicFlag));
            }
            else
            {
                pFrameNode->vpicture.pData2 = NULL;
            }
#endif
            if(bForbidUseFlag == 0)
            {
                FbmEnqueue(&pFbm->pEmptyBufferQueue, pFrameNode);
                pFbm->nEmptyBufferNum++;
            }
            else
            {
                pFrameNode->vpicture.nID += FRM_FORBID_USE_TAG;
            }
            break;
        }
    }

    VideoPicture* pPic = &pFrameNode->vpicture;

    if(index >= pFbm->nMaxFrameNum)
    {
        logw("return release picture failed, as no frameNode");
        pPic = NULL;
    }

    pthread_mutex_unlock(&pFbm->mutex);
    return pPic;
}

unsigned int FbmGetBufferOffset(Fbm* pFbm, int isYBufferFlag)
{
    VideoFbmInfo*  pFbmInfo = NULL;
    if(pFbm == NULL)
        return 0;
    pFbmInfo = (VideoFbmInfo*)pFbm->pFbmInfo;
    if(pFbmInfo == NULL)
        return 0;
    if(pFbm == pFbmInfo->pFbmFirst)
    {
        return 0;
    }
    else
    {
        return     (isYBufferFlag==1)? pFbmInfo->nMinorYBufOffset: pFbmInfo->nMinorCBufOffset;
    }
}
#endif
unsigned int FbmComputeAfbcBufferSize(VideoPicture* pPicture, int b10BitPicFlag)
{
    s32 nLower2BitBufSize = 0;
	s32 PriChromaStride = 0;
	s32 frmbufCsize = 0;
	s32 nAfbcBufSize = 0;
	s32 nTotalPicPhyBufSize = 0;
	logd("************pPicture->nWidth=%d, pPicture->nHeight=%d\n", pPicture->nWidth, pPicture->nHeight);
    nLower2BitBufSize = ((((pPicture->nWidth+3)>>2) + 31) & 0xffffffe0) * pPicture->nHeight * 3/2;
    PriChromaStride = ((pPicture->nWidth/2) + 31)&0xffffffe0;
    frmbufCsize = 2 * (PriChromaStride * (((pPicture->nHeight/2)+15)&0xfffffff0)/4);
    if(b10BitPicFlag)
    {
    	nAfbcBufSize = ((pPicture->nWidth+15)>>4) * ((pPicture->nHeight+4+15)>>4) * (512 + 16) + 32 + 1024;
        nTotalPicPhyBufSize = nAfbcBufSize + nLower2BitBufSize;
    }
    else
    {
    	nAfbcBufSize = ((pPicture->nWidth+15)>>4) * ((pPicture->nHeight+4+15)>>4) * (384 + 16) + 32 + 1024;
        nTotalPicPhyBufSize = nAfbcBufSize;
    }
	nTotalPicPhyBufSize = (nTotalPicPhyBufSize+1023)&~1023;
	logd("*********nTotalPicPhyBufSize=%d\n", nTotalPicPhyBufSize);
	return nTotalPicPhyBufSize;
}
