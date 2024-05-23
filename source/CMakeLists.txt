

add_lib_sources(cba.c)
add_lib_sources(lock.c)
add_lib_sources(ringbuf.c)
add_lib_sources(string.c)
add_lib_sources(timer.c)

add_subdirectory("core")
add_subdirectory("lib")

