cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(kagekero C CXX)

option(PACK_ASSETS "Pack game assets into data.pfs" OFF)
option(BUILD_FULL_VERSION "Build full version" OFF)

set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)

if (NGAGESDK)
  find_package(SDL3 REQUIRED)
  #find_package(SDL3_mixer REQUIRED)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  get_SDL3("3.2.16")
endif()

set(kagekero_sources
  src/aabb.c
  src/app.c
  src/fixedp.c
  src/kagekero.c
  src/kero.c
  src/main.c
  src/map.c
  src/pfs.c
  src/utils.c
)

#add_executable(kagekero WIN32 ${kagekero_sources})
add_executable(kagekero ${kagekero_sources})

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include")

set_property(TARGET kagekero PROPERTY C_STANDARD 99)

if(NGAGESDK)
  target_link_options(kagekero PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(kagekero PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(kagekero PRIVATE "SHELL:-s UID3=0x1000c37e") # kagekero.exe UID

  #target_link_libraries(kagekero PRIVATE SDL3_mixer::SDL3_mixer)
  target_link_libraries(kagekero PRIVATE SDL3::SDL3)
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
    ${EPOC_LIB}/avkon.lib)

  set(res_dir "${CMAKE_CURRENT_SOURCE_DIR}/res")

  add_library(kagekero_app STATIC ${kagekero_app_sources})
  build_dll(kagekero_app kagekero app 0x1000007a 0x100039ce 0x1badc0de "${kagekero_app_libs}")
  build_aif(${res_dir} kagekero 0x1badc0de)
  build_resource(${res_dir} kagekero "")

elseif(DREAMCAST)

  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES} GL pthread)

  # Set up romdisk directory and copy data.pfs.
  set(KOS_ROMDISK_DIR "${CMAKE_CURRENT_BINARY_DIR}/romdisk_kagekero")
  file(MAKE_DIRECTORY "${KOS_ROMDISK_DIR}")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/data.pfs DESTINATION ${KOS_ROMDISK_DIR})

  # Add romdisk to the executable.
  kos_add_romdisk(kagekero "${KOS_ROMDISK_DIR}" kagekero)

  # Create output directory and copy final executable.
  set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/dc-kagekero")
  file(MAKE_DIRECTORY "${OUTPUT_DIR}")
  add_custom_command(TARGET kagekero POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:kagekero>" "${OUTPUT_DIR}/kagekero.elf"
  )

elseif(UNIX)
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES} m)

  if(PACK_ASSETS)
    add_executable(packer tools/packer.cpp)
    set_target_properties(packer PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools"
    )
  endif()
else()
  target_link_libraries(kagekero PRIVATE ${SDL3_LIBRARIES})
endif()

function(pack_assets asset_dir assets)
  add_custom_target(data.pfs ALL
    WORKING_DIRECTORY ${asset_dir}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/packer ${assets}
    COMMAND ${CMAKE_COMMAND} -E copy
      ${asset_dir}/data.pfs
      ${EXPORT_DIR}/data.pfs
  )
endfunction()

function(compress_maps asset_dir)
  add_custom_target(001.tmj.gz ALL
    WORKING_DIRECTORY ${asset_dir}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/gzip -k -f *.tmj
  )
endfunction()

if(PACK_ASSETS)
  if(BUILD_FULL_VERSION)
    if(NGAGESDK)
      set(assets
        "001.tmj.gz"
        "002.tmj.gz"
        "003.tmj.gz"
        "004.tmj.gz"
        "005.tmj.gz"
        "006.tmj.gz"
        "007.tmj.gz"
        "frame.png"
        "frame_400x240.png"
        "frame_512x512.png"
        "kero.png"
        "tileset.png"
      )
      compress_maps("${CMAKE_CURRENT_SOURCE_DIR}/full-assets")
    else()
      set(assets
        "001.tmj"
        "002.tmj"
        "003.tmj"
        "004.tmj"
        "005.tmj"
        "006.tmj"
        "007.tmj"
        "frame.png"
        "frame_400x240.png"
        "frame_512x512.png"
        "kero.png"
        "tileset.png"
      )
    endif()
    pack_assets("${CMAKE_CURRENT_SOURCE_DIR}/full-assets" "${assets}")

  else()
    if (NGAGESDK)
      set(assets
        "001.tmj.gz"
        "002.tmj.gz"
        "frame.png"
        "frame_400x240.png"
        "frame_512x512.png"
        "kero.png"
        "tileset.png"
      )
      compress_maps("${CMAKE_CURRENT_SOURCE_DIR}/assets")
    else()
      set(assets
        "001.tmj"
        "002.tmj"
        "frame.png"
        "frame_400x240.png"
        "frame_512x512.png"
        "kero.png"
        "tileset.png"
      )
    endif()

    pack_assets("${CMAKE_CURRENT_SOURCE_DIR}/assets" "${assets}")

  endif()

endif()

if (NINTENDO_3DS)
  set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/data.pfs DESTINATION ${ROMFS_DIR})
  set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/kagekero.smdh")

  ctr_generate_smdh("${SMDH_FILE}"
    NAME "kagekero"
    DESCRIPTION "A minimalist, cross-platform puzzle-platformer."
    AUTHOR "Michael Fitzmayer"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png"
  )

  ctr_create_3dsx(
    kagekero
    ROMFS "${ROMFS_DIR}"
    SMDH "${SMDH_FILE}"
  )
endif()
