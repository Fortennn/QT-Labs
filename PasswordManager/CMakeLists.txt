cmake_minimum_required(VERSION 3.16)

project(PasswordManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    passwordtablemodel.cpp
    passwordtablemodel.h
    passwordentry.h
    databasemanager.cpp
    databasemanager.h
    passwordrepository.cpp
    passwordrepository.h
    passwordfilterproxymodel.cpp
    passwordfilterproxymodel.h
    addentrydialog.cpp
    addentrydialog.h
    passwordleakchecker.cpp
    passwordleakchecker.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PasswordManager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(PasswordManager
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(PasswordManager PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

if(${QT_VERSION_MAJOR} EQUAL 6)
    qt_finalize_executable(PasswordManager)
endif()
