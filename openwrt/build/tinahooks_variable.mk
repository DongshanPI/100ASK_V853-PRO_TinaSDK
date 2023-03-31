

ifeq ($(and $(LICHEE_TOP_DIR), $(LICHEE_IC), $(LICHEE_BOARD)),)
$(warning  "LICHEE_TOP_DIR: $(LICHEE_TOPDIR)")
$(warning  "LICHEE_IC: $(LICHEE_IC)")
$(warning  "LICHEE_BOARD: $(LICHEE_BOARD)")
$(error "There must be at least one empty string!")

else

# out
TARGET_OUT_DIR:=$(LICHEE_TOP_DIR)/out/$(LICHEE_IC)/$(LICHEE_BOARD)/openwrt
# target/linux
TINA_TARGET_LINUX_DIR:=$(LICHEE_TOP_DIR)/openwrt/target/$(LICHEE_IC)
# defconfig
TINA_TARGET_DEFCONFIG:=$(LICHEE_TOP_DIR)/openwrt/target/$(LICHEE_IC)/$(LICHEE_IC)-$(LICHEE_BOARD)/defconfig

TARGET_EXTERNAL_PATH:=$(LICHEE_TOP_DIR)/prebuilt/hostbuilt/make4.1/bin:$(LICHEE_TOP_DIR)/prebuilt/hostbuilt/python3.8/bin
endif

