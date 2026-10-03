cmake_minimum_required(VERSION 3.16)
project(town_scene CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# On Windows, point FREEGLUT_ROOT at the freeglut MSVC package (the folder with include/, lib/ and bin/)
set(FREEGLUT_ROOT "" CACHE PATH "freeglut install folder (Windows)")

add_executable(town-scene src/main.cpp)
target_include_directories(town-scene PRIVATE third_party/stb)

if(WIN32)
  if(NOT FREEGLUT_ROOT)
    message(FATAL_ERROR "Set FREEGLUT_ROOT, e.g. -DFREEGLUT_ROOT=C:/libs/freeglut")
  endif()
  target_include_directories(town-scene PRIVATE "${FREEGLUT_ROOT}/include")
  target_link_libraries(town-scene PRIVATE "${FREEGLUT_ROOT}/lib/x64/freeglut.lib" OpenGL::GL OpenGL::GLU)
  # freeglut.dll next to the executable
  add_custom_command(TARGET town-scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FREEGLUT_ROOT}/bin/x64/freeglut.dll" $<TARGET_FILE_DIR:town-scene>)
  # Textures are loaded from assets/ relative to the working directory: run from the project folder
  set_property(TARGET town-scene PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
else()
  find_package(GLUT REQUIRED)
  target_link_libraries(town-scene PRIVATE GLUT::GLUT OpenGL::GL OpenGL::GLU m)
endif()
