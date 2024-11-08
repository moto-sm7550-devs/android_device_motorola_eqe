#
# Copyright (C) 2022-2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit from eqe device
$(call inherit-product, device/motorola/eqe/device.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

PRODUCT_NAME := lineage_eqe
PRODUCT_DEVICE := eqe
PRODUCT_MANUFACTURER := motorola
PRODUCT_BRAND := motorola
PRODUCT_MODEL := motorola edge 50 pro

PRODUCT_GMS_CLIENTID_BASE := android-motorola

PRODUCT_BUILD_PROP_OVERRIDES += \
    DeviceProduct=eqe_g \
    BuildDesc=$(call normalize-path-list, "eqe_g-user 14 U2UMS34.27-38-1-2-3-1 76e47-863bd0 release-keys")

BUILD_FINGERPRINT := motorola/eqe_g/eqe:14/U2UMS34.27-38-1-2-3-1/76e47-863bd0:user/release-keys
