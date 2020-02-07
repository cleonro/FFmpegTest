find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)

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

set(SRC_FFMPEG_TEST
    ffmain.cpp
    ffmainwindow.h ffmainwindow.cpp ffmainwindow.ui
    fftest.h fftest.cpp
    )

add_executable(ffmpeg_test ${SRC_FFMPEG_TEST})
target_link_libraries(ffmpeg_test Qt5::Widgets Qt5::Multimedia)

##ffmpeg
#target_link_libraries(ffmpeg_test avformat avcodec avutil swresample)
target_link_libraries(ffmpeg_test ${FFMPEG_AVCODEC_LIBRARY} ${FFMPEG_AVFORMAT_LIBRARY} ${FFMPEG_AVUTIL_LIBRARY})

