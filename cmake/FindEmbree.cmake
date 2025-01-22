find_path(EMBREE_INCLUDE_PATH embree4/rtcore.h
  /usr/include
  /usr/local/include
  /opt/local/include)

if (APPLE)
find_library(EMBREE_LIBRARY NAMES embree4 PATHS
  /usr/lib
  /usr/local/lib
  /opt/local/lib)
elseif (WIN32)
  message(FATAL_ERROR "Windows is not supported.")
else ()
find_library(EMBREE_LIBRARY NAMES embree4 PATHS
  /usr/lib
  /usr/local/lib
  /opt/local/lib)
endif ()

if (EMBREE_INCLUDE_PATH AND EMBREE_LIBRARY)
  set(EMBREE_FOUND TRUE)
endif ()