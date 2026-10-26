if(ESP_PLATFORM)
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

  set(incl "." "../../main")
  set(req p44utils)

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

  idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incl}" REQUIRES "${req}")
  target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-reorder -Wno-missing-field-initializers -frtti)
  return()
endif()

add_library(p44lrgraphics STATIC
  blocksview.cpp
  canvasview.cpp
  coloreffectview.cpp
  epxview.cpp
  fonts.cpp
  imageview.cpp
  lifeview.cpp
  lightspotview.cpp
  p44view.cpp
  textview.cpp
  torchview.cpp
  viewfactory.cpp
  viewscroller.cpp
  viewsequencer.cpp
  viewstack.cpp
  fonts/font_3x3.cpp
  fonts/font_3x4.cpp
  fonts/font_5x5.cpp
  fonts/font_5x7.cpp
  fonts/font_5x8.cpp
  fonts/font_bios.cpp
  fonts/font_m3x6.cpp
  fonts/font_m5x7.cpp
  fonts/font_m6x11.cpp
  fonts/font_sixtyfour.cpp
  fonts/font_vcr_osd_mono.cpp
)

target_include_directories(p44lrgraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(p44lrgraphics PUBLIC p44utils vdcd_config)
target_compile_options(p44lrgraphics PRIVATE -Wno-reorder -Wno-missing-field-initializers)

option(P44LRG_TIMING_TESTS "Build presentation timing regression tests" OFF)
if(P44LRG_TIMING_TESTS)
  add_executable(p44lrg_timing_test tests/presentation_timing.cpp)
  target_link_libraries(p44lrg_timing_test PRIVATE p44lrgraphics)
endif()
