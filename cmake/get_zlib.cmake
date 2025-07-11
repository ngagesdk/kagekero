macro(get_zlib version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)
  FetchContent_Declare(
    zlib
    URL https://zlib.net/zlib-${version}.tar.gz
    URL_HASH
      SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
  )
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable Zlib Examples" FORCE)
  FetchContent_MakeAvailable(zlib)

  set(ZLIB_LIBRARIES zlib)
endmacro(get_zlib)
