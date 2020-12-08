#
# Component Makefile for using p44utils on ESP32 IDF platform
#

VIEWCONFIG_OBJS = \
  viewfactory.o

EPX_OBJS = \
  epxview.o

IMAGE_OBJS = \
  imageview.o

COMPONENT_SRCDIRS := \
  .

COMPONENT_OBJS:= \
  p44view.o \
  canvasview.o \
  coloreffectview.o \
  lifeview.o \
  lightspotview.o \
  textview.o \
  torchview.o \
  viewscroller.o \
  viewsequencer.o \
  viewstack.o \
  $(call compile_only_if,$(CONFIG_P44LRGRAPHICS_ENABLE_VIEWCONFIG),$(VIEWCONFIG_OBJS)) \
  $(call compile_only_if,$(CONFIG_P44LRGRAPHICS_ENABLE_EPX_SUPPORT),$(EPX_OBJS)) \
  $(call compile_only_if,$(CONFIG_P44LRGRAPHICS_IMAGE_SUPPORT),$(IMAGE_OBJS))

COMPONENT_ADD_INCLUDEDIRS = \
  .

CPPFLAGS += \
  -Wno-reorder \
  -frtti \
  -isystem /Volumes/CaseSens/openwrt/build_dir/target-mipsel_24kc_musl/boost_1_71_0

# %%%ugly hack above ^^^^^^ for now%%% just take a boost checkout we already have
