cmake_minimum_required(VERSION 3.0)
project(Wonderland)

find_package(OpenGL REQUIRED)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(external)

include_directories(
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glad-opengl-3.3/include/
	external/
	scene/
)

add_executable(main
		scene/main.cpp
		scene/utils/texture_manager.cpp
		scene/utils/world_manager.cpp
		scene/entities/ground.cpp
        scene/entities/chunk.cpp
		scene/entities/static_model.cpp
		scene/entities/animated_model.cpp
	scene/render/shader.cpp
)

target_link_libraries(main
	${OPENGL_LIBRARY}
	glfw
	glad
)
