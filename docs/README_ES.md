# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Castellano** | [Documentación](docs/GETTING_STARTED_ES.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **?PRIMERA VEZ USANDO ESP32-S3 CON KISSTELEGRAM?**
> **LEE ESTO PRIMERO:** [**GETTING_STARTED_ES.md**](docs/GETTING_STARTED_ES.md)
> ESP32-S3 requiere un **proceso de carga en dos pasos** debido a particiones personalizadas. Ignorar esta guía causará errores de arranque y particiones equivocadas!

---

## Una Biblioteca Robusta de Grado Empresarial para Bots de Telegram en ESP32-S3

KissTelegram es la **única biblioteca de Telegram para ESP32** construida desde cero para aplicaciones críticas. A diferencia de otras bibliotecas que dependen de la clase Arduino `String` (causando fragmentación de memoria y fugas), KissTelegram utiliza arrays puros de `char[]` para una estabilidad inquebrantable.

### ?Por qué KissTelegram?

- Cansado de proyectos perdidos por bibliotecas débiles, fugas de memoria, soluciones de último momento, falta de soporte, palabras vacías, términos que no funcionan, reinicios....

- Esta era mi visión y experiencia con otras librerías y este és el resultado con KissTelegram:

- **Cero Pérdida de Mensajes**: Cola persistente en LittleFS que sobrevive a bloqueos, reinicios y fallos de WiFi
- **Sin Fugas de Memoria**: Implementación pura de `char[]`, sin fragmentación de String
- **Seguridad SSL/TLS**: Conexiones seguras a la API de Telegram con validación de certificados (hasta 2035)
- **Gestión Inteligente de Energía**: 6 modos de potencia (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Prioridades de Mensajes**: CRITICAL, HIGH, NORMAL, LOW con gestión inteligente de colas
- **Modo Turbo**: Procesamiento por lotes para colas grandes de mensajes (0,9 msg/s)
- **i18n Multilingüe**: Selección de idioma del envío de mensajes en tiempo de compilación (7 idiomas, sin sobrecarga en tiempo de ejecución)
- **OTA Empresarial**: Actualizaciones de firmware de doble arranque con reversión automática y Gestión de seguridad
- **100% Utilización de Flash**: Esquema de partición personalizada que maximiza el flash de 16MB del ESP32-S3
- **Más Seguro que OTA de Espressif**: Autenticación PIN/PUK, Comprobación de suma de verificación, ventana de validación de 60s
- **Independiente de bibliotecas externas**: Todo hecho desde cero, parser JSON propio para las bibiotecas de KissTelegram.

---

## Requisitos de Hardware

- **ESP32-S3** con **16MB Flash** / **8MB PSRAM**
- Conectividad WiFi
- Arduino IDE o PlatformIO

---

## Instalaci贸n

### Arduino IDE

1. Descarga este repositorio como ZIP
2. Abre Arduino IDE 鈫?Sketch 鈫?Include Library 鈫?Add .ZIP Library
3. Selecciona el archivo descargado

### PlatformIO

A帽ade a tu `platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Esquema de Partici贸n Personalizado

KissTelegram incluye un `partitions.csv` optimizado que maximiza el uso de flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB de almacenamiento SPIFFS** - Eso son 8MB más que los esquemas predeterminados de Espressif!

Para usar este esquema de partici贸n:
1. Copia `partitions.csv` a tu directorio de proyecto
2. En Arduino IDE: Tools ->Partition Scheme ->Custom
3. En PlatformIO: `board_build.partitions = partitions.csv`

---

## Inicio R谩pido

### Ejemplo B谩sico

```cpp
#include "KissTelegram.h"
#include "KissCredentials.h"

#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

KissCredentials credentials;
KissTelegram bot(BOT_TOKEN);

void messageHandler(const char* chat_id, const char* text,
                    const char* command, const char* param) {
  if (strcmp(command, "/start") == 0) {
    bot.sendMessage(chat_id, "Hello! I'm alive!");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect WiFi
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Initialize credentials
  credentials.begin();
  credentials.setOwnerChatID(CHAT_ID);

  // Enable bot
  bot.enable();
  bot.setWifiStable();
}

void loop() {
  bot.checkMessages(messageHandler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

### Ejemplo de Actualizaciones OTA

```cpp
#include "KissOTA.h"

KissOTA* otaManager;

void fileReceivedCallback(const char* file_id, size_t file_size,
                          const char* file_name) {
  if (otaManager && strstr(file_name, ".bin")) {
    otaManager->processReceivedFile(file_id, file_size, file_name);
  }
}

void setup() {
  // ... WiFi and bot setup ...

  // Initialize OTA
  otaManager = new KissOTA(&bot, &credentials);
  bot.onFileReceived(fileReceivedCallback);
}

void loop() {
  bot.checkMessages(messageHandler);
  bot.processQueue();

  if (otaManager) {
    otaManager->loop();
  }

  delay(bot.getRecommendedDelay());
}
```

**Proceso OTA:**
1. Envíaa `/ota` a tu bot
2. Introduce el PIN con `/otapin YOUR_PIN`
3. Carga el archivo de firmware `.bin`
4. El bot verifica la suma de verificaci贸n autom谩ticamente
5. Confirma con `/otaconfirm`
6. Después de reiniciar, valida con `/otaok` dentro de 60 segundos
7. Reversión automática si la validación falla!

- Lee Readme_KissOTA.md en tu idioma preferido para saber más sobre la solución.

---

## Características Clave Explicadas

### 1. Cola de Mensajes Persistente

Los mensajes se almacenan en LittleFS con eliminación automática por lotes:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sobrevive a bloqueos, desconexiones de WiFi, reinicios
- Reintentos automáticos de envíos fallidos
- Eliminación inteligente por lotes (cada 10 mensajes + cuando la cola está vacía)
- Garantía de cero pérdida de mensajes

### 2. Gesti贸n de Energ铆a

6 modos de energía inteligentes se adaptan a las necesidades de tu aplicación:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase de inicio inicial (10s)
- **POWER_LOW**: Actividad mínima, sondeo lento
- **POWER_IDLE**: Sin actividad reciente, comprobaciones reducidas
- **POWER_ACTIVE**: Operación normal
- **POWER_TURBO**: Procesamiento por lotes de alta velocidad (intervalos de 50ms)
- **POWER_MAINTENANCE**: Anulación manual para actualizaciones
- **Decay Timing para cambios suave**

### 3. Prioridades de Mensajes

Cuatro niveles de prioridad aseguran que los mensajes críticos se envíen primero saltando sobre los de menor prioridad:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La cola procesa: **CRITICAL /HIGH /NORMAL /LOW**
Procesos internos: **OTAMODE /MAINTENANCEMODE**

### 4. Seguridad SSL/TLS

Conexiones seguras con validación de certificados (hasta 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback automático entre seguro e inseguro
- Comprobaciones periódicas de ping para mantener la conexión
- Código de conexión reutilizable ahorra encabezado de conexión para máximo rendimiento

### 5. Modo Turbo

Se activa automáticamente cuando se envían lotes usando el comando /llenar:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Procesa 10 mensajes por ciclo
- Intervalos de 50ms entre lotes
- Logra rendimiento de 0.9 msg/s
- Se desactiva automáticamente cuando la cola se ha enviado

### 6. Modos de Operación

Perfiles preconfigurados para diferentes escenarios:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Predeterminado (sondeo: 10s, reintento: 3)
- **MODE_PERFORMANCE**: Rápido (sondeo: 5s, reintento: 2)
- **MODE_POWERSAVE**: Lento (sondeo: 30s, reintento: 2)
- **MODE_RELIABILITY**: Robusto (sondeo: 15s, reintento: 5)

### 7. Diagnósticos

Monitoreo y depuración completos:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Muestra:
- Memoria libre (heap/PSRAM)
- Estadísticas de cola de mensajes
- Calidad de conexión
- Historial de modo de energía
- Uso de almacenamiento
- Tiempo de actividad

---

## 8. Gestión de WiFi
- WiFi Manager integrado activa otras tareas solo hasta que WiFi sea estable
- Previene condiciones de carrera
- Reclama mensajes en curso para ir al almacenamiento FS hasta que se restablezca la conexión, puede mantener hasta 3500 msg (predeterminado pero fácilmente expandible, depende de cuánto espacio quiera utilizar)
- Monitoreo de calidad de conexión (EXCELLENT, GOOD, FAIR, POOR, DEAD) y nivel de salida RSSI
- Solo tienes que ocuparte de tu sketch, a?ade tú código y KissTelegram se ocupa de las tareas críticas, WiFi, SSL, Mensajes, OTA, Gestión Energía, Prioridades.. la de trabajo que te ahorras....

---

## 9. Característica Clave: Comando `/estado`

**La herramienta de depuración más poderosa que siempre vas a incluir en tu skecth**

Envía `/estado` a tu bot y obtendrás un **informe completo de la salud de tu sketch** al momento, (disponible en 7 idiomas):

```
KissTelegram_test, [15/12/2025 13:11]
?? KissTelegram v0.9.0
?? Build: Dec 15 2025 13:10:23 (0xD984C13E)

?? FIABILIDAD SISTEMA
? Sistema: FIABLE
? Mensajes enviados: 940
?? Mensajes pendientes: 70
? Mensajes perdidos: 0
?? Descartes (cola llena): 0

?? ADVERSIDADES EXTERNAS
?? Errores totales: 0
?? Recuperados (fallback): 0
?? Caídas WiFi: 0

?? INFORMACIóN TéCNICA
?? Tiempo funcionamiento: 0h 0m
?? RAM libre: 223960 bytes
?? PSRAM libre: 1027820 bytes
?? FS libre: 13549568 bytes
?? Max. en FS: 3500 Mensajes
?? Modo Energía: 3 
?? Se?al WiFi: -64 dBm (Regular)
?? SSL: SEGURO
?? Turbo: INACTIVO
?? Auto-mensajes: SI

```

**Por qué `/estado` es esencial:**
- Verificación instantánea de salud del sistema
- Monitoreo de calidad de WiFi (diagnostica problemas de conectividad)
- Detección de fugas de memoria (observa el heap libre)
- Estado de la cola de mensajes (ve mensajes pendientes/fallidos)
- Seguimiento de tiempo de actividad (monitoreo de estabilidad)
- Tu primera herramienta de diagnóstico

**Consejo profesional:** Haz que `/estado` sea tu primer mensaje después de cada actualización de firmware para verificar que todo funcione!

---

## 10. NTP
- Código propio para sincronizar/resincronizar para SSL. GNSS, LTE y Scheduler (Enterprise Edition)
---

## 11. Documentación (7 Idiomas)

- **[GETTING_STARTED_ES.md](docs/GETTING_STARTED_ES.md)** - **COMIENZA AQUí!** Guía completa desde que recibes el ESP32-S3 hasta el primer mensaje envíado a Telegram
- **[README_ES.md](docs/README_ES.md)** (este archivo) - Descripción general de características, inicio rápido, referencia de API
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Comparación técnica con 6 bibliotecas de Telegram (solo en inglés, pero és autoexplicativo)
- **[README_KissOTA_XX.md](docs/README_KissOTA_ES.md)** - Es de un gran valor porqué te detalla los pasos del sistema de actualización OTA (7 idiomas: EN, ES, FR, IT, DE, PT, CN)

**Elige tu idioma:** Todos los mensajes del construcctor que envía aa Telegram se muestran 7 idiomas mediante selección del idioma durante la compilación (lang.h).


## Ventajas de Seguridad OTA

KissTelegram OTA es **mucho más seguro que la arquitectura de Espressif y ahorra espacio en tu ESP32S3**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Autenticación | PIN + PUK | Ninguna |
| Confirmación de Suma de Verificación | CRC32 automático | Manual |
| Backup y Reversión | Automática | Manual |
| Ventana de Validación | 60s con `/otaok` | Ninguna |
| Detección de Bucle de Arranque | Sí | No |
| Integración en Telegram | Nativa | Requiere código personalizado |
| Optimización de Flash | 13MB SPIFFS | 5MB SPIFFS |

---

## Referencia de API

### Inicialización

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Mensajería

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configuración

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Monitoreo

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### Almacenamiento

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Ejemplos

Consulta el .ino incluido en la biblioteca para explorar algunos escenarios y características y mi forma de código en KissTelegram. Mejor aún, descomenta tu idioma en [lang.h] para recibir mensajes de los constructores principales (.cpp) en tu idioma local, si todos los idiomas están comentados los mensajes son en castellano, el idioma predeterminado:

Las convenciones de código están en inglés, pero los pensamientos y comentarios son en castellano, mi idioma nativo, usa tu traductor en línea, el código es fácil, dentro del código está mi visión y el concepto de KissTelegram...

````cpp

// =========================================================================
// LANGUAGE SELECTION - Uncomment ONE language
// =========================================================================
// #define LANG_CN  // 涓枃
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Fran?ais
// #define LANG_IT  // Italiano
// #define LANG_PT  // Portugués
````

---
## Configuración Básica de Configuración
- Renombra system_setup_template.h a system_setup.h en tu carpeta de KissTelegram para comenzar la compilación.
- Reemplaza las siguientes líneas por tus credenciales.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licencia

Este proyecto está bajo licencia MIT - ve el archivo [LICENSE](LICENSE) para más detalles.

---

## Arquitectura, Visión, Concepto, Soluciones y Dise?o (y el responsable de cualquier mal funcionamiento, soy yo...)

**Vicente Soriano**
Correo electrónico: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Colaboradores**
- Muchos asistentes de IA en Traducciones, Código, Solución de problemas y muchas horas intentando que no reinventen la rueda.....

---


## Contribuyendo

Las contribuciones son bienvenidas! Por favor, siéntete libre de enviar un Pull Request o enviarme un e-mail, pero prefiero un PR para que otros encuentren su pregunta.

---

## Soporte

Si encuentras útil esta biblioteca, por favor considera:
- Darle una estrella a este repositorio
- Reportar bugs a través de GitHub Issues
- Compartir tus proyectos usando KissTelegram
- Comentar a tus conocidos las funciones y soluciones de esta librería
- Proponer casos de uso y experiencias con KissTelegram

---

