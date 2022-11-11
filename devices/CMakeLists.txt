file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)

add_library(c-devices OBJECT
	${SRC} ${INC}
)

target_link_libraries(c-devices device-interface)
