LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/console

MODULE_SRCS += \
	$(LOCAL_DIR)/ledz.c

include make/module.mk
