// ============================================================================
// modules.cpp - Archivo de enlace para compilar módulos en subdirectorio
// ============================================================================
// Arduino IDE no compila automáticamente archivos .cpp en subdirectorios.
// Este archivo incluye las implementaciones para que el linker las encuentre.
// ============================================================================

#include "modules/KissLTEModule.cpp"
#include "modules/QuectelModule.cpp"
#include "modules/SIMCOMModule.cpp"
