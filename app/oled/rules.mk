LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/console

MODULE_SRCS += \
	$(LOCAL_DIR)/oled.c \
	$(LOCAL_DIR)/DEV_Config.c \
	$(LOCAL_DIR)/OLED_Driver.c \

include make/module.mk
