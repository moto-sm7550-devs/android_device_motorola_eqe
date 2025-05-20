#
# Copyright (C) 2025 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
TARGET_SUPPORTS_OMX_SERVICE := false
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from eqe device
$(call inherit-product, device/motorola/eqe/device.mk)

# Inherit some common 2by2 stuff.
$(call inherit-product, vendor/2by2/config/common_full_phone.mk)

PRODUCT_NAME := eqe
PRODUCT_DEVICE := eqe
PRODUCT_MANUFACTURER := motorola
PRODUCT_BRAND := motorola
PRODUCT_MODEL := motorola edge 50 pro

CUSTOM_PROCESSOR_INFO := Qualcomm Snapdragon 7 Gen 3

PRODUCT_GMS_CLIENTID_BASE := android-motorola

PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="eqe_g-user 15 V1UMS35H.10-67-3-1 f758d4 release-keys" \
    BuildFingerprint=motorola/eqe_g/eqe:15/V1UMS35H.10-67-3-1/f758d4:user/release-keys \
    DeviceProduct=eqe_g
