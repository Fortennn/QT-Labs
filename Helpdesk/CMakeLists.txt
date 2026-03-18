cmake_minimum_required(VERSION 3.19)
project(Helpdesk VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(Helpdesk
    main.cpp
    ticket.h
    tickettablemodel.h
    tickettablemodel.cpp
    ticketdialog.h
    ticketdialog.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    ticketdialog.ui
)

target_link_libraries(Helpdesk PRIVATE Qt6::Widgets)

set_target_properties(Helpdesk PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
