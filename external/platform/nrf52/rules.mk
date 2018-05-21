LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/system_nrf52.c \
    $(LOCAL_DIR)/nrf_drv_usbd_errata.c \

include make/module.mk

