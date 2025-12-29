// KissJson.h
// Vicente Soriano - victek@gmail.com
// Helpers JSON char - Zero memory leaks

#ifndef KISS_JSON_H
#define KISS_JSON_H

#include "kiss_setup.h"

class KissJson {
public:
  // ========== BÚSQUEDA Y PARSEO ==========
  
  // Buscar posición de una clave en JSON
  // Retorna posición del valor (después de ":") o -1 si no existe
  static int findValue(const char* json, const char* key, int startPos = 0);
  
  // Extraer string entre comillas a buffer
  // Maneja caracteres escapados (\", \\)
  static bool extractString(const char* json, int startPos, char* buffer, size_t bufferSize);
  
  // Extraer número entero
  static int extractInt(const char* json, int startPos);
  
  // Extraer unsigned long
  static unsigned long extractULong(const char* json, int startPos);
  
  // Buscar siguiente objeto mensaje {\"i\":...}
  static int findNextMessage(const char* json, int startPos = 0);
  
  // ========== CONSTRUCCIÓN JSON ==========
  
  // Escapar texto para JSON (", \, caracteres control)
  // Retorna false si buffer insuficiente
  static bool escapeText(const char* input, char* output, size_t outputSize);
  
  // Construir objeto mensaje: {"i":N,"c":"chat","t":"text","p":P,"s":S,"ts":T}
  static bool buildMessage(char* buffer, size_t bufferSize,
                           uint32_t id, const char* chatId, const char* text,
                           int priority, int sent, unsigned long timestamp);
  
  // ========== VALIDACIÓN ==========
  
  // Verificar si JSON es válido (básico: balanceo de {})
  static bool isValid(const char* json);
  
  // Contar objetos en array
  static int countObjects(const char* json, const char* arrayName);
  
  // ========== UTILIDADES ==========
  
  // Encontrar fin de objeto (} matching)
  static int findObjectEnd(const char* json, int startPos);
  
  // Saltar espacios en blanco
  static int skipWhitespace(const char* json, int startPos);
  
private:
  // Helper interno para buscar patrón
  static const char* findPattern(const char* haystack, const char* needle, int startPos);
};

#endif // KISS_JSON_H