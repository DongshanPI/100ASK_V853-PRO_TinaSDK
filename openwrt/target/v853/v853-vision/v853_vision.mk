$(call inherit-product-if-exists, target/allwinner/v853-common/v853-common.mk)

PRODUCT_PACKAGES +=

PRODUCT_COPY_FILES +=

PRODUCT_AAPT_CONFIG := large xlarge hdpi xhdpi
PRODUCT_AAPT_PERF_CONFIG := xhdpi
PRODUCT_CHARACTERISTICS := musicbox

PRODUCT_BRAND := allwinner
PRODUCT_NAME := v853_vision
PRODUCT_DEVICE := v853-vision
PRODUCT_MODEL := Allwinner v853 vision board
