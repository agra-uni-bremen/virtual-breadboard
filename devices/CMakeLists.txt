file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)


add_library(devices ${SRC} ${INC})
target_link_libraries(devices device-interface)