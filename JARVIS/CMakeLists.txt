cmake_minimum_required(VERSION 3.16)
project(JARVIS VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Обмежуємо паралельні завдання збірки щоб пк не зависал
set(CMAKE_BUILD_PARALLEL_LEVEL 2)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql WebSockets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Network Sql WebSockets REQUIRED)

include(FetchContent)

# Отримуємо вихідний код ЛИШЕ для хедерів
FetchContent_Declare(
    llama_cpp_headers
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG        b4827
)
FetchContent_MakeAvailable(llama_cpp_headers)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS "src/*.h")
file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS "src/*.ui")

add_executable(JARVIS
    ${SRC_FILES}
    ${HDR_FILES}
    ${UI_FILES}
)

target_include_directories(JARVIS PRIVATE
    src
    src/ui
    src/ui/widgets
    src/ai
)

# Хедери з вихідного коду
target_include_directories(JARVIS PRIVATE 
    src 
    ${llama_cpp_headers_SOURCE_DIR}/include
    ${llama_cpp_headers_SOURCE_DIR}/ggml/include
)

# Лінкуємось напряму до MSVC DLL (MinGW це вміє для C-API)
# Важливо: використовуємо шлях до DLL в папці libs/llama-bin
set(LLAMA_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/llama-bin")
target_link_libraries(JARVIS PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::WebSockets
    "${LLAMA_BIN_DIR}/llama.dll"
)

# Великий стек потрібний для llama.cpp.
if (MINGW)
    target_link_options(JARVIS PRIVATE -Wl,--stack,67108864)
endif()

# ПІСЛЯ ЗБІРКИ: Копіюємо всі DLL (llama.dll, ggml.dll тощо) в папку з екзешником
add_custom_command(TARGET JARVIS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LLAMA_BIN_DIR}/"
        "$<TARGET_FILE_DIR:JARVIS>/"
    COMMENT "Copying llama.cpp runtime DLLs to output directory..."
)
