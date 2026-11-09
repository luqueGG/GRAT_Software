# GRAT_Software — CMake build definition.
# C++20 desktop application using Qt6 (Widgets + SQL) over SQLite, built with Ninja.

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

# --- Qt6 dependencies ---
find_package(Qt6 6 COMPONENTS Widgets Sql REQUIRED)

# --- Application target ---
add_executable(grat_software
    src/main.cpp
    src/core/db/database.cpp
    src/core/crud/table_specs.cpp
    src/core/crud/table_gateway.cpp
    src/core/crud/validator.cpp
    src/core/auth/auth_service.cpp
    src/core/auth/permission_checker.cpp
    src/ui/login_dialog.cpp
    src/ui/login_dialog.ui
    src/ui/main_window.cpp
    src/ui/main_window.ui
    src/ui/register_form.cpp
    src/ui/crud_widget.cpp
    resources/grat_schema.qrc
)

target_include_directories(grat_software PRIVATE src)

target_link_libraries(grat_software PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

# Install defaults (optional, harmless if unused)
install(TARGETS grat_software DESTINATION bin)
