cmake_minimum_required (VERSION 3.8)

project (terrible)

add_executable (${PROJECT_NAME}
	main.cpp
	Terrible.cpp Terrible.h
	LazyGlobal.cpp LazyGlobal.h
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
