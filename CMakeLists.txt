cmake_minimum_required(VERSION 3.10)
project(kagekero C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PACK_ASSETS "Pack game assets into data.pfs" OFF)
option(BUILD_FULL_VERSION "Build full version" OFF)
option(DREAMCAST "Build for Dreamcast" OFF)
option(DISABLE_ZLIB "Disable zlib dependency" OFF)

set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)
set(ASSET_OUTPUT ${EXPORT_DIR}/data.pfs)

# SDL3 setup.
if (NGAGESDK)
  find_package(SDL3 REQUIRED)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  if(NOT DISABLE_ZLIB)
    include(get_zlib)
    get_zlib("1.3.1")
  endif()
  get_SDL3("3.2.16")
endif()

# Game source files.
set(kagekero_sources
  src/aabb.c
  src/app.c
  src/cheats.c
  src/core.c
  src/fixedp.c
  src/kero.c
  src/main.c
  src/map.c
  src/overlay.c
  src/pfs.c
  src/utils.c
)

add_executable(kagekero WIN32 ${kagekero_sources})
target_compile_definitions(kagekero PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include"
)
set_property(TARGET kagekero PROPERTY C_STANDARD 99)

# NGAGE-specific.
if(NGAGESDK)
  target_link_libraries(kagekero PRIVATE SDL3::SDL3)
  target_link_options(kagekero PRIVATE "SHELL:-s UID1=0x1000007a" "SHELL:-s UID2=0x100039ce" "SHELL:-s UID3=0x1000c37e")
  target_link_libraries(kagekero PRIVATE ${EPOC_EXTRAS}/lib/zlib.lib)

  set(kagekero_app_sources
    src/ngage.cpp
    src/ngage_application.cpp
    src/ngage_appui.cpp
    src/ngage_appview.cpp
    src/ngage_document.cpp
  )

  set(kagekero_app_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/avkon.lib
  )

  set(ninja_dll_sources
    src/ninja.cpp
  )

  set(ninja_dll_libs "")

  add_library(kagekero_app STATIC ${kagekero_app_sources})
  build_dll(kagekero_app kagekero app 0x1000007a 0x100039ce 0x1badc0de "${kagekero_app_libs}")
  build_aif(${CMAKE_CURRENT_SOURCE_DIR}/res kagekero 0x1badc0de)
  build_resource(${CMAKE_CURRENT_SOURCE_DIR}/res kagekero "")

  add_library(ninja_dll STATIC ${ninja_dll_sources})
  build_dll(ninja_dll ninja dll 0x1000007a 0x100039e6 0xa00013bd "${ninja_dll_libs}")
endif()

# Host-only: build packer and pack data.pfs
if(PACK_ASSETS)
  set(PACKER_BINARY_DIR ${CMAKE_BINARY_DIR}/host-tools)
  file(MAKE_DIRECTORY ${PACKER_BINARY_DIR})

  if(NGAGESDK)

  set(PACKER_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/tools/packer.exe)

  elseif(WIN32)
    # Use default compiler and build config for MSVC.
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}/tools
        -B ${PACKER_BINARY_DIR}
      RESULT_VARIABLE packer_cmake_result
    )

    execute_process(
      COMMAND ${CMAKE_COMMAND}
        --build ${PACKER_BINARY_DIR} --config Release
      RESULT_VARIABLE packer_build_result
    )

    set(PACKER_EXECUTABLE ${PACKER_BINARY_DIR}/Release/packer.exe)

  else()
    # Use host system compilers (avoid cross-compilers like KOS or devkitPro).
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}/tools
        -B ${PACKER_BINARY_DIR}
        -DCMAKE_SYSTEM_NAME=Generic
        -DCMAKE_C_COMPILER=/usr/bin/cc
        -DCMAKE_CXX_COMPILER=/usr/bin/c++
      RESULT_VARIABLE packer_cmake_result
    )

    execute_process(
      COMMAND ${CMAKE_COMMAND}
        --build ${PACKER_BINARY_DIR}
      RESULT_VARIABLE packer_build_result
    )

    set(PACKER_EXECUTABLE ${PACKER_BINARY_DIR}/packer)
  endif()

  if(NOT NGAGESDK)
    if(NOT packer_cmake_result EQUAL 0 OR NOT packer_build_result EQUAL 0)
      message(FATAL_ERROR "Failed to configure or build host packer tool")
    endif()
  endif()

  function(compress_maps asset_dir)
    find_program(GZIP_EXECUTABLE gzip PATHS ${CMAKE_CURRENT_SOURCE_DIR}/tools NO_DEFAULT_PATH)
    if(NOT GZIP_EXECUTABLE)
      find_program(GZIP_EXECUTABLE gzip)
    endif()

    add_custom_target(001.tmj.gz ALL
      WORKING_DIRECTORY ${asset_dir}
      COMMAND ${GZIP_EXECUTABLE} -k -f *.tmj
    )
  endfunction()

  if(DISABLE_ZLIB)
    set(MAP_SUFFIX "tmj")
  else()
    set(MAP_SUFFIX "tmj.gz")
  endif()

  set(BASE_ASSETS
    001.${MAP_SUFFIX}
    002.${MAP_SUFFIX}
    font.png
    frame.png
    frame_400x240.png
    frame_512x512.png
    kero.png
    overlay.png
    tileset.png
  )

  if(BUILD_FULL_VERSION)
    set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/full-assets")

    if (NOT DISABLE_ZLIB)
      compress_maps("${ASSET_DIR}")
    endif()

    set(ASSET_LIST
      "${BASE_ASSETS}"
      003.${MAP_SUFFIX}
      004.${MAP_SUFFIX}
      005.${MAP_SUFFIX}
      006.${MAP_SUFFIX}
      007.${MAP_SUFFIX}
    )
  else()

    set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

    if (NOT DISABLE_ZLIB)
      compress_maps("${ASSET_DIR}")
    endif()

    set(ASSET_LIST ${BASE_ASSETS})
  endif()

  add_custom_command(
    OUTPUT ${ASSET_OUTPUT}
    WORKING_DIRECTORY ${ASSET_DIR}
    COMMAND ${PACKER_EXECUTABLE} ${ASSET_LIST}
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSET_DIR}/data.pfs ${ASSET_OUTPUT}
    DEPENDS ${PACKER_EXECUTABLE}
    COMMENT "Packing assets into data.pfs"
  )

  add_custom_target(data.pfs ALL DEPENDS ${ASSET_OUTPUT})
endif()

if(UNIX)
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES} ${ZLIB_LIBRARIES} m)

  if(PACK_ASSETS)
    add_executable(packer tools/packer.cpp)
    set_target_properties(packer PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools"
    )
  endif()
elseif(EMSCRIPTEN)
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES})
else()
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES} ${ZLIB_LIBRARIES})
endif()

# Dreamcast-specific setup.
if(DREAMCAST)
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES} GL pthread)

  set(KOS_ROMDISK_DIR "${CMAKE_CURRENT_BINARY_DIR}/romdisk_kagekero")
  file(MAKE_DIRECTORY "${KOS_ROMDISK_DIR}")

  add_custom_command(
    OUTPUT ${KOS_ROMDISK_DIR}/data.pfs
    DEPENDS ${ASSET_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSET_OUTPUT} ${KOS_ROMDISK_DIR}/data.pfs
    COMMENT "Copying data.pfs to Dreamcast romdisk"
  )

  add_custom_target(copy_data_pfs ALL DEPENDS ${KOS_ROMDISK_DIR}/data.pfs)
  add_dependencies(kagekero copy_data_pfs)

  kos_add_romdisk(kagekero "${KOS_ROMDISK_DIR}" kagekero)

  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  file(MAKE_DIRECTORY "${OUTPUT_DIR}")
  add_custom_command(TARGET kagekero POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:kagekero>" "${OUTPUT_DIR}/kagekero.elf")
endif()

# 3DS-specific.
if(NINTENDO_3DS)
  set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
  file(COPY ${ASSET_OUTPUT} DESTINATION ${ROMFS_DIR})
  set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/kagekero.smdh")

  ctr_generate_smdh("${SMDH_FILE}"
    NAME "kagekero"
    DESCRIPTION "A minimalist, cross-platform puzzle-platformer."
    AUTHOR "Michael Fitzmayer"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")

  ctr_create_3dsx(kagekero ROMFS "${ROMFS_DIR}" SMDH "${SMDH_FILE}")
endif()
