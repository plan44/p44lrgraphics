set(srcs
  "p44view.cpp"
  "canvasview.cpp"
  "coloreffectview.cpp"
  "lifeview.cpp"
  "lightspotview.cpp"
  "textview.cpp"
  "torchview.cpp"
  "viewscroller.cpp"
  "viewsequencer.cpp"
  "viewstack.cpp")

set(incl
  "." "../../main")

set(req
  p44utils)

if(CONFIG_P44LRGRAPHICS_ENABLE_VIEWCONFIG)
    list(APPEND srcs "viewfactory.cpp")
endif()

if(CONFIG_P44LRGRAPHICS_ENABLE_EPX_SUPPORT)
    list(APPEND srcs "epxview.cpp")
endif()

if(CONFIG_P44LRGRAPHICS_IMAGE_SUPPORT)
    list(APPEND srcs "imageview.cpp")
    list(APPEND req libpng)
endif()



idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${incl}"
                    REQUIRES "${req}")

# Sample how to include definitions
#target_compile_definitions(${COMPONENT_LIB} PUBLIC
#    -DUNITY_INCLUDE_CONFIG_H
#)

target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wno-reorder
  -frtti
  -isystem /Volumes/CaseSens/openwrt/build_dir/target-mipsel_24kc_musl/boost_1_71_0)

# %%%ugly hack above ^^^^^^ for now%%% just take a boost checkout we already have
