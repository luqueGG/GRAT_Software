include_guard(GLOBAL)

# Use the application target until grat_core is available.
if(TARGET grat_core)
    set(_grat_validation_target grat_core)
else()
    set(_grat_validation_target grat_software)
endif()

target_sources(${_grat_validation_target} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core/crud/validation/field_validation_rules.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/core/crud/validation/table_gateway_validation_lookup.cpp"
)
unset(_grat_validation_target)
