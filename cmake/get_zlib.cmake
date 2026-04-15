macro(get_zlib version)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)
  FetchContent_Declare(
    zlib
    URL https://zlib.net/zlib-${version}.tar.gz
    URL_HASH
      SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
  )
  set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable Zlib Examples" FORCE)
  FetchContent_MakeAvailable(zlib)

  set(ZLIB_LIBRARIES zlib)
endmacro(get_zlib)
