SET(pwd ${CMAKE_CURRENT_LIST_DIR})

AUX_SOURCE_DIRECTORY(${pwd} src)
add_executable(lens ${src})
message(STATUS "lens added")

target_link_libraries(lens ${stdgl_libraries})
target_link_libraries(lens ${ALL_LIBS})

if(UNIX)
  FIND_PACKAGE(JPEG REQUIRED)
  TARGET_LINK_LIBRARIES(lens ${JPEG_LIBRARIES})
endif(UNIX)
