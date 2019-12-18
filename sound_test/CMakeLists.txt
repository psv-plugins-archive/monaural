cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(MonauralConfig)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(A_NAME "${PROJECT_NAME}")
set(A_SELF eboot.bin)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -std=c99")
link_directories("${CMAKE_SOURCE_DIR}/../build/monaural_stubs")

set(VITA_APP_NAME "Monaural Config")
set(VITA_TITLEID "AKRK00003")
set(VITA_VERSION "01.00")

add_executable("${A_NAME}"
  main.c
  debug_screen/debugScreen.c
)

target_link_libraries("${A_NAME}"
  SceAppMgr_stub
  SceAudio_stub
  SceCtrl_stub
  SceDisplay_stub
  SceLibKernel_stub
  Monaural_stub
)

vita_create_self("${A_SELF}" "${A_NAME}" UNSAFE)
vita_create_vpk("${A_NAME}.vpk" "${VITA_TITLEID}" "${A_SELF}"
  VERSION "${VITA_VERSION}"
  NAME "${VITA_APP_NAME}"
  FILE ps.pcm ps.pcm
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/pic0.png sce_sys/pic0.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE sce_sys/livearea/contents/bg0.png sce_sys/livearea/contents/bg0.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
)
