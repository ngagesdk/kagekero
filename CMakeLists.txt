cmake_minimum_required(VERSION 3.10)
project(kagekero C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PACK_ASSETS "Pack game assets into data.pfs" OFF)
option(BUILD_FULL_VERSION "Build full version" OFF)
option(DREAMCAST "Build for Dreamcast" OFF)

set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)
set(ASSET_OUTPUT ${EXPORT_DIR}/data.pfs)

# SDL3 setup
if (NGAGESDK)
  find_package(SDL3 REQUIRED)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  get_SDL3("3.2.16")
endif()

# Game source files
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

add_executable(kagekero ${kagekero_sources})

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include"
)
set_property(TARGET kagekero PROPERTY C_STANDARD 99)

# 📱 NGAGE-specific
if(NGAGESDK)
  target_link_libraries(kagekero PRIVATE SDL3::SDL3)
  target_link_options(kagekero PRIVATE "SHELL:-s UID1=0x1000007a" "SHELL:-s UID2=0x100039ce" "SHELL:-s UID3=0x1000c37e")
  target_link_libraries(kagekero PRIVATE ${EPOC_EXTRAS}/lib/zlib.lib)

  set(kagekero_app_sources
    src/ngage.cpp src/ngage_application.cpp
    src/ngage_appui.cpp src/ngage_appview.cpp
    src/ngage_document.cpp
  )

  add_library(kagekero_app STATIC ${kagekero_app_sources})
  build_dll(kagekero_app kagekero app 0x1000007a 0x100039ce 0x1badc0de "")
  build_aif(${CMAKE_CURRENT_SOURCE_DIR}/res kagekero 0x1badc0de)
  build_resource(${CMAKE_CURRENT_SOURCE_DIR}/res kagekero "")
endif()

# 🖥️ Host-only: build packer and pack data.pfs
if(PACK_ASSETS)
  set(PACKER_BINARY_DIR ${CMAKE_BINARY_DIR}/host-tools)
  file(MAKE_DIRECTORY ${PACKER_BINARY_DIR})

  # 🔧 Build packer using system CMake for host
  execute_process(
    COMMAND cmake -S ${CMAKE_SOURCE_DIR}/tools -B ${PACKER_BINARY_DIR}
    RESULT_VARIABLE packer_cmake_result
  )
  if(NOT packer_cmake_result EQUAL 0)
    message(FATAL_ERROR "Failed to configure host packer tool")
  endif()

  execute_process(
    COMMAND cmake --build ${PACKER_BINARY_DIR}
    RESULT_VARIABLE packer_build_result
  )
  if(NOT packer_build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build host packer tool")
  endif()

  set(PACKER_EXECUTABLE ${PACKER_BINARY_DIR}/packer)

  if(BUILD_FULL_VERSION)
    set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/full-assets")
    set(ASSET_LIST
      001.tmj 002.tmj 003.tmj
      frame.png frame_400x240.png
      frame_512x512.png kero.png tileset.png)
  else()
    set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    set(ASSET_LIST
      001.tmj 002.tmj
      frame.png frame_400x240.png
      frame_512x512.png kero.png tileset.png)
  endif()

  add_custom_command(
    OUTPUT ${ASSET_OUTPUT}
    WORKING_DIRECTORY ${ASSET_DIR}
    COMMAND ${PACKER_EXECUTABLE} ${ASSET_LIST}
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSET_DIR}/data.pfs ${ASSET_OUTPUT}
    DEPENDS ${PACKER_EXECUTABLE}
    COMMENT "📦 Packing assets into data.pfs"
  )

  add_custom_target(data.pfs ALL DEPENDS ${ASSET_OUTPUT})
endif()

if(UNIX)
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

# 🕹️ Dreamcast-specific setup
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

# 🎮 3DS-specific
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