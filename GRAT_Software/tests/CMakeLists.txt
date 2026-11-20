# tests/CMakeLists.txt — entry point for the test suites (P6 infra).
# Incorporates available subdirectories; missing ones do not fail the build.

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/CMakeLists.txt")
    add_subdirectory(core)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/reportes/CMakeLists.txt")
    add_subdirectory(reportes)
endif()