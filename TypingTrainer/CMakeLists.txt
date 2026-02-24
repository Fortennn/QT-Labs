cmake_minimum_required(VERSION 3.19)
project(TypingTrainer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(TypingTrainer
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    keyboardwidget.h
    keyboardwidget.cpp
    textdisplaywidget.h
    textdisplaywidget.cpp
    textmodel.h
    textmodel.cpp
    lessonloader.h
    lessonloader.cpp
)

target_link_libraries(TypingTrainer
    PRIVATE
        Qt::Core
        Qt::Widgets
)

# Copy lessons/ folder next to the executable after build
add_custom_command(TARGET TypingTrainer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/lessons"
        "$<TARGET_FILE_DIR:TypingTrainer>/lessons"
    COMMENT "Copying lessons/ to build output directory"
)

include(GNUInstallDirs)

install(TARGETS TypingTrainer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY lessons DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_app_script(
    TARGET TypingTrainer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
