# tests/core/CMakeLists.txt — core test suite (P3).
#
# Qt Test executables registered with CTest. Every suite uses temporary SQLite
# databases or in-memory doubles; none opens GRAT_Software/db/grat.db.
#
# The non-visual sources come from P6's reusable 'grat_core' target when it
# exists. Until then this directory builds an equivalent object library with the
# same core sources, so the suite runs on the current main without editing the
# main CMakeLists.txt.

find_package(Qt6 REQUIRED COMPONENTS Core Sql Test)

set(GRAT_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

if(TARGET grat_core)
    set(GRAT_TEST_CORE_TARGET grat_core)
else()
    add_library(grat_test_core OBJECT
        ${GRAT_APP_DIR}/src/core/db/database.cpp
        ${GRAT_APP_DIR}/src/core/errors/error_handler.cpp
        ${GRAT_APP_DIR}/src/core/crud/table_specs.cpp
        ${GRAT_APP_DIR}/src/core/crud/table_gateway.cpp
        ${GRAT_APP_DIR}/src/core/crud/validator.cpp
        ${GRAT_APP_DIR}/src/core/crud/validation/field_validation_rules.cpp
        ${GRAT_APP_DIR}/src/core/crud/validation/record_validation_rules.cpp
        ${GRAT_APP_DIR}/src/core/crud/validation/table_gateway_validation_lookup.cpp
        ${GRAT_APP_DIR}/src/core/crud/controllers/crud_controller.cpp
        ${GRAT_APP_DIR}/src/core/crud/controllers/child_crud_controller.cpp
        ${GRAT_APP_DIR}/src/core/crud/repositories/sql_record_repository.cpp
        ${GRAT_APP_DIR}/src/core/auth/auth_service.cpp
        ${GRAT_APP_DIR}/src/core/auth/permission_checker.cpp
        ${GRAT_APP_DIR}/resources/grat_schema.qrc
    )
    target_include_directories(grat_test_core PUBLIC ${GRAT_APP_DIR}/src)
    target_link_libraries(grat_test_core PUBLIC Qt6::Core Qt6::Sql)
    set(GRAT_TEST_CORE_TARGET grat_test_core)
endif()

# grat_add_core_test(<name> <sources...>) — one Qt Test executable per suite.
function(grat_add_core_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GRAT_APP_DIR}/src
    )
    target_link_libraries(${name} PRIVATE ${GRAT_TEST_CORE_TARGET} Qt6::Core Qt6::Sql Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    # Headless and isolated from the developer's working directory.
    set_tests_properties(${name} PROPERTIES
        LABELS core
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

grat_add_core_test(test_core_fixtures test_core_fixtures.cpp)
grat_add_core_test(test_validator test_validator.cpp)
grat_add_core_test(test_permissions test_permissions.cpp)
grat_add_core_test(test_controllers test_controllers.cpp)
grat_add_core_test(test_gateway test_gateway.cpp)
