macro(get_SDL3 version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)
  if(MSVC OR (WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang"))
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-VC.zip
      URL_HASH
        SHA256=05c63e401c388cf3cc822d57d28753301428895677b495a43c2e9ac5f0fa84ff)
  elseif(MINGW)
    FetchContent_Declare(
      SDL3
      URL https://github.com/libsdl-org/SDL/releases/download/release-${version}/SDL3-devel-${version}-mingw.zip
      URL_HASH
        SHA256=631dff26b92e8c0fa60b0168798d78d8ff660bc3d9df060e75edc9bd2558521b)
  else()
    find_package(SDL3 QUIET)
    if(NOT SDL3_FOUND)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG "release-${version}")
      FetchContent_MakeAvailable(SDL3)
      set(SDL3_LIBRARIES SDL3::SDL3)
    endif()
  endif()

  if(WIN32)
    FetchContent_MakeAvailable(SDL3)
    find_package(SDL3 CONFIG REQUIRED PATHS ${sdl3_SOURCE_DIR} NO_DEFAULT_PATH)
    get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
    file(COPY_FILE ${SDL3_DLL} ${EXPORT_DIR}/SDL3.dll ONLY_IF_DIFFERENT)
  endif()
endmacro(get_SDL3)
