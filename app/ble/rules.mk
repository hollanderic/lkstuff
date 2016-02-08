LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/blueapp.c \

MODULE_ARM_OVERRIDE_SRCS := \

MODULE_COMPILEFLAGS += -Wno-format

MODULE_DEPS += \
    lib/ble \


include make/module.mk
