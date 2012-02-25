cmake_minimum_required (VERSION 2.6)
project (Coldwind CXX)

Include(CPack)

if(WIN32)
  set(COLDWIND_WIN32 "true")
  set(COLDWIND_WIN32_DEFINE "1")
else()
  set(COLDWIND_WIN32 "false")
  set(COLDWIND_WIN32_DEFINE "0")
endif()

# The version number
set (Coldwind_VERSION_MAJOR 0)
set (Coldwind_VERSION_MINOR 1)

# check libs and packages (headers + lib)
set(Coldwind_glade overEngineering.glade)
set(Coldwind_fangpng fang.png)
set(Coldwind_killmegif KillMe.gif)
set(Coldwind_errorgif error.gif)
set(Coldwind_glade_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")
set(Coldwind_fangpng_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")
set(Coldwind_killmegif_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")
set(Coldwind_errorgif_location "${CMAKE_INSTALL_PREFIX}/share/coldwind")

INSTALL(FILES ${Coldwind_glade} DESTINATION ${Coldwind_glade_location})
INSTALL(FILES ${Coldwind_fangpng} DESTINATION ${Coldwind_fangpng_location})
INSTALL(FILES ${Coldwind_killmegif} DESTINATION ${Coldwind_killmegif_location})
INSTALL(FILES ${Coldwind_errorgif} DESTINATION ${Coldwind_errorgif_location})
INSTALL(PROGRAMS coldwind DESTINATION bin)

configure_file (
  "config.h.in"
  "config.h"
  )

# std C libraries
#find_package(stdlibs REQUIRED)

# libpthread with pthread.h
#find_package(pthread REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

cmake_policy(SET CMP0011 NEW)

set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "-std=c++0x")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0 -Wall -Wextra -pedantic" )
set(CMAKE_CXX_FLAGS_RELEASE "-Os")

find_package(CURL REQUIRED)
set(LIBS ${LIBS} ${CURL_LIBRARIES})
set(INCLUDES ${INCLUDES} ${CURL_INCLUDE_DIRS})

find_package(LibXml2 REQUIRED)
set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})
set(INCLUDES ${INCLUDES} ${LIBXML2_INCLUDE_DIR})
add_definitions( ${LIBXML2_DEFINITIONS} )

find_package(GTK3 3.0 REQUIRED gtk gtkmm gthread)
set(LIBS ${LIBS} ${GTK3_LIBRARIES})
set(INCLUDES ${INCLUDES} ${GTK3_INCLUDE_DIRS})


add_definitions( -D_GNU_SOURCE -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE )

include_directories(
  ${GTK3_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})

add_executable(coldwind main.cxx parser.cxx application.cxx hasher.cxx downloader.cxx image.cxx lurker.cxx manager.cxx request.cxx)

target_link_libraries(coldwind ${LIBS})

