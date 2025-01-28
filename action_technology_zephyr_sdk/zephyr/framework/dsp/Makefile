ccflags-y += -DSYS_LOG_DOMAIN=\"dsphal\"
obj-y += dsp_hal.o dsp_function.o dsp_buffer.o dsp_image.o

ifneq ($(CONFIG_DSP_LIB_IN_SDFS),y)
asflags-y += -Wa,-I${ZEPHYR_BASE}/ext/actions/porting/hal/images
obj-y += dsp_builtin_images.o
endif
