cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(ncore C CXX)

option(PACK_ASSETS "Pack game assets into data.pfs" OFF)

set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)

if (NGAGESDK)
  find_package(SDL3 REQUIRED)
  #find_package(SDL3_mixer REQUIRED)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  get_SDL3("3.2.16")
endif()

set(ncore_sources
  src/aabb.c
  src/app.c
  src/fixedp.cpp
  src/hero.c
  src/main.c
  src/map.c
  src/ncore.c
  src/pfs.c
  src/utils.c
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_executable(ncore WIN32 ${ncore_sources})
else()
  add_executable(ncore ${ncore_sources})
endif()

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include")

set_property(TARGET ncore PROPERTY C_STANDARD 99)

if(NGAGESDK)
  target_link_options(ncore PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(ncore PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(ncore PRIVATE "SHELL:-s UID3=0x1000c37e") # ncore.exe UID

  #target_link_libraries(ncore PRIVATE SDL3_mixer::SDL3_mixer)
  target_link_libraries(ncore PRIVATE SDL3::SDL3)

else()
  target_link_libraries(ncore PRIVATE ${SDL3_LIBRARIES})
endif()

if(WIN32)
  function(pack_assets asset_dir assets)
    add_custom_target(data.pfs ALL
      WORKING_DIRECTORY ${asset_dir}
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/packer ${assets}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${asset_dir}/data.pfs
        ${CMAKE_BINARY_DIR}/export/data.pfs
    )
  endfunction()
endif()

if(PACK_ASSETS)
  set(assets
    001.tmj
    002.tmj
    frame.png
    hero.png
    tileset.png
  )
  pack_assets(${CMAKE_CURRENT_SOURCE_DIR}/assets "${assets}")
endif()

if (NINTENDO_3DS)
  set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
  #file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/data.pfs DESTINATION ${ROMFS_DIR})
  set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/ncore.smdh")
  ctr_generate_smdh("${SMDH_FILE}"
    NAME "ncore"
    DESCRIPTION "A game engine, written in C."
    AUTHOR "Michael Fitzmayer"
    #ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
    )
  ctr_create_3dsx(
    ncore
    ROMFS "${ROMFS_DIR}"
    SMDH "${SMDH_FILE}")
endif()
