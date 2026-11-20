# tests/reportes/CMakeLists.txt — P4 own tests.

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_reportes
    test_reportes.cpp
    ../../src/core/reports/report_service.cpp
    ../../src/core/db/database.cpp
    ../../src/core/errors/error_handler.cpp
    ../../src/core/auth/permission_checker.cpp
)

target_include_directories(test_reportes PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(test_reportes PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Sql
    Qt6::Svg
)

add_test(NAME test_reportes COMMAND test_reportes)