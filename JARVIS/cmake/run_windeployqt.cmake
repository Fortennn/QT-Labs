# Helper run as `cmake -P` from a POST_BUILD step.
#
# windeployqt is best-effort: it tries to figure out which Qt plugins the
# .exe needs and copies them. In some installs (e.g. Qt 6.10.x MinGW
# without the Debug components installed) it errors out with
# "Unable to find the platform plugin" — which by itself would otherwise
# kill the whole build. This wrapper swallows the non-zero exit code so
# the build still completes (we have a separate manual DLL copy step in
# the main CMakeLists.txt that fills in the gaps).
#
# Inputs:
#   WINDEPLOYQT     — absolute path to windeployqt.exe
#   WINDEPLOYQT_ARGS — semicolon-separated list of args (CMake list)

if(NOT WINDEPLOYQT)
    message(STATUS "[run_windeployqt] WINDEPLOYQT not set, skipping.")
    return()
endif()

if(NOT EXISTS "${WINDEPLOYQT}")
    message(STATUS "[run_windeployqt] '${WINDEPLOYQT}' not found, skipping.")
    return()
endif()

separate_arguments(_args NATIVE_COMMAND "${WINDEPLOYQT_ARGS}")

execute_process(
    COMMAND "${WINDEPLOYQT}" ${_args}
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE  _stderr
)

if(_rc EQUAL 0)
    message(STATUS "[run_windeployqt] OK")
else()
    message(STATUS
        "[run_windeployqt] exited ${_rc} — continuing without failing the build.")
    if(_stdout)
        message(STATUS "stdout:\n${_stdout}")
    endif()
    if(_stderr)
        message(STATUS "stderr:\n${_stderr}")
    endif()
endif()
