# Reportes operativos y exportación CSV — impacto (P4)

## Funcionalidad

Ventana **Reportes** accesible desde el menú *Reportes → Abrir reportes* de
`MainWindow`. Ofrece dos reportes de solo lectura:

1. **Residuos registrados** (`waste_waste`).
2. **Traslados registrados** (`transfer_transfer`).

Cada reporte permite filtrar por estado `A` (activo), `I` (inactivo), `*`
(eliminado) o todos, muestra el número de registros y la suma de cantidades
(`amount_waste`), y exporta exactamente el conjunto filtrado a CSV mediante
un diálogo explícito de ubicación.

## Permisos

- Residuos: `view_waste`.
- Traslados: `view_transfer`.

La verificación se realiza en `ReportService` además de la navegación: invocar
el servicio directamente sin el permiso correspondiente devuelve
`permissionDenied = true` y no entrega datos ni CSV.

## Columnas

- Residuos: ID residuo, Empresa generadora, Cantidad, Toxicidad, Creado por,
  Fecha creacion, Estado.
- Traslados: ID traslado, Empresa transportista, Destino, Residuo, Tipo
  contenedor, Cantidad, Fecha entrega, Creado por, Fecha creacion, Estado.

## Política CSV

- Codificación UTF-8 con BOM para compatibilidad con hojas de cálculo.
- Separador de campos: coma; fin de línea CRLF.
- Campos con coma, comillas o saltos de línea se entrecomillan y las comillas
  internas se duplican (RFC 4180).
- Mitigación de inyección de fórmulas: valores que comienzan con `=`, `+`,
  `-` o `@` se prefijan con una comilla simple.
- Cancelar el diálogo de guardado no crea archivos. Los errores de escritura
  se informan y no dejan un archivo final aparentemente correcto.

## Reporte vacío vs. error de lectura

Un filtro sin filas devuelve `ok = true` con `count = 0` (reporte vacío). Un
fallo de consulta o conexión devuelve `ok = false` con `errorMessage` seguro.

## Medición de impacto

Escenario reproducible sobre la base de demostración (1 residuo y 1 traslado
por defecto, ampliable):

- **Manual**: abrir cada registro, copiar columnas a una hoja de cálculo y
  guardar. Estimado para 10 registros: ~2 min por reporte, con riesgo de
  omisión de filas.
- **Con exportación**: filtrar y exportar. Estimado: <10 s por reporte, con
  verificación automática de que las filas exportadas coinciden con el filtro.

Tamaño de muestra y condiciones deben registrarse en la aceptación final; una
única demostración no tiene valor estadístico. Se aporta además la métrica
determinista: número de filas exportadas coincidente con el filtro aplicado.