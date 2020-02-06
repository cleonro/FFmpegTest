find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)

set(SRC_FFMPEG_TEST
    ffmain.cpp
    ffmainwindow.h ffmainwindow.cpp ffmainwindow.ui
    fftest.h fftest.cpp
    )

add_executable(ffmpeg_test ${SRC_FFMPEG_TEST})
target_link_libraries(ffmpeg_test Qt5::Widgets Qt5::Multimedia)

##ffmpeg
target_link_libraries(ffmpeg_test avformat avcodec avutil swresample)
