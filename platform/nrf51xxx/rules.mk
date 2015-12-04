LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x0
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m3

ifeq ($(NRF51_CHIP),nrf51822-qfaa)
GLOBAL_DEFINES +=
MEMSIZE ?= 16384
endif
ifeq ($(NRF51_CHIP),nrf51822-ceaa)
GLOBAL_DEFINES +=
MEMSIZE ?= 16384
endif
ifeq ($(NRF51_CHIP),nrf51822-qfab)
GLOBAL_DEFINES +=
MEMSIZE ?= 16384
endif
ifeq ($(NRF51_CHIP),nrf51822-cdab)
GLOBAL_DEFINES +=
MEMSIZE ?= 16384
endif
ifeq ($(NRF51_CHIP),nrf51822-qfac)
GLOBAL_DEFINES +=
MEMSIZE ?= 32768
endif
ifeq ($(NRF51_CHIP),nrf51822-cfac)
GLOBAL_DEFINES +=
MEMSIZE ?= 32768
endif

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
#	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/system_nrf51.h



LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	lib/cbuf

include $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk
