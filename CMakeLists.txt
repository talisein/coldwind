cmake_minimum_required (VERSION 2.6)
project (Coldwind CXX)

# The version number
set (Coldwind_VERSION_MAJOR 0)
set (Coldwind_VERSION_MINOR 1)

# check libs and packages (headers + lib)
set(Coldwind_glade overEngineering.glade)
set(Coldwind_fangpng fang.png)
set(Coldwind_killmegif KillMe.gif)
set(Coldwind_glade_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")
set(Coldwind_fangpng_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")
set(Coldwind_killmegif_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")

INSTALL(FILES ${Coldwind_glade} DESTINATION ${Coldwind_glade_location})
INSTALL(FILES ${Coldwind_fangpng} DESTINATION ${Coldwind_fangpng_location})
INSTALL(FILES ${Coldwind_killmegif} DESTINATION ${Coldwind_killmegif_location})
INSTALL(PROGRAMS coldwind DESTINATION bin)

configure_file (
  "config.h.in"
  "config.h"
  )

# std C libraries
#find_package(stdlibs REQUIRED)

# libpthread with pthread.h
#find_package(pthread REQUIRED)

find_package(Git)
if(Git_FOUND)

endif(Git_FOUND)

find_package(PkgConfig)

pkg_check_modules(GTKMM3 gtkmm-3.0 REQUIRED)
pkg_check_modules(XML++ libxml-2.0 REQUIRED)
pkg_check_modules(GTHREAD gthread-2.0 REQUIRED)
pkg_check_modules(CURL libcurl REQUIRED)

set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0" )

add_definitions(-Wall -Wextra -pedantic -std=c++0x -D_GNU_SOURCE)

link_directories(
  ${GTKMM3_LIBRARY_DIRS} ${GTHREAD_LIBRARY_DIRS} ${XML++_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})

include_directories(
  ${GTKMM3_INCLUDE_DIRS} ${GTHREAD_LIBRARY_DIRS} ${XML++_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

add_executable(coldwind main.cxx parser.cxx application.cxx hasher.cxx downloader.cxx image.cxx lurker.cxx)

target_link_libraries(coldwind ${GTKMM3_LIBRARIES} ${GTHREAD_LIBRARIES} ${XML++_LIBRARIES} ${CURL_LIBRARIES})

