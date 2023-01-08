file(GLOB_RECURSE SRC *.cpp)
file(GLOB_RECURSE INC *.h*)

add_library(c-devices OBJECT
	${SRC} ${INC}
)
target_compile_features(c-devices PUBLIC cxx_std_20)
target_link_libraries(c-devices device-interface)
