CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)
project(Final)

IF (EXISTS ${CMAKE_SOURCE_DIR}/external)
  MESSAGE ("Using external directory for Windows/Mac build.")
  # Directories
  LINK_DIRECTORIES("/usr/local/lib" "/opt/local/lib")
  INCLUDE_DIRECTORIES("/usr/local/include" "/opt/local/include")
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
  set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

  # Flags
  set(CMAKE_CXX_FLAGS "--std=c++11 -g -fmax-errors=1")

  # Packages
  FIND_PACKAGE(OpenGL REQUIRED)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIRS})
  LINK_DIRECTORIES(${OPENGL_LIBRARY_DIRS})
  ADD_DEFINITIONS(${OPENGL_DEFINITIONS})

  MESSAGE(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
  LIST(APPEND stdgl_libraries ${OPENGL_gl_LIBRARY})

  # Compile external dependencies
  add_subdirectory (external)

  if (APPLE)
  	FIND_LIBRARY(COCOA_LIBRARY Cocoa REQUIRED)
  endif(APPLE)

  include_directories(
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/assimp-3.0.1270/include/
	external/bullet-2.81-rev2613/src/
	.
  )

  add_library(
          ImGui STATIC
          external/imgui/imconfig.h
          external/imgui/imgui.cpp
          external/imgui/imgui.h
          external/imgui/imgui_demo.cpp
          external/imgui/imgui_draw.cpp
          external/imgui/imgui_impl_glfw_gl3.cpp
          external/imgui/imgui_impl_glfw_gl3.h
          external/imgui/imgui_internal.h
          external/imgui/stb_rect_pack.h
          external/imgui/stb_textedit.h
          external/imgui/stb_truetype.h
          )

  set(ALL_LIBS
  	${OPENGL_LIBRARY}
  	glfw
    assimp
  	GLEW_1130
    ImGui
  )
  add_definitions(
  	-DTW_STATIC
  	-DTW_NO_LIB_PRAGMA
  	-DTW_NO_DIRECT3D
  	-DGLEW_STATIC
  	-D_CRT_SECURE_NO_WARNINGS
  )
ELSE()
  MESSAGE ("Using standard Linux build.")
  MESSAGE ("This will fail, need to use external dependencies!.")

  FILE(GLOB cmakes ${CMAKE_SOURCE_DIR}/cmake/*.cmake)
  FOREACH(cmake ${cmakes})
  	INCLUDE(${cmake})
  ENDFOREACH(cmake)
ENDIF()

MESSAGE(STATUS "stdgl: ${stdgl_libraries}")

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(src/shaders)
ADD_SUBDIRECTORY(src/assets)

IF (EXISTS ${CMAKE_SOURCE_DIR}/sln/CMakeLists.txt)
	ADD_SUBDIRECTORY(sln)
ENDIF()
