include_guard(GLOBAL)

# P6 will provide grat_core. Until then, keep P1 buildable on the current target.
if(TARGET grat_core)
    set(_grat_architecture_target grat_core)
else()
    set(_grat_architecture_target grat_software)
endif()

target_sources(${_grat_architecture_target} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core/crud/controllers/child_crud_controller.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core/crud/controllers/crud_controller.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core/crud/repositories/sql_record_repository.cpp"
)
unset(_grat_architecture_target)
