# cmake/features/reportes.cmake — fuentes del frente de reportes (P4).
# Fuentes sin interfaz -> grat_core; fuentes visuales -> grat_software.
# Consumido por el CMake principal cuando la infraestructura de P6 este integrada.

set(GRAT_REPORTES_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/reports/report_service.cpp
)

set(GRAT_REPORTES_UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/reports/reports_widget.cpp
)