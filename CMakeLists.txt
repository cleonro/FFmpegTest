project(FFmpegTest)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)

set(CMAKE_AUTOMOC ON)

find_path(FFMPEG_INCLUDE_DIR NAMES avcodec.h)
if(NOT FFMPEG_INCLUDE_DIR)
            message(FATAL_ERROR "avcodec.h not found!")
    else()
            message("FFMPEG_INCLUDE_DIR: " ${FFMPEG_INCLUDE_DIR}/../)
    endif()
include_directories(${FFMPEG_INCLUDE_DIR}/../)

find_library(FFMPEG_AVCODEC_LIBRARY NAMES avcodec libavcodec)
if(NOT FFMPEG_AVCODEC_LIBRARY)
           message(FATAL_ERROR "ffmpeg avcodec library not found!")
else()
           message("FFMPEG_AVCODEC_LIBRARY: " ${FFMPEG_AVCODEC_LIBRARY})
endif()

find_library(FFMPEG_AVFORMAT_LIBRARY NAMES avformat libavformat)
if(NOT FFMPEG_AVFORMAT_LIBRARY)
           message(FATAL_ERROR "ffmpeg avformat library not found!")
else()
           message("FFMPEG_AVFORMAT_LIBRARY: " ${FFMPEG_AVFORMAT_LIBRARY})
endif()

find_library(FFMPEG_AVUTIL_LIBRARY NAMES avutil libavutil)
if(NOT FFMPEG_AVUTIL_LIBRARY)
           message(FATAL_ERROR "ffmpeg avutil library not found!")
else()
           message("FFMPEG_AVCODEC_LIBRARY: " ${FFMPEG_AVUTIL_LIBRARY})
endif()

find_library(FFMPEG_SWRESAMPLE_LIBRARY NAMES swresample libswresample)
if(NOT FFMPEG_SWRESAMPLE_LIBRARY)
           message(FATAL_ERROR "ffmpeg swresample library not found!")
else()
           message("FFMPEG_SWRESAMPLE_LIBRARY: " ${FFMPEG_SWRESAMPLE_LIBRARY})
endif()


set(SRC_FFMPEG_TEST
    ffmain.cpp
    ffmainwindow.h ffmainwindow.cpp ffmainwindow.ui
    fftest.h fftest.cpp
    ffaudiooutput.h ffaudiooutput.cpp
    sbr_utils.c
    )

set(UI_SRC_FFMPEG_TEST
    ffmainwindow.ui
)

include_directories(${PROJECT_BINARY_DIR})
message("Build directory: " ${PROJECT_BINARY_DIR})

qt5_wrap_ui(UI_SRC_FFMPEG_TEST ${UI_SRC_FFMPEG_TEST})

add_executable(ffmpeg_test ${SRC_FFMPEG_TEST} ${UI_SRC_FFMPEG_TEST})
target_link_libraries(ffmpeg_test Qt5::Widgets Qt5::Multimedia)

##ffmpeg
#target_link_libraries(ffmpeg_test avformat avcodec avutil swresample)
target_link_libraries(ffmpeg_test ${FFMPEG_AVCODEC_LIBRARY} ${FFMPEG_AVFORMAT_LIBRARY} ${FFMPEG_AVUTIL_LIBRARY} ${FFMPEG_SWRESAMPLE_LIBRARY})

