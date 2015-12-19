cmake_minimum_required (VERSION 2.6)
project (Coldwind CXX C)
cmake_policy(SET CMP0011 NEW)

Include(CPack)

if(WIN32)
  set(COLDWIND_WIN32 "true")
  set(COLDWIND_WIN32_DEFINE "1")
else(WIN32)
  set(COLDWIND_WIN32 "false")
  set(COLDWIND_WIN32_DEFINE "0")
endif(WIN32)

# The version number
set (Coldwind_VERSION_MAJOR 0)
set (Coldwind_VERSION_MINOR 5)

# check libs and packages (headers + lib)
INSTALL(PROGRAMS coldwind DESTINATION bin)

configure_file (
  "config.h.in"
  "config.h"
  )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS "-std=c++1y")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -Og -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(CURL REQUIRED)
set(LIBS ${LIBS} ${CURL_LIBRARIES})
set(INCLUDES ${INCLUDES} ${CURL_INCLUDE_DIRS})

find_package(LibXml2 REQUIRED)
set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})
set(INCLUDES ${INCLUDES} ${LIBXML2_INCLUDE_DIR})
add_definitions( ${LIBXML2_DEFINITIONS} )

find_package(GTK3 3.4 REQUIRED gtk gtkmm gthread)
set(LIBS ${LIBS} ${GTK3_LIBRARIES})
set(INCLUDES ${INCLUDES} ${GTK3_INCLUDE_DIRS})

find_package(PkgConfig)
pkg_check_modules(JSONGLIB json-glib-1.0)
set(LIBS ${LIBS} ${JSONGLIB_LIBRARIES})
set(INCLUDES ${INCLUDES} ${JSONGLIB_INCLUDE_DIRS})

add_definitions( -D_GNU_SOURCE -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_32 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32 )

include_directories( ${INCLUDES} )

add_executable(coldwind coldwind_resources.c main.cxx parser.cxx application.cxx hasher.cxx downloader.cxx lurker.cxx manager.cxx request.cxx window.cxx window_gtk3.cxx windowimpl.cxx xml_reader.cpp xml_writer.cpp horizon_post.c post.cpp)

target_link_libraries(coldwind ${LIBS} -lpthread)

