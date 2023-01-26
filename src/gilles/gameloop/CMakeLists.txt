set(gameloop_source_files
  gameloop.c
  gameloop.h
)

set(LIBXML_INCLUDE_DIR /usr/include/libxml2)
include_directories("." ${LIBXML_INCLUDE_DIR})

add_library(gameloop STATIC ${gameloop_source_files})
