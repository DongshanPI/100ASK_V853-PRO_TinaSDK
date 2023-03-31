PKG_NAME:=ge8300-km
#GPU_BUILD_DIR:=$(LINUX_DIR)/modules/gpu/img-rgx/linux/rogue_km
GPU_BUILD_DIR:=$(LINUX_DIR)/modules/gpu/
ifeq ($(ARCH), arm)
GPU_KO_DIR:=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_release/target_armv7-a/pvrsrvkm.ko
GPU_KO_DIR+=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_release/target_armv7-a/dc_sunxi.ko
else
#GPU_KO_DIR:=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_release/target_aarch64/pvrsrvkm.ko
#GPU_KO_DIR+=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_release/target_aarch64/dc_sunxi.ko
GPU_KO_DIR:=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_nullws_release/target_aarch64/pvrsrvkm.ko
GPU_KO_DIR+=$(GPU_BUILD_DIR)/img-rgx/linux/rogue_km/binary_sunxi_linux_nullws_release/target_aarch64/dc_sunxi.ko
endif