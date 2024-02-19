set(gameloop_source_files
  gameloop.c
  gameloop.h
  browseloop.c
  browseloop.h
  hoeldb.h
  hoeldb.c
  telemetry.h
  telemetry.c
)


add_library(gameloop STATIC ${gameloop_source_files})

target_compile_definitions(gameloop PRIVATE SIMMAP_ALL)
