#
# Component Makefile
#
# This Makefile should, at the very least, just include $(SDK_PATH)/make/component.mk. By default,
# this will take the sources in this directory, compile them and link them into
# lib(subdirectory_name).a in the build directory. This behaviour is entirely configurable,
# please read the SDK documents if you need to do this.
#

COMPONENT_ADD_INCLUDEDIRS := . touch_key wifi esp32_shell led ota ws ci1006 i2s ethernet

COMPONENT_SRCDIRS :=  . touch_key wifi esp32_shell led ota ws ci1006 i2s ethernet
