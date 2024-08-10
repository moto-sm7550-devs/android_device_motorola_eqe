#
# Copyright (C) 2022-2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

DEVICE_PATH := device/motorola/eqe

# Inherit from motorola sm7550-common
include device/motorola/sm7550-common/BoardConfigCommon.mk

# Bootloader
TARGET_BOOTLOADER_BOARD_NAME := eqe

# Display
TARGET_SCREEN_DENSITY := 400

# HIDL
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += $(DEVICE_PATH)/device_framework_matrix_eqe.xml
DEVICE_MANIFEST_CROW_FILES += $(DEVICE_PATH)/manifest_eqe.xml
ODM_MANIFEST_SKUS += dn dne
ODM_MANIFEST_DN_FILES := $(DEVICE_PATH)/manifest_eqe_dn.xml
ODM_MANIFEST_DNE_FILES := $(DEVICE_PATH)/manifest_eqe_dne.xml

# Kernel
TARGET_KERNEL_CONFIG += \
	vendor/ext_config/moto-kalama-eqe.config

# Kernel Modules
BOARD_SYSTEM_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/modules.load.system_dlkm))
BOARD_VENDOR_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/modules.load))
BOARD_VENDOR_KERNEL_MODULES_BLOCKLIST_FILE := $(DEVICE_PATH)/modules.blocklist
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/modules.load.vendor_boot))
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_BLOCKLIST_FILE := $(DEVICE_PATH)/modules.blocklist.vendor_boot
BOARD_VENDOR_RAMDISK_RECOVERY_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/modules.load.recovery))
BOOT_KERNEL_MODULES := $(strip $(shell cat $(DEVICE_PATH)/modules.load.recovery $(DEVICE_PATH)/modules.load.vendor_ramdisk))
SYSTEM_KERNEL_MODULES := $(strip $(shell cat $(DEVICE_PATH)/modules.load.system_dlkm))

TARGET_KERNEL_EXT_MODULES := \
  qcom/opensource/mmrm-driver \
  qcom/opensource/mm-drivers/hw_fence \
  qcom/opensource/mm-drivers/msm_ext_display \
  qcom/opensource/mm-drivers/sync_fence \
  qcom/opensource/audio-kernel \
  qcom/opensource/camera-kernel \
  qcom/opensource/dataipa/drivers/platform/msm \
  qcom/opensource/datarmnet/core \
  qcom/opensource/datarmnet-ext/aps \
  qcom/opensource/datarmnet-ext/offload \
  qcom/opensource/datarmnet-ext/shs \
  qcom/opensource/datarmnet-ext/perf \
  qcom/opensource/datarmnet-ext/perf_tether \
  qcom/opensource/datarmnet-ext/sch \
  qcom/opensource/datarmnet-ext/wlan \
  qcom/opensource/securemsm-kernel \
  qcom/opensource/display-drivers/msm \
  qcom/opensource/eva-kernel \
  qcom/opensource/video-driver \
  qcom/opensource/graphics-kernel \
  qcom/opensource/wlan/platform \
  qcom/opensource/wlan/qcacld-3.0/.kiwi_v2 \
  qcom/opensource/bt-kernel \
  nxp/opensource/driver \
  motorola/drivers/mmi_annotate \
  motorola/drivers/mmi_info \
  motorola/drivers/power/bm_adsp_ulog \
  motorola/drivers/power/mmi_charger \
  motorola/drivers/power/qti_glink_charger \
  motorola/drivers/power/qpnp_adaptive_charge \
  motorola/drivers/misc/utag \
  motorola/drivers/mmi_relay \
  motorola/drivers/power/mmi_lpd_mitigate \
  motorola/drivers/moto_f_usbnet \
  motorola/drivers/misc/mmi_sys_temp \
  motorola/drivers/power/wakeup_sources \
  motorola/drivers/watchdogtest \
  motorola/drivers/regulator/wl2868c \
  motorola/drivers/sensors \
  motorola/drivers/misc/sx937x_multi \
  motorola/drivers/input/touchscreen/touchscreen_mmi \
  motorola/drivers/input/touchscreen/goodix_berlin_mmi \
  motorola/drivers/input/misc/goodix_fod_mmi \
  motorola/drivers/moto_mm \
  motorola/drivers/moto_mmap_fault \
  motorola/drivers/moto_swap

# Partitions
ifneq ($(WITH_GMS),true)
BOARD_PRODUCTIMAGE_PARTITION_RESERVED_SIZE := 3318226944
BOARD_SYSTEMIMAGE_PARTITION_RESERVED_SIZE := 873680896
BOARD_SYSTEM_EXTIMAGE_PARTITION_RESERVED_SIZE := 916299776
endif
BOARD_MOT_DP_GROUP_SIZE := 10867441664 # ( BOARD_SUPER_PARTITION_SIZE - 4MB )
BOARD_SUPER_PARTITION_SIZE := 10871635968

# Properties
TARGET_PRODUCT_PROP += $(DEVICE_PATH)/product.prop
TARGET_SYSTEM_PROP += $(DEVICE_PATH)/system.prop
TARGET_VENDOR_PROP += $(DEVICE_PATH)/vendor.prop

# Recovery
TARGET_RECOVERY_UI_MARGIN_HEIGHT := 90

# Security
VENDOR_SECURITY_PATCH := 2024-05-01

# inherit from the proprietary version
include vendor/motorola/eqe/BoardConfigVendor.mk
