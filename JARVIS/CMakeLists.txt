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
file(GLOB_RECURSE UI_FILES  CONFIGURE_DEPENDS "src/*.ui")

# Resource file (icon etc.). Optional — if `resources.qrc` is missing the
# Qt resource system simply has no embedded assets and main.cpp falls back
# to looking for `assets/icon.png` next to the executable.
set(QRC_FILES "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
    list(APPEND QRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
endif()

# WIN32 hides the console window on Windows GUI builds — without it the
# executable launches with a stray cmd.exe-style console, which is also
# the #1 visual indicator that "the .exe didn't bundle Qt DLLs" because
# crashes there are silent.
if(WIN32)
    add_executable(JARVIS WIN32
        ${SRC_FILES}
        ${HDR_FILES}
        ${UI_FILES}
        ${QRC_FILES}
    )
else()
    add_executable(JARVIS
        ${SRC_FILES}
        ${HDR_FILES}
        ${UI_FILES}
        ${QRC_FILES}
    )
endif()

target_include_directories(JARVIS PRIVATE
    src
    src/ui
    src/ui/widgets
    src/ai
    src/net
)

# Хедери з вихідного коду llama.cpp
target_include_directories(JARVIS PRIVATE
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

# ПІСЛЯ ЗБІРКИ: Копіюємо `assets/icon.png` поряд з екзешником (бекап на
# випадок, якщо .qrc не зібрано — main.cpp шукає файл у applicationDirPath).
add_custom_command(TARGET JARVIS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:JARVIS>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
        "$<TARGET_FILE_DIR:JARVIS>/assets/icon.png"
    COMMENT "Copying icon.png alongside executable..."
)

# =============================================================================
#  WINDEPLOYQT + ручне копіювання Qt DLL — щоб .exe запускався скрізь
# =============================================================================
#
# Це основна причина, чому збірка JARVIS, запущена на одному ПК, не стартує
# на іншому: Qt DLL (Qt6Core.dll, Qt6Widgets.dll тощо), плагін платформи
# (`platforms/qwindows.dll`), стилі та плагіни форматів зображень не
# знаходяться поряд з .exe. windeployqt — це утиліта з Qt, яка повинна
# вирішувати цю проблему автоматично.
#
# АЛЕ: у деяких інсталяціях Qt (особливо у Qt 6.10+ MinGW без галочки
# "Debug Libraries") windeployqt падає з помилкою "Unable to find the
# platform plugin" в режимі Debug — і тягне за собою всю збірку.
# Тому ми запускаємо windeployqt через CMake-обгортку, що ковтає помилку,
# а потім ще й вручну копіюємо набір DLL/плагінів, які точно знаємо що
# потрібні. Якщо обидва кроки спрацюють — буде дублікат файлів (нічого
# страшного, copy_if_different). Якщо windeployqt зламається — ручний
# копіювач все одно покладе мінімум, потрібний для запуску.
if(WIN32)
    # Шукаємо windeployqt.exe поряд з qmake.exe.
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
    if(NOT DEFINED WINDEPLOYQT_EXECUTABLE)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    endif()

    # -- (1) windeployqt через обгортку (не-фатальний) --
    if(WINDEPLOYQT_EXECUTABLE)
        # --no-translations, --no-system-d3d-compiler — економимо ~70 MB.
        # --compiler-runtime копіює VC++ Redist DLL якщо знайде їх у Qt SDK.
        set(_wdq_args
            "--no-translations --no-system-d3d-compiler --no-opengl-sw "
            "--compiler-runtime $<IF:$<CONFIG:Debug>,--debug,--release> "
            "\"$<TARGET_FILE:JARVIS>\""
        )
        string(JOIN "" _wdq_args_joined ${_wdq_args})
        add_custom_command(TARGET JARVIS POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                "-DWINDEPLOYQT=${WINDEPLOYQT_EXECUTABLE}"
                "-DWINDEPLOYQT_ARGS=${_wdq_args_joined}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_windeployqt.cmake"
            COMMENT "Running windeployqt (помилки не валять збірку)..."
        )
    else()
        message(WARNING
            "windeployqt не знайдено! Спробуй задати -DWINDEPLOYQT_EXECUTABLE=<шлях>. "
            "Ручний копіювач Qt DLL нижче все одно спрацює, якщо знайде Qt bin.")
    endif()

    # -- (2) Ручний копіювач Qt DLL --
    # Перебираємо модулі, з якими лінкується JARVIS, і копіюємо DLL у
    # каталог .exe. Підбираємо Debug-варіант (Qt6Coreed.dll) тільки якщо
    # він реально існує у Qt bin; інакше беремо Release (рятує випадок коли
    # людина встановила Qt без Debug-бібліотек).
    foreach(_lib Core Gui Widgets Network Sql WebSockets)
        set(_rel "${_qt_bin_dir}/Qt${QT_VERSION_MAJOR}${_lib}.dll")
        set(_dbg "${_qt_bin_dir}/Qt${QT_VERSION_MAJOR}${_lib}d.dll")
        if(EXISTS "${_dbg}" AND EXISTS "${_rel}")
            set(_pick "$<IF:$<CONFIG:Debug>,${_dbg},${_rel}>")
        elseif(EXISTS "${_dbg}")
            set(_pick "${_dbg}")
        elseif(EXISTS "${_rel}")
            set(_pick "${_rel}")
        else()
            set(_pick "")
        endif()
        if(_pick)
            add_custom_command(TARGET JARVIS POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_pick}" "$<TARGET_FILE_DIR:JARVIS>/"
                COMMENT "Copying Qt${QT_VERSION_MAJOR}${_lib}.dll"
            )
        endif()
    endforeach()

    # -- (3) Платформний плагін (qwindows.dll / qwindowsd.dll) --
    # Без цього файлу Qt при запуску .exe пише "Could not load Qt platform
    # plugin "windows"". Це другий обов'язковий файл після Qt6Core.dll.
    set(_qt_plat_dir "${_qt_root_dir}/plugins/platforms")
    if(EXISTS "${_qt_plat_dir}/qwindows.dll" OR EXISTS "${_qt_plat_dir}/qwindowsd.dll")
        if(EXISTS "${_qt_plat_dir}/qwindowsd.dll" AND EXISTS "${_qt_plat_dir}/qwindows.dll")
            set(_qwin_pick "$<IF:$<CONFIG:Debug>,${_qt_plat_dir}/qwindowsd.dll,${_qt_plat_dir}/qwindows.dll>")
        elseif(EXISTS "${_qt_plat_dir}/qwindowsd.dll")
            set(_qwin_pick "${_qt_plat_dir}/qwindowsd.dll")
        else()
            set(_qwin_pick "${_qt_plat_dir}/qwindows.dll")
        endif()
        add_custom_command(TARGET JARVIS POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:JARVIS>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qwin_pick}" "$<TARGET_FILE_DIR:JARVIS>/platforms/"
            COMMENT "Copying platforms plugin..."
        )
    endif()

    # -- (4) Стилі (qmodernwindowsstyle.dll) — інакше Qt падає у деяких темах --
    if(EXISTS "${_qt_root_dir}/plugins/styles")
        add_custom_command(TARGET JARVIS POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:JARVIS>/styles"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_qt_root_dir}/plugins/styles"
                "$<TARGET_FILE_DIR:JARVIS>/styles"
            COMMENT "Copying Qt styles plugins..."
        )
    endif()

    # -- (5) TLS-бекенд (qopensslbackend.dll) — для HTTPS у ModelDownloadDialog --
    if(EXISTS "${_qt_root_dir}/plugins/tls")
        add_custom_command(TARGET JARVIS POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:JARVIS>/tls"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_qt_root_dir}/plugins/tls"
                "$<TARGET_FILE_DIR:JARVIS>/tls"
            COMMENT "Copying Qt TLS plugins..."
        )
    endif()

    # -- (6) imageformats — для PNG-іконки --
    if(EXISTS "${_qt_root_dir}/plugins/imageformats")
        add_custom_command(TARGET JARVIS POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:JARVIS>/imageformats"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_qt_root_dir}/plugins/imageformats"
                "$<TARGET_FILE_DIR:JARVIS>/imageformats"
            COMMENT "Copying Qt imageformats plugins..."
        )
    endif()

    # -- (7) MinGW runtime DLLs (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll) --
    foreach(_mingw_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        if(EXISTS "${_qt_bin_dir}/${_mingw_dll}")
            add_custom_command(TARGET JARVIS POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qt_bin_dir}/${_mingw_dll}"
                    "$<TARGET_FILE_DIR:JARVIS>/"
                COMMENT "Copying ${_mingw_dll}..."
            )
        endif()
    endforeach()
endif()

# =============================================================================
#  INSTALL — для збору портативного релізу
# =============================================================================
#
# Дозволяє запакувати готовий до роздачі реліз через `cmake --install . --prefix dist`.
# Кладе .exe, всі DLL з libs/llama-bin/, assets/, та повертає windeployqt
# результати поруч з .exe.
install(TARGETS JARVIS
    RUNTIME DESTINATION .
)
install(DIRECTORY "${LLAMA_BIN_DIR}/"
    DESTINATION .
    FILES_MATCHING
        PATTERN "*.dll"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    DESTINATION .
)
