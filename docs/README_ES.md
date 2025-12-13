# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Espa?ol** | [Documentación](GETTING_STARTED_ES.md) | [Benchmarks](BENCHMARK.md)

---

> 馃毃 **驴PRIMERA VEZ USANDO ESP32-S3 CON KISSTELEGRAM?**
> **LEE ESTO PRIMERO:** [**GETTING_STARTED_ES.md**](GETTING_STARTED_ES.md) 鈿狅笍
> ESP32-S3 requiere un **proceso de carga en dos pasos** debido a particiones personalizadas. 隆Ignorar esta gu铆a causar谩 errores de arranque!

---

## Una Biblioteca Robusta de Grado Empresarial para Bots de Telegram en ESP32-S3

KissTelegram es la **煤nica biblioteca de Telegram para ESP32** construida desde cero para aplicaciones cr铆ticas. A diferencia de otras bibliotecas que dependen de la clase Arduino `String` (causando fragmentaci贸n de memoria y fugas), KissTelegram utiliza arrays puros de `char[]` para una estabilidad inquebrantable.

### 驴Por qu茅 KissTelegram?

- Cansado de proyectos perdidos por bibliotecas d茅biles, fugas de memoria, soluciones de 煤ltimo momento, falta de soporte, palabras de moda, reinicios....

- Mi visi贸n, ahora los hechos:

- **Cero P茅rdida de Mensajes**: Cola persistente en LittleFS que sobrevive a bloqueos, reinicios y fallos de WiFi
- **Sin Fugas de Memoria**: Implementaci贸n pura de `char[]`, sin fragmentaci贸n de String
- **Seguridad SSL/TLS**: Conexiones seguras a la API de Telegram con validaci贸n de certificados
- **Gesti贸n Inteligente de Energ铆a**: 6 modos de potencia (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Prioridades de Mensajes**: CRITICAL, HIGH, NORMAL, LOW con gesti贸n inteligente de colas
- **Modo Turbo**: Procesamiento por lotes para colas grandes de mensajes (0,9 msg/s)
- **i18n Multiling眉e**: Selecci贸n de idioma en tiempo de compilaci贸n (7 idiomas, sin sobrecarga en tiempo de ejecuci贸n)
- **OTA Empresarial**: Actualizaciones de firmware de doble arranque con reversi贸n autom谩tica y puerta de seguridad
- **100% Utilizaci贸n de Flash**: Esquema de partici贸n personalizado maximiza el flash de 16MB del ESP32-S3
- **M谩s Seguro que OTA de Espressif**: Autenticaci贸n PIN/PUK, verificaci贸n de suma de verificaci贸n, ventana de validaci贸n de 60s
- **Independiente de bibliotecas externas**: Todo hecho desde cero, analizador JSON propio.

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

**13MB de almacenamiento SPIFFS** - 隆eso son 8MB m谩s que los esquemas predeterminados de Espressif!

Para usar este esquema de partici贸n:
1. Copia `partitions.csv` a tu directorio de proyecto
2. En Arduino IDE: Tools 鈫?Partition Scheme 鈫?Custom
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
1. Env铆a `/ota` a tu bot
2. Introduce el PIN con `/otapin YOUR_PIN`
3. Carga el archivo de firmware `.bin`
4. El bot verifica la suma de verificaci贸n autom谩ticamente
5. Confirma con `/otaconfirm`
6. Despu茅s de reiniciar, valida con `/otaok` dentro de 60 segundos
7. 隆Reversi贸n autom谩tica si la validaci贸n falla!

- Lee Readme_KissOTA.md en tu idioma preferido para saber m谩s sobre la soluci贸n.

---

## Caracter铆sticas Clave Explicadas

### 1. Cola de Mensajes Persistente

Los mensajes se almacenan en LittleFS con eliminaci贸n autom谩tica por lotes:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sobrevive a bloqueos, desconexiones de WiFi, reinicios
- Reintentos autom谩ticos de env铆os fallidos
- Eliminaci贸n inteligente por lotes (cada 10 mensajes + cuando la cola est谩 vac铆a)
- Garant铆a de cero p茅rdida de mensajes

### 2. Gesti贸n de Energ铆a

6 modos de energ铆a inteligentes se adaptan a las necesidades de tu aplicaci贸n:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase de inicio inicial (10s)
- **POWER_LOW**: Actividad m铆nima, sondeo lento
- **POWER_IDLE**: Sin actividad reciente, comprobaciones reducidas
- **POWER_ACTIVE**: Operaci贸n normal
- **POWER_TURBO**: Procesamiento por lotes de alta velocidad (intervalos de 50ms)
- **POWER_MAINTENANCE**: Anulaci贸n manual para actualizaciones
- **Decay Timing para cambios suave**

### 3. Prioridades de Mensajes

Cuatro niveles de prioridad aseguran que los mensajes cr铆ticos se env铆en primero:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La cola procesa: **CRITICAL 鈫?HIGH 鈫?NORMAL 鈫?LOW**
Procesos internos: **OTAMODE 鈫?MAINTENANCEMODE**

### 4. Seguridad SSL/TLS

Conexiones seguras con validación de certificados (hasta 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback automático entre seguro e inseguro
- Comprobaciones peri贸dicas de ping para mantener la conexi贸n
- C贸digo de conexi贸n reutilizable ahorra encabezado de conexi贸n para m谩ximo rendimiento

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

Monitoreo y depuraci贸n completos:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Muestra:
- Memoria libre (heap/PSRAM)
- Estad铆sticas de cola de mensajes
- Calidad de conexi贸n
- Historial de modo de energ铆a
- Uso de almacenamiento
- Tiempo de actividad

---

## 8. Gesti贸n de WiFi
- WiFi Manager integrado activa otras tareas solo hasta que WiFi sea estable
- Previene condiciones de carrera
- Reclama mensajes en curso para ir al almacenamiento FS hasta que se restablezca la conexi贸n, puede mantener hasta 3500 msg (predeterminado pero f谩cilmente expandible)
- Monitoreo de calidad de conexi贸n (EXCELLENT, GOOD, FAIR, POOR, DEAD) y nivel de salida RSSI
- Solo tienes que ocuparte de tu sketch, utiliza KissTelegram para el resto

---

## 9. Caracter铆stica Clave: Comando `/estado`

**La herramienta de depuraci贸n m谩s poderosa que jam谩s usar谩s:**

Env铆a `/estado` a tu bot y obt茅n un **informe completo de salud** en segundos:

```
馃摝 KissTelegram v1.x.x
馃幆 SYSTEM RELIABILITY
鉁?System: RELIABLE
鉁?Messages sent: 5,234
馃捑 Messages pending: 0
馃摗 WiFi Signal: -59 dBm (Good)
馃攲 WiFi reconnections: 2
鈴憋笍 Uptime: 86,400 seconds (24h)
馃捑 Free memory: 223 KB
馃搳 Queue statistics: All systems operational
```

**Por qu茅 `/estado` es esencial:**
- 鉁?Verificaci贸n instant谩nea de salud del sistema
- 鉁?Monitoreo de calidad de WiFi (diagnostica problemas de conectividad)
- 鉁?Detecci贸n de fugas de memoria (observa el heap libre)
- 鉁?Estado de la cola de mensajes (ve mensajes pendientes/fallidos)
- 鉁?Seguimiento de tiempo de actividad (monitoreo de estabilidad)
- 鉁?Disponible en 7 idiomas
- 鉁?Tu primera herramienta al depurar problemas

**Consejo profesional:** 隆Haz que `/estado` sea tu primer mensaje despu茅s de cada actualizaci贸n de firmware para verificar que todo funcione!

---

## 10. NTP
- C贸digo propio para sincronizar/resincronizar para SSL y Scheduler (Enterprise Edition)
---

## 11. Documentaci贸n (7 Idiomas)

- **[GETTING_STARTED_ES.md](GETTING_STARTED_ES.md)** - **隆COMIENZA AQU脥!** Gu铆a completa desde desempacar ESP32-S3 hasta el primer mensaje de Telegram
- **[README_ES.md](README_ES.md)** (este archivo) - Descripci贸n general de caracter铆sticas, inicio r谩pido, referencia de API
- **[BENCHMARK.md](BENCHMARK.md)** - Comparaci贸n t茅cnica con 6 otras bibliotecas de Telegram (solo en ingl茅s)
- **[README_KissOTA_XX.md](README_KissOTA_ES.md)** - Documentaci贸n del sistema de actualizaci贸n OTA (7 idiomas: EN, ES, FR, IT, DE, PT, CN)

**Elige tu idioma:** Todos los mensajes del sistema de KissTelegram admiten 7 idiomas mediante selecci贸n en tiempo de compilaci贸n.


## Ventajas de Seguridad OTA

KissTelegram OTA es **m谩s seguro que la implementaci贸n de Espressif**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Autenticaci贸n | PIN + PUK | Ninguna |
| Verificaci贸n de Suma de Verificaci贸n | CRC32 autom谩tico | Manual |
| Backup y Reversi贸n | Autom谩tico | Manual |
| Ventana de Validaci贸n | 60s con `/otaok` | Ninguna |
| Detecci贸n de Bucle de Arranque | S铆 | No |
| Integraci贸n de Telegram | Nativa | Requiere c贸digo personalizado |
| Optimizaci贸n de Flash | 13MB SPIFFS | 5MB SPIFFS |

---

## Referencia de API

### Inicializaci贸n

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Mensajer铆a

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configuraci贸n

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

Consulta el .ino incluido en la biblioteca para explorar algunos escenarios y caracter铆sticas y mi estilo de c贸digo de KissTelegram. Mejor a煤n, descomenta tu idioma en [lang.h] para recibir mensajes de los constructores principales (.cpp) en tu idioma local, si todos los idiomas est谩n comentados obtienes mensajes en espa帽ol, el idioma predeterminado:

Las convenciones de c贸digo est谩n en ingl茅s, pero los pensamientos y comentarios en mi idioma nativo, usa tu traductor en l铆nea, el c贸digo es f谩cil, detr谩s del c贸digo hay mucho m谩s complicado ...

````cpp

// =========================================================================
// LANGUAGE SELECTION - Uncomment ONE language
// =========================================================================
// #define LANG_CN  // 涓枃
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Fran莽ais
// #define LANG_IT  // Italiano
// #define LANG_PT  // Portugu锚s
````

---
## Configuraci贸n B谩sica de Configuraci贸n
- Renombra system_setup_template.h a system_setup.h en tu carpeta de KissTelegram para comenzar la compilaci贸n.
- Reemplaza las siguientes l铆neas por tus configuraciones.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licencia

Este proyecto est谩 bajo licencia MIT - ve el archivo [LICENSE](LICENSE) para m谩s detalles.

---

## Arquitectura, Visi贸n, Concepto, Soluciones y Dise帽o (y el responsable de cualquier mal funcionamiento)

**Vicente Soriano**
Correo electr贸nico: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Colaboradores**
- Muchos asistentes de IA en Traducciones, C贸digo, Soluci贸n de Problemas y bromas.

---


## Contribuyendo

隆Las contribuciones son bienvenidas! Por favor, si茅ntete libre de enviar un Pull Request.

---

## Soporte

Si encuentras 煤til esta biblioteca, por favor considera:
- Darle una estrella a este repositorio
- Reportar bugs a trav茅s de GitHub Issues
- Compartir tus proyectos usando KissTelegram

---

