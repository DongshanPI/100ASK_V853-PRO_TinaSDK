
һ��linux���˵��:

1. ÿ��linux so��Ӧ�ı��빤�������£�

so_dir: arm-linux-gnueabi             host: arm-linux-gnueabi      toolchain_dir: external-toolchain
so_dir: arm-linux-gnueabihf           host: arm-linux-gnueabihf    toolchain_dir: ϵͳ�Դ�
so_dir: arm-none-linux-gnueabi        host: arm-none-linux-gnueabi toolchain_dir: ϵͳ�Դ�
so_dir: arm-none-linux-gnueabi        host: arm-none-linux-gnueabi toolchain_dir: ϵͳ�Դ�
so_dir: toolchain-sunxi-aarch64-glibc host: aarch64-openwrt-linux  toolchain_dir: tina_toolchain
so_dir: toolchain-sunxi-aarch64-musl  host: aarch64-openwrt-musl   toolchain_dir: tina_toolchain
so_dir: toolchain-sunxi-arm9-glibc    host: arm-openwrt-linux-gnueabi     toolchain_dir: tina_toolchain
so_dir: toolchain-sunxi-arm9-musl     host: arm-openwrt-linux-muslgnueabi toolchain_dir: tina_toolchain
so_dir: toolchain-sunxi-arm-glibc     host: arm-openwrt-linux-gnueabi     toolchain_dir: tina_toolchain
so_dir: toolchain-sunxi-arm-musl      host: arm-openwrt-linux-muslgnueabi toolchain_dir: tina_toolchain

2.���벽�����£�arm-linux-gnueabi��Ϊ������

2.1 export���빤������
     TOOLS_CHAIN=/home/user/workspace/tools_chain/
     export PATH=${TOOLS_CHAIN}/arm-linux-gnueabi/bin:$PATH

2.2. ����automake����ع��ߣ�./bootstrap

2.3. ����makefile��
2.3.1 ģʽ��./configure --prefix=INSTALL_PATH --host=HOST_NAME LDFLAGS="-LSO_PATH"
2.3.2 ʾ����./configure --prefix=/home/user/workspace/libcedarc/install --host=arm-linux LDFLAGS="-L/home/user/workspace/libcedarc/lib/arm-linux-gnueabi"
2.3.3 �ر�˵��������ں��õ���linux3.10, ��������flag��CFLAGS="-DCONF_KERNEL_VERSION_3_10" CPPFLAGS="-DCONF_KERNEL_VERSION_3_10"
      ����./configure --prefix=/home/user/workspace/libcedarc/install --host=arm-linux CFLAGS="-DCONF_KERNEL_VERSION_3_10" CPPFLAGS="-DCONF_KERNEL_VERSION_3_10" LDFLAGS="-L/home/user/workspace/libcedarc/lib/arm-linux-gnueabi"

2.4 ���룺make ; make install

�����汾�Ķ�˵����

13) CdarC-v1.3.0

1. �ر�˵����
1.1 �Ӵ˰汾��ʼ���汾�Ķ�˵�����õ���ʽ��

1.2 cedarc�ڲ�����ģ���������ԣ���Cedarc֧�ֵİ汾���ޣ���Ҫ��ͼ����ͬ�汾�Ĳ���ģ�����۵�һ����룬
Ҳ��Ҫ��ͼ����ͬ�汾��ģ�鶯̬��ֱ�����۵�һ�����С�cedarcֻ֧�ֱ�����ݣ���֧��ֱ�����м��ݡ�

1.3 ������˵����
a.����AndroidQ(2020-05֮��汾)��AndroidR�������Ͼɰ汾�Ѳ�֧�֡��Ӵ˰汾��ʼ������android��ϲ���
andorideabi_32������Ҫ64λ�⣬����������
b.��������Linux��������������������������ϵά����Ա��

1.4 ��ֲ��˵����
Ŀǰʵ�ֽӿڻ���ģ���������⣬memory��ve��������filesinkҲ�ǽӿڻ��ģ���������ֲ�ԱȽϸߡ�
������ģ�����϶Ȼ���̫���ˣ�����7��ģ���໥��������������󽵵�����ֲ�ԣ������汾ϣ�����Ż���

2. �汾�Ķ���
2.1 ֧��Linux5.4�����ں�ion�ӿڷ����仯����ȡ�����ַ����ͳһ��VE������ɡ�
2.2 ֧��AW1860����оƬVEģ���������scale���ܣ�����scale����,AV1Ӳ�����빦�ܡ�
2.3 ֧��ION����,��AOSP�е�libion����ֲ������������openmaxӦ��ʹ����ֲ��Ľӿڡ�
2.4 ��vdecoder.c�ж���bitstream��picture���ļ����湦�ܣ�֧��MD5ֵ��
2.5 ����aftertreatmentģ�飬���ں���ͳһ�ӿڣ�Ŀǰʵ�ֵĺ���ģ��������scale��
2.6 ����mem��ops��ve��ops���û����贫�룬��vdecoder.cģ�鴴���������ݸ���ģ��ʹ�á�

1). CedarC-v1.0.4

1. �ر�˵����

1.1 cameraģ����أ�( >=android7.0��ƽ̨��Ҫ���������޸�,����ƽ̨����֮ǰ������)
    �޸ĵ�ԭ��a.omx��android framework��û�ж�NV21��NV12������ͼ���ʽ����ϸ�֣�������OMX_COLOR_FormatYUV420SemiPlanar
                ���б�ʾ��OMX_COLOR_FormatYUV420SemiPlanar�����Ա�ʾNV21��Ҳ���Ա�ʾNV12��
                b.��ACodec��OMX_COLOR_FormatYUV420SemiPlanar����NV12��camera��OMX_COLOR_FormatYUV420SemiPlanar����NV21��
                ֮ǰ��������omx_vencͨ�����̵İ����������ּ��ݣ�����������ACodec��������NV12������������camera��������
                NV21��
                c.android7.0��ΪȨ�޹����ԭ���޷���ȡ�����̵İ����������޷���omx_venc���м��ݣ�ֻ�����ϲ�caller�����
                ���ݣ���cts���ACodec�Ľӿڽ��в��ԣ����Ķ�ACodec�������Ӧ��cts���ԣ�����ֻ���޸�camera
                d.�޸ĵ�ԭ��Ϊ����չͼ���ʽ��ö�����͵ĳ�Ա����OMX_COLOR_FormatYVU420SemiPlanar�����ڱ�ʾNV21��
                  �� OMX_COLOR_FormatYUV420SemiPlanar --> NV12
                     OMX_COLOR_FormatYVU420SemiPlanar --> NV21

    �޸ĵĵط���
    a. ͬ��ͷ�ļ���ͬ��openmax/omxcore/inc/OMX_IVCommon.h ��./native/include/media/openmax/OMX_IVCommon.h
    b. cameraģ���ڵ���openmax/venc����ӿ�ʱ��Ҫ�����޸ģ�
       �޸�ǰ NV21 --> OMX_COLOR_FormatYUV420SemiPlanar��
       �޸ĺ� NV21 --> OMX_COLOR_FormatYVU420SemiPlanar;

1.2 ���ؽ�����˵����
    ֮ǰ���ؽ����Ĳ�����vdecoder.c��һ����У�Ϊ���������ԣ���v1.0.4��
�Ѽ��ؽ����Ĳ����ŵ��ϲ�����߽��У��ϲ�ɵ���vdecoder.c��AddVDPlugin�ӿ�Ĭ��
�������е��ӽ��룬���߲������´��밴����أ�

static void InitVDLib(const char *lib)
{
    void *libFd = NULL;
    if(lib == NULL)
    {
        loge(" open lib == NULL ");
        return;
    }

    libFd = dlopen(lib, RTLD_NOW);

    VDPluginFun *PluginInit = NULL;

    if (libFd == NULL)
    {
        loge("dlopen '%s' fail: %s", lib, dlerror());
        return ;
    }

    PluginInit = (VDPluginFun*)dlsym(libFd, "CedarPluginVDInit");
    if (PluginInit == NULL)
    {
        logw("Invalid plugin, CedarPluginVDInit not found.");
        return;
    }
    logd("vdecoder open lib: %s", lib);
    PluginInit(); /* init plugin */
    return ;
}

static void AddVDLib(void)
{
    InitVDLib("/system/lib/libawh265.so");
    InitVDLib("/system/lib/libawh265soft.so");
    InitVDLib("/system/lib/libawh264.so");
    InitVDLib("/system/lib/libawmjpeg.so");
    InitVDLib("/system/lib/libawmjpegplus.so");
    InitVDLib("/system/lib/libawmpeg2.so");
    InitVDLib("/system/lib/libawmpeg4base.so");
    InitVDLib("/system/lib/libawmpeg4normal.so");
    InitVDLib("/system/lib/libawmpeg4vp6.so");
    InitVDLib("/system/lib/libawmpeg4dx.so");
    InitVDLib("/system/lib/libawmpeg4h263.so");
    InitVDLib("/system/lib/libawvp8.so");

    return;
}

2. �Ķ������£�
2.1 openmax:venc add p_skip interface
2.2 h265:fix the HevcDecodeNalSps and HevcInitialFBM
2.3 h264:refactor the H264ComputeOffset
2.4 mjpeg scale+rotate������
2.5 vdcoder/h265: add the code of parser HDR info
2.6 vdecoder/h265: add the process of error-frame
2.7 vdecoder/h264: make sure pMbNeighborInfoBuf is 16K-align to fix mbaff function
2.8 openmax/vdec: remove cts-flag
2.9 openmax/venc: remove cts-flag
2.10 detection a complete frame bitstream and crop the stuff zero data
2.11 vdecoder/h265:fix the bug that the pts of keyFrame is error when seek
2.12 openmax/inc: adjust the define of struct
2.13 vencoder: add lock for vencoderOpen()
2.14 vdecoder/ALMOST decoders:fix rotate and scaledown
2.15 vdecoder/h265:fix the process of searching the start-code when sbm cycles
2.16 vdecoder/h265:fix the bug when request fbm fail after fbm initial
2.17 vdecoder/h265:improve the process when poc is abnormal
2.18 cedarc: unify the release of android and linux
2.19 vdecoder/avs: make mbInfoBuf to 16K align

2).CedarC-v1.0.5

1. �Ķ������£�
1.1.configure.ac:fix the config for linux compiling
1.2.openmax/venc: revert mIsFromCts on less than android_7.0 platfrom
1.3.vdecoder/h265soft:make h265soft be compatible with AndroidN
1.4.cedarc: merge the submit of cedarc-release
1.5.vdecoder/h265:use the flag "bSkipBFrameIfDelay"
1.6.vdecoder:fix the buffer size for thumbnail mode
1.7.cedarc: fix compile error of linux
1.8.omx:venc add fence for androidN
1.9.openmax:fix some erros
1.10.omx_venc: add yvu420sp for omx_venc init
1.11.videoengine:add 2k limit for h2
1.12.cedarc: add the toolschain of arm-openwrt-linux-muslgnueabi for v5
1.13.cedarc: ���0x1663 mpeg4 ���Ż���������
1.14.vdecoder: fix compile error of soft decoder for A83t
1.15.omx_vdec: fix the -1 bug for cts
1.16.�޸�mpeg2 ��ȡve version�ķ�ʽ
1.17.vencoder: fix for input addrPhy check
1.18.cedarc: merge the submit of cedarc-release

3). CedarC-v1.1

1. �ر�˵����
1.1 v1.1������H6-dev������֧���޸ĳɹ���������h265 10bit��afbc���ܣ��˹�����
    cedarxƥ���޸ģ������޷�������Ч��
1.2 v1.1 �ر��������������������ڴ��memory�ӿڣ��ϲ�����Ҫ���������ڴ棬��ģ��
    �ڲ�ʵ��memory�Ľӿڻ���������ӿڣ�
1.3 ������VideoDecoderGetVeIommuAddr��VideoDecoderFreeVeIommuAddr�������ӿڣ�����
    ��iommu��buffer���а����󶨲�����

2. �Ķ������£�
2.1 ����h265 10bit��afbc���ܵ�֧�֣�
2.2 ����vp9 Ӳ������������
2.3 ���Ӷ�iommu buffer�����֧�֣�
2.4 ��veģ��������ع���
2.5 ��sbmģ��������ع���
2.6 memory�ӿڲ��ٶ��⿪�š�

4). CedarC-v1.1.1

1. �Ķ������£�
1.1 ����cedarc-release��Ŀǰrelease������cedarc������������android��linxuƽ̨��
1.2 vdecoder/Vp8:process the case of showFrm
1.3 vdecoder/h265: increase the size of HEVC_LOCAL_SBM_BUF_SIZE
1.4 vdecoder/videoengine: add the function of checkAlignStride
1.5 vdecoder/h265: set proc info
1.6 vdecoder/sbm: �޸�H265 sbmFrame ����Խ���bug

5). CedarC-v1.1.2

1. �Ķ������£�
1.1 vdecoder/h264: set proc info
1.2 vdecoder/mjpge: set proc info
1.3 vdecoder: improve function of savePicture
1.4 vencoder: fix for jpeg get phy_addr and androidN get chroma addr
1.5 ��fbmInfo�����offset����Ϣ
1.6 openmax/vdec: set mCropEnable to false on linux
1.7 �������Լ��ķ�ʽ��ȷ���ڴ��ʹ�÷�ʽ
1.8 unmap the fbm buffer when native window changed
1.9 vp8 return the alterframe error for '���ɺ�����.mkv'

6). CedarC-v1.1.3
1.1 vdecoder: change the 6k range
1.2 openmax:venc: fix for recorder
1.3 vdecoder: fix the bug for initializeVideoDecoder fails
1.4 vdecoder/h265Soft: fix the bug: crash when seek the video H265_test.mkv
1.5 ve: control phyOffset in ve module
1.6 use iomem.type to check iommu mode
1.7 vdecoder: add nBufFd when call FbmReturnReleasePicture
1.8 openmax: load the libawwmv3.so when init
1.9 vdecoder/VP9:reset some parameter for Vp9HwdecoderReset()
1.10 ve: dynamic set dram high channal

7). CedarC-v1.1.4
1.1 vencoder: add for thumb write back func
1.2 vencoder:fix for set thumb write back scaler dynamic
1.3 vencoder: fix for only thumb write back no encode
1.4 �޸ļ��Ӳ��busy��״̬λ�ĵȴ�ʱ��
1.5 vdecoder/sbmFrame: fix the error video 720P_V_.HEVC_AAC_2M_15F.mkv

8). CedarC-v1.1.6

1. �Ķ������£�
1.1 vdecoder/h264:after reset,the first frame pts is same to the last bitstream
1.2 h265:fix the bug of parse-extradata
1.3 �޸�H264 ������pts�쳣��bug
1.4 vdecoder: add lock for VideoEngineCreate
1.5 fix gts test fail
1.6 vdecoder/vc1: fix the bug: error when seek
1.7 ����H264����ܹ���������һ֡û�н����bug
1.8 openmax/vdec: not support metadata buffer
1.9 ve: fix for getIcVersion when other process is reseting ve
1.10 openmax/vdec: open mem-ops when use
1.11 vdecoder/h264/distinguish SbmReturnStream of stream and frame for resolution change
1.12 omx:venc: fix for recorder of h6-kk44-iommu
1.13 openmax/vdec: support afbc function
1.14 h264:fix the progress of erro
1.15 openmax/vdec: plus timeout to 5s

2.�ر�˵��:
2.1 mediacodecͨ·��afbc���ܵ�֧��
    afbc���ܵ�֧���漰������ģ�飬��һ��cedarc/openmaxģ����޸ģ���ģ����޸�����ɣ���
 cedarc�Ĵ�����µ�cedarc-v1.1.6����°汾���ɣ�
    �����framework����޸ģ�Ŀǰframework���patchֻ���ɵ�H6�ķ����ϣ�����������Ҫ����afbc
 �Ĺ��ܣ�����AL3����ϲ���ṩframework_patch.
 
9). CedarC-v1.1.7

1. �Ķ������£�
1.1. vdecoder/avs:the case of diff pts > 2s for TvStream
1.2. vencoder:jpeg fix for exif buffer memory leak
1.3. h264:fix the bug of frameStream-end
1.4. vdecoder: fix the bug when get memops
1.5. vdecoder/h264:u16 to s32
1.6. vdecoder: add iptv-info for h264 and h265
1.7. vdecoder/fbm: avoid memory leak
1.8. ���H8 �����.mp4���Ż���������
1.9. vdecoder/sbmH264: surpport secure video
1.10. ����H264�ķֱ���������Dram buffer �Ĵ�С
1.11. ����surface �л�ʱ������A026.mpeg4��ס������
1.12. secure ģʽ��֧��iommu
1.13. H265 ��ve Ƶ����H6�ϵ���Ϊ696MHZ
1.14. memory: fix for get phy_addr 0 when is iommu mode
1.15. H264 ��������ͷ��Ϣ�Ĵ���
1.16. openmax/vdec: increase the input-buffer-size to 6 MB
1.17. vdecoder: optimize the policy of set vefreq
1.18. ��mpeg2����������Ӵ���֡��ʶ��
1.19. omx:venc: fix for gpu buf
1.20. vdecoder/avs/fix the flag of bIsProgressive
1.21. openmax/vdec: support native-handle
1.22. ���ve Ƶ��������
1.23. vdecoder: not get the veLock with the softDecoder case

10). CedarC-v1.1.7 -- patch-002

1. �Ķ������£�
1.1. �޸�H264����dram buffer�ķ�ʽ
1.2. �޸�T3 mpeg4v2�Ľ��뷽ʽ,�˸�ʽVE��֧��Ӳ��
1.3. ����widewine ģʽ��extradata �Ĵ���
1.4. vdecoder/H264/decoder one frame then return
1.5. ����parser����width��height��case
1.6. openmax/vdec: add the policy of LIMIT_STREAM_NUM
1.7. videoengine:fix the specificdat value
1.8. videoengine: fix the ve unlock in VideoEngineReopen
1.9. openmax/vdec: add the function of di
1.10. vdecoder/H264:resolution change for online video
1.11. ����û�л�ȡ��fbm ��Ϣʱ,�������ͽ��յ�eos��Ƕ����������˳���bug
1.12. openmax/vdec: remove bCalledByOmxFlag
1.13. h265: fix the bug decoding the slcieRps as numOfRefs is outoff range
1.14. openmax/vdec: not init decoder in the status of idle

10). CedarC-v1.1.7 -- patch-003

1. �Ķ������£�
1.1 �޸�h264 cts failed
1.2 openmax/vdec: reset MAX_NUM_OUT_BUFFERS from 15 to 4 as it consume too much buffer
1.3 h265: fix the pts of eos frame for gts
1.4 fbm: fix the value of pMetaData
1.5 �޸�mpeg2 pt2.vob ������bug

10). CedarC-v1.1.7 -- patch-004

1. �Ķ������£�
1.1 openmax/vdec: fix the process of decoding the last frame which size changes
1.2 h265: fix the bug of decoding extraData
1.3 fbm: add code for allocating metadata buffer for linux
1.4 di not support 4K stream, for 4K interlace stream, ve does scaledown
1.5 vdecoder: limit nVbvBufferSize to [1 MB, 32MB]
1.6 openmax/vdec: increase OMX_VIDEO_DEC_INPUT_BUFFER_SIZE_SECURE from 256 KB to 1 MB
1.7 �޸�H264��֡���µ�pts ������������
1.8 vdecoder:add the decIpVersion for T7
1.9 vdecoder/h264:add reset parameters of H264ResetDecoderParams()
1.10 openMAX: Adapt DI process with two input di pictures to the platform of H3
1.11 openmax/vdec: just set nv21 format in di-function case
1.12 demo: add vencDem to cedarc
1.13 support the field structure of vc1 frame packed mode
1.14 demo:demoVencoder: fix for style error    
1.15 config: add config file of T7 platform

11). CedarC-v1.1.8

1. �Ķ������£�
1.1. ����mp4normal ����ͼģʽ��û��specialdataʱ,�޷������bug
1.2. ����mpeg4Normal ������ͼ��bug
1.3. ֧��mjpeg444�Ľ���
1.4. vdecoder: fix the bug when sbm inits failed
1.5. vdecoder:fix for VC1. Be compatible to 64 bit system
1.6. �޸�0x1663����mpeg4�ļ�����������
1.7. cdcUtil: fix ion handle for linux4.4
1.8. �ָ�vbv buffer size �����÷�ʽ
1.9. omx:venc: fix for h265 enc error
1.10. vdecoder:A63 upgrade,Mpeg1/2/4 addr register
1.11. vdecoder:catch DDR value for H265
1.12. ��������05_100M.ts ��Ƶpts ���������
1.13. cedarc:avs:add support for sun8iw7p1
1.14. 1708 оƬ��mjpeg ����mjpegPlus
1.15. vdecoder/h265:protecting nFrameDuration
1.16. vdecoder/sbm: fix bug: crash when length of stream is 0
1.17. ���andoido ��֧��
1.18. �޸�H265�Ķ�֡����
1.19. cedarc:avs_plus:fix avs_plus unsupport error on chip-1680
1.20. cedarc/log: dynamic show log by property_get
1.21. cedarc: compile so in system/lib/ and system/vendor/lib
1.22. vdecoder/fbm: reduce frmbuf_c_size of afbc 

11). CedarC-v1.1.9

1. �Ķ������£�

1.1 �޸�linuxĿ¼�����ƣ�

    �޸�ǰ                                 �޸ĺ�
arm-linux-gnueabi                    arm-linux-gnueabi
arm-linux-gnueabihf                  arm-linux-gnueabihf
arm-none-linux-gnueabi               arm-none-linux-gnueabi
arm-linux-gnueabihf-linaro           toolchain-sunxi-aarch64-glibc
arm926-uclibc                        toolchain-sunxi-aarch64-musl
arm-aarch64-openwrt-linux            toolchain-sunxi-arm9-glibc
arm-openwrt-linux-muslgnueabi        toolchain-sunxi-arm9-musl
arm-openwrt-linux-muslgnueabi-v5     toolchain-sunxi-arm-glibc
arm-openwrt-linux-uclibc             toolchain-sunxi-arm-musl

˵�����޸�ԭ��Ϊ������BU1�����ṩ�Ĺ�������������
      ǰ��3��Ϊ�õñȽ϶�ı��빤�����������ڸ���BU�ķ���������
      ����6��ΪBU1�ṩ�Ĺ��������������so���������BU��Ҫʹ�ã���������cedarc�������ͬ�£�����ϲ��/��С�ࣩ���й�ͨ�˽⣻

1.2 �޸�linux so���ƣ�
 
    �޸�ǰ                            �޸ĺ�
libcdc_vd_avs.so                   libawavs.so
libcdc_vd_h264.so                  libawh264.so
libcdc_vd_h265.so                  libawh265.so
libcdc_vd_mjpeg.so                 libawmjpeg.so
libcdc_vd_mjpegs.so                libawmjpegplus.so
libcdc_vd_mpeg2.so                 libawmpeg2.so
libcdc_vd_mpeg4base.so             libawmpeg4base.so
libcdc_vd_mpeg4dx.so               libawmpeg4dx.so
libcdc_vd_mpeg4h263.so             libawmpeg4h263.so
libcdc_vd_mpeg4normal.so           libawmpeg4normal.so
libcdc_vd_mpeg4vp6.so              libawmpeg4vp6.so
libcdc_vd_vp8.so                   libawvp8.so
libcdc_vd_vp9Hw.so                 libawvp9Hw.so
libcdc_vd_wmv3.so                  libawwmv3.so
libcdc_vencoder.so                 libvencoder.so
libcdc_ve.so                       libVE.so
libcdc_videoengine.so              libvideoengine.so

˵�����޸�linux so�����Ƶ�ԭ����Ҫ��Ϊ���� androidƽ̨��so����һ�¡�

12). CedarC-v1.1.10

1. �Ķ������£�
1.1 ֧�� android P ϵͳ��
1.2 ���� android P gms���ԣ�
1.3 �����ع����openmaxģ�飻
1.4 fix bugs��

2. �ر�˵����

2.1 ���������Ľӿڲ������п�Դ:
    ��Դ��������encoderģ������һ��so: libvendoer.so;
    ��Դ����encoderģ��ָ��������ģ�飺libvendoer.so / libvenc_base.so / libvenc_common.so / libvenc_h264.so / libvenc_h265.so / libvenc_jpeg.so;
          libvendoer.so / libvenc_base.so��������ģ����п�Դ��libvenc_common.so / libvenc_h264.so / libvenc_h265.so / libvenc_jpeg.soΪ��Դ���֡�
          
����ʹ�÷����ĸı䣺��Դʱ���ñ�����ֻ������һ��so���ɣ���Դ����Ҫ��������so .
12). CedarC-v1.2.0
1. �Ķ������£�
1.1 ���CdcMalloc.c�����ڼ��malloc���ڴ��Ƿ����й©����
1.2 ���sysfs���ƣ���ͨ��cat ve�Ľڵ�ʵʱ�鿴sbm/fbm/vdecoder/�ڴ����Ϣ��
1.3 ��Ӷ�̬debug���ƣ�ͨ���޸������ļ�cedarc.confʵ��(�޸������ļ�����Ҫkill����صĽ�����mediaserver�Ż���Ч)��
1.4 ������������isp�Ĺ��ܽӿڣ�ʹ���߿ɵ�������VideoEncIspFunction�ӿڣ�
1.5 fix bugs��
