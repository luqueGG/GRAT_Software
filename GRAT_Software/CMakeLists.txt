# GRAT_Software — CMake build definition.
# C++20 desktop application using Qt6 (Widgets + SQL + Svg) over SQLite, built with Ninja.

cmake_minimum_required(VERSION 3.16)

project(GRAT_Software
    VERSION 0.1.0
    LANGUAGES CXX
)

# --- C++20 ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt auto-processing of MOC / UIC / RCC ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --- Qt6 dependencies (always required by the application) ---
find_package(Qt6 6 COMPONENTS Widgets Sql Svg REQUIRED)

# --- Core library: non-visual business logic and data access ---
# Convention agreed with the team:
#   * sources without UI dependency -> grat_core
#   * visual sources, main and .qrc resources -> grat_software
# Paths in this file are relative to GRAT_Software/.
add_library(grat_core STATIC
    src/app/session.h
    src/core/db/database.cpp
    src/core/db/database.h
    src/core/errors/error_handler.cpp
    src/core/errors/error_handler.h
    src/core/auth/auth_service.cpp
    src/core/auth/auth_service.h
    src/core/auth/permission_checker.cpp
    src/core/auth/permission_checker.h
    src/core/crud/table_specs.cpp
    src/core/crud/table_spec.h
    src/core/crud/table_gateway.cpp
    src/core/crud/table_gateway.h
    src/core/crud/validator.cpp
    src/core/crud/validator.h
    src/core/crud/controllers/crud_controller.cpp
    src/core/crud/controllers/crud_controller.h
    src/core/crud/controllers/child_crud_controller.cpp
    src/core/crud/controllers/child_crud_controller.h
    src/core/crud/repositories/i_record_repository.h
    src/core/crud/repositories/sql_record_repository.cpp
    src/core/crud/repositories/sql_record_repository.h
    src/core/crud/validation/field_validation_rules.cpp
    src/core/crud/validation/field_validation_rules.h
    src/core/crud/validation/record_validation_rules.cpp
    src/core/crud/validation/record_validation_rules.h
    src/core/crud/validation/table_gateway_validation_lookup.cpp
    src/core/crud/validation/table_gateway_validation_lookup.h
    src/core/crud/validation/i_validation_lookup.h
    src/core/crud/validation/i_validation_rule.h
    src/core/reports/report_service.cpp
    src/core/reports/report_service.h
)

target_include_directories(grat_core PUBLIC src)

target_link_libraries(grat_core PUBLIC
    Qt6::Core
    Qt6::Sql
)

target_compile_options(grat_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# --- Application target: executable with UI, main and resources ---
add_executable(grat_software
    src/main.cpp
    src/ui/login_dialog.cpp
    src/ui/login_dialog.h
    src/ui/login_dialog.ui
    src/ui/main_window.cpp
    src/ui/main_window.h
    src/ui/main_window.ui
    src/ui/register_form.cpp
    src/ui/register_form.h
    src/ui/crud_widget.cpp
    src/ui/crud_widget.h
    src/ui/master_detail_widget.cpp
    src/ui/master_detail_widget.h
    src/ui/admin_roles_widget.cpp
    src/ui/admin_roles_widget.h
    src/ui/reports/reports_widget.cpp
    src/ui/reports/reports_widget.h
    src/ui/visual_icons.cpp
    src/ui/visual_icons.h
    src/ui/visual_text.cpp
    src/ui/visual_text.h
    resources/grat_schema.qrc
    resources/grat_resources.qrc
)

target_link_libraries(grat_software PRIVATE
    grat_core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Svg
)

target_compile_options(grat_software PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Install defaults (optional, harmless if unused)
install(TARGETS grat_software DESTINATION bin)

# --- Optional feature extensions (owned by P1, P2 and P4) ---
# Each extension may add sources to grat_core (non-visual) or grat_software (visual).
# Missing files do not break the build so that partial deliveries can still compile.
foreach(_grat_feature IN ITEMS arquitectura validacion reportes)
    set(_grat_feature_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/features/${_grat_feature}.cmake")
    if(EXISTS "${_grat_feature_file}")
        include("${_grat_feature_file}")
    endif()
endforeach()
unset(_grat_feature)
unset(_grat_feature_file)

# --- Test infrastructure ---
# CTest provides BUILD_TESTING; Qt Test is only required when tests are enabled.
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
