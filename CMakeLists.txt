cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(monaural)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O3 -std=gnu99 -mtune=cortex-a9")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

add_executable(${PROJECT_NAME}
  main.c
  config.c
)

target_link_libraries(${PROJECT_NAME}
  SceIofilemgrForDriver_stub
  SceSysclibForDriver_stub
  SceSysmemForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME} CONFIG exports.yml UNSAFE)
vita_create_stubs(${PROJECT_NAME}_stubs ${PROJECT_NAME}.skprx exports.yml KERNEL)
