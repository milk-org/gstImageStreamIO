cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x -g -fpermissive")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)

if(NOT TARGET ImageStreamIO)
    if(NOT DEFINED IMAGESTREAMIO_ROOT)
      if(DEFINED ENV{IMAGESTREAMIO_ROOT})
        set(IMAGESTREAMIO_ROOT $ENV{IMAGESTREAMIO_ROOT})
      else()
        set(IMAGESTREAMIO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/tplib/ImageStreamIO)
      endif()
    endif()
    if(IS_DIRECTORY ${IMAGESTREAMIO_ROOT})
        add_subdirectory(${IMAGESTREAMIO_ROOT} ImageStreamIO)
    else()
        message( FATAL_ERROR "wyrm not found, specify it by set IMAGESTREAMIO_ROOT to the right path" )
    endif()
endif()

include_directories(
  ${GLIB_INCLUDE_DIRS}
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
)

set(GSTREAMER_MINIMUM_VERSION 1.0.5)

pkg_check_modules(GST1_TEST gstreamer-1.0)
if ( GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-base-1.0)
    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-video-1.0)
    #    pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
    #    pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
    #    pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)
    #    pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)
    #    add_definitions(-DGST_API_VERSION_1=1)
else()
  message(FATAL_ERROR "gstreamer-1.0 not found")
endif()

include_directories(
  ${GSTREAMER_INCLUDE_DIRS}
  ${GSTREAMER-APP_INCLUDE_DIRS}
  ${GSTREAMER-AUDIO_INCLUDE_DIRS}
  ${GSTREAMER-PBUTILS_INCLUDE_DIRS}
  ${GSTREAMER-FFT_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
  ${GSTREAMER_LIBRARY_DIRS}
  ${GSTREAMER-APP_LIBRARY_DIRS}
  ${GSTREAMER-AUDIO_LIBRARY_DIRS}
  ${GSTREAMER-PBUTILS_LIBRARY_DIRS}
  ${GSTREAMER-FFT_LIBRARY_DIRS}
)

add_library( gstimageStreamIO SHARED
  src/gstImageStreamIO.c
  src/gstImageStreamIOsrc.c
)

set(gst_imageStreamIOsrc_LIBRARIES
  ${GSTREAMER_LIBRARIES}
  ${GSTREAMER-APP_LIBRARIES}
  ${GSTREAMER-AUDIO_LIBRARIES}
  ${GSTREAMER-PBUTILS_LIBRARIES}
  ${GSTREAMER-FFT_LIBRARIES}

  pthread
  ${GLIB_LIBRARIES}
  ${GLIB_GIO_LIBRARIES}
  ${GLIB_GOBJECT_LIBRARIES}

  ImageStreamIO
)

target_link_libraries(gstimageStreamIO ${gst_imageStreamIOsrc_LIBRARIES} ImageStreamIO)

add_executable(gstimageStreamIOsrc_server src/server.c)
target_link_libraries(gstimageStreamIOsrc_server ${GSTREAMER_LIBRARIES})
add_executable(gstimageStreamIOsrc_client src/client.c)
target_link_libraries(gstimageStreamIOsrc_client ${GSTREAMER_LIBRARIES})


