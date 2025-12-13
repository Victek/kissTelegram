// KissJson.cpp
// Vicente Soriano - victek@gmail.com
// Implementación helpers JSON - Zero allocations

#include "KissJson.h"
#include "lang.h"

// ========== BÚSQUEDA Y PARSEO ==========

int KissJson::findValue(const char* json, const char* key, int startPos) {
  if (!json || !key) return -1;
  
  // Construir patrón: "key":
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  
  const char* pos = findPattern(json, pattern, startPos);
  if (!pos) return -1;
  
  // Avanzar después de ":"
  int offset = (pos - json) + strlen(pattern);
  
  // Saltar espacios
  return skipWhitespace(json, offset);
}

bool KissJson::extractString(const char* json, int startPos, char* buffer, size_t bufferSize) {
  if (!json || !buffer || bufferSize == 0) return false;
  
  // Debe empezar con comilla
  if (json[startPos] != '"') {
    buffer[0] = '\0';
    return false;
  }
  
  startPos++; // Saltar comilla inicial
  size_t pos = 0;
  bool escaped = false;
  
  while (json[startPos] != '\0' && pos < bufferSize - 1) {
    char c = json[startPos++];
    
    if (escaped) {
      // Convertir escape sequences de vuelta
      if (c == 'n') buffer[pos++] = '\n';
      else if (c == 'r') buffer[pos++] = '\r';
      else if (c == 't') buffer[pos++] = '\t';
      else if (c == '\\') buffer[pos++] = '\\';
      else if (c == '"') buffer[pos++] = '"';
      else buffer[pos++] = c;  // Otros escapes
      escaped = false;
      continue;
    }
    
    if (c == '\\') {
      // Inicio de escape
      escaped = true;
      continue;
    }
    
    if (c == '"') {
      // Fin de string
      buffer[pos] = '\0';
      return true;
    }
    
    buffer[pos++] = c;
  }
  
  // Truncado o sin cerrar
  buffer[pos] = '\0';
  return false;
}

int KissJson::extractInt(const char* json, int startPos) {
  if (!json) return 0;
  
  int result = 0;
  bool negative = false;
  int pos = startPos;
  
  // Manejar signo
  if (json[pos] == '-') {
    negative = true;
    pos++;
  }
  
  // Extraer dígitos
  while (json[pos] >= '0' && json[pos] <= '9') {
    result = result * 10 + (json[pos] - '0');
    pos++;
  }
  
  return negative ? -result : result;
}

unsigned long KissJson::extractULong(const char* json, int startPos) {
  if (!json) return 0;
  
  unsigned long result = 0;
  int pos = startPos;
  
  while (json[pos] >= '0' && json[pos] <= '9') {
    result = result * 10 + (json[pos] - '0');
    pos++;
  }
  
  return result;
}

int KissJson::findNextMessage(const char* json, int startPos) {
  if (!json) return -1;
  
  const char* pos = findPattern(json, "{\"i\":", startPos);
  return pos ? (pos - json) : -1;
}

// ========== CONSTRUCCIÓN JSON ==========

bool KissJson::escapeText(const char* input, char* output, size_t outputSize) {
  if (!input || !output || outputSize < 2) return false;
  
  size_t outPos = 0;
  size_t inPos = 0;
  
  while (input[inPos] != '\0' && outPos < outputSize - 2) {
    char c = input[inPos++];
    
    // Caracteres que necesitan escape
    if (c == '"') {
      if (outPos + 2 >= outputSize) return false;
      output[outPos++] = '\\';
      output[outPos++] = '"';
    } else if (c == '\\') {
      if (outPos + 2 >= outputSize) return false;
      output[outPos++] = '\\';
      output[outPos++] = '\\';
    } else if (c == '\n') {
      if (outPos + 2 >= outputSize) return false;
      output[outPos++] = '\\';
      output[outPos++] = 'n';
    } else if (c == '\r') {
      if (outPos + 2 >= outputSize) return false;
      output[outPos++] = '\\';
      output[outPos++] = 'r';
    } else if (c == '\t') {
      if (outPos + 2 >= outputSize) return false;
      output[outPos++] = '\\';
      output[outPos++] = 't';
    } else if (c < 32) {
      // Caracteres control - ignorar
      continue;
    } else {
      output[outPos++] = c;
    }
  }
  
  output[outPos] = '\0';
  return (input[inPos] == '\0'); // true si procesó todo
}

bool KissJson::buildMessage(char* buffer, size_t bufferSize,
                            uint32_t id, const char* chatId, const char* text,
                            int priority, int sent, unsigned long timestamp) {
  if (!buffer || bufferSize < 100 || !chatId || !text) return false;
  
  // HEAP en vez de stack para escapedText, es más efectivo, joder...
  char* escapedText = (char*)malloc(512);
  if (!escapedText) {
    KISS_LOG("❌ Error malloc escapedText");
    return false;
  }
  
  if (!escapeText(text, escapedText, 512)) {
    KISS_LOG("⚠️ Texto truncado en escape");
  }
  
  // Construir JSON
  int written = snprintf(buffer, bufferSize,
                        "{\"i\":%u,\"c\":\"%s\",\"t\":\"%s\",\"p\":%d,\"s\":%d,\"ts\":%lu}",
                        id, chatId, escapedText, priority, sent, timestamp);
  
  // Liberar memoria de una vez .. ya llevas 12KB desperdiciados..
  free(escapedText);
  
  return (written > 0 && written < (int)bufferSize);
}

// ========== VALIDACIÓN ==========

bool KissJson::isValid(const char* json) {
  if (!json) return false;
  
  int braceCount = 0;
  int bracketCount = 0;
  bool inString = false;
  bool escaped = false;
  
  for (int i = 0; json[i] != '\0'; i++) {
    char c = json[i];
    
    if (escaped) {
      escaped = false;
      continue;
    }
    
    if (c == '\\') {
      escaped = true;
      continue;
    }
    
    if (c == '"') {
      inString = !inString;
      continue;
    }
    
    if (inString) continue;
    
    if (c == '{') braceCount++;
    else if (c == '}') braceCount--;
    else if (c == '[') bracketCount++;
    else if (c == ']') bracketCount--;
    
    // Detectar desbalance
    if (braceCount < 0 || bracketCount < 0) return false;
  }
  
  return (braceCount == 0 && bracketCount == 0 && !inString);
}

int KissJson::countObjects(const char* json, const char* arrayName) {
  if (!json || !arrayName) return 0;
  
  // Buscar inicio del array
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\":[", arrayName);
  
  const char* arrayStart = findPattern(json, pattern, 0);
  if (!arrayStart) return 0;
  
  int count = 0;
  int pos = (arrayStart - json) + strlen(pattern);
  
  // Contar objetos {
  while (json[pos] != '\0' && json[pos] != ']') {
    if (json[pos] == '{') count++;
    pos++;
  }
  
  return count;
}

// ========== UTILIDADES ==========

int KissJson::findObjectEnd(const char* json, int startPos) {
  if (!json || json[startPos] != '{') return -1;
  
  int braceCount = 0;
  bool inString = false;
  bool escaped = false;
  
  for (int i = startPos; json[i] != '\0'; i++) {
    char c = json[i];
    
    if (escaped) {
      escaped = false;
      continue;
    }
    
    if (c == '\\') {
      escaped = true;
      continue;
    }
    
    if (c == '"') {
      inString = !inString;
      continue;
    }
    
    if (inString) continue;
    
    if (c == '{') braceCount++;
    else if (c == '}') {
      braceCount--;
      if (braceCount == 0) return i;
    }
  }
  
  return -1;
}

int KissJson::skipWhitespace(const char* json, int startPos) {
  if (!json) return startPos;
  
  int pos = startPos;
  while (json[pos] == ' ' || json[pos] == '\t' || 
         json[pos] == '\n' || json[pos] == '\r') {
    pos++;
  }
  
  return pos;
}

// ========== PRIVADOS ==========

const char* KissJson::findPattern(const char* haystack, const char* needle, int startPos) {
  if (!haystack || !needle) return nullptr;
  
  const char* searchStart = haystack + startPos;
  return strstr(searchStart, needle);
}