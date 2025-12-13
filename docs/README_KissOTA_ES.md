# KissOTA - Sistema de Actualización OTA para ESP32

## Índice
- [Descripción General](#descripción-general)
- [Características Principales](#características-principales)
- [Comparativa con Otros Sistemas OTA](#comparativa-con-otros-sistemas-ota)
- [Arquitectura del Sistema](#arquitectura-del-sistema)
- [Ventajas sobre el OTA Estándar de Espressif](#ventajas-sobre-el-ota-estándar-de-espressif)
- [Requisitos del Hardware](#requisitos-del-hardware)
- [Configuración de Particiones](#configuración-de-particiones)
- [Flujo del Proceso OTA](#flujo-del-proceso-ota)
- [Comandos Disponibles](#comandos-disponibles)
- [Características de Seguridad](#características-de-seguridad)
- [Gestión de Errores y Recuperación](#gestión-de-errores-y-recuperación)
- [Uso Básico](#uso-básico)
- [Preguntas Frecuentes (FAQ)](#preguntas-frecuentes-faq)

---

## Descripción General

KissOTA es un sistema avanzado de actualización Over-The-Air (OTA) diseñado específicamente para dispositivos ESP32-S3 con PSRAM. A diferencia del sistema OTA estándar de Espressif, KissOTA proporciona múltiples capas de seguridad, validación y recuperación automática, todo integrado con Telegram para actualizaciones remotas seguras.

**Autor:** Vicente Soriano (victek@gmail.com)
**Versión actual:** 0.9.0
**Licencia:** Incluida en KissTelegram Suite

---

## Características Principales

### 🔒 Seguridad Robusta
- **Autenticación PIN/PUK**: Control de acceso mediante códigos PIN y PUK
- **Bloqueo automático**: Tras 3 intentos fallidos de PIN
- **Verificación CRC32**: Validación de integridad del firmware antes del flash
- **Validación post-flash**: Requiere confirmación manual del usuario

### 🛡️ Protección contra Fallos
- **Backup automático**: Copia del firmware actual antes de actualizar
- **Rollback automático**: Restauración si el nuevo firmware falla
- **Detección de boot loops**: Si falla 3 veces en 5 minutos, rollback automático
- **Partición factory**: Red de seguridad final si todo lo demás falla
- **Timeout global**: 7 minutos para completar todo el proceso

### 💾 Uso Eficiente de Recursos
- **Buffer PSRAM**: Usa 7-8MB de PSRAM para descargar sin tocar flash
- **Ahorro de espacio**: Solo necesita particiones factory + app (sin OTA_0/OTA_1)
- **Limpieza automática**: Elimina archivos temporales y backups antiguos

### 📱 Integración con Telegram
- **Actualización remota**: Envía el .bin directamente por Telegram
- **Feedback en tiempo real**: Mensajes de progreso y estado
- **Multi-idioma**: Soporte para 7 idiomas (ES, EN, FR, IT, DE, PT, CN)

---

## Comparativa con Otros Sistemas OTA

### KissOTA vs Otras Soluciones Populares

| Característica | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **Transporte** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Acceso remoto** | ✅ Global sin config | ❌ Solo LAN* | ❌ Solo LAN* | ❌ Solo LAN* | ❌ Solo LAN* |
| **Requiere IP conocida** | ❌ No | ✅ Sí | ✅ Sí | ✅ Sí | ✅ Sí |
| **Port forwarding** | ❌ No necesario | ⚠️ Para acceso remoto | ⚠️ Para acceso remoto | ⚠️ Para acceso remoto | ⚠️ Para acceso remoto |
| **Servidor web** | ❌ No | ✅ AsyncWebServer | ✅ WebServer | ✅ Configurable | ✅ WebServer |
| **Autenticación** | 🔒 PIN/PUK (robusto) | ⚠️ Usuario/pass básico | ⚠️ Password opcional | ⚠️ Básica | ⚠️ Usuario/pass básico |
| **Backup firmware** | ✅ En LittleFS | ❌ No | ❌ No | ❌ No | ❌ No |
| **Rollback automático** | ✅✅✅ 3 niveles | ⚠️ Limitado (2 part.) | ❌ No | ⚠️ Limitado (2 part.) | ⚠️ Limitado (2 part.) |
| **Validación manual** | ✅ 60s con /otaok | ❌ No | ❌ No | ⚠️ Opcional | ❌ No |
| **Boot loop detection** | ✅ Automático | ❌ No | ❌ No | ❌ No | ❌ No |
| **Buffer de descarga** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Progreso en tiempo real** | ✅ Telegram messages | ✅ Web UI | ⚠️ Serial only | ⚠️ Configurable | ✅ Web UI |
| **Interfaz de usuario** | 📱 Telegram chat | 🖥️ Navegador web | 🖥️ Navegador web | ⚡ Programática | 🖥️ Navegador web |
| **Dependencias** | KissTelegram | ESPAsyncWebServer | ESP mDNS | Ninguna (nativo) | WebServer |
| **Flash requerido** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Seguridad en internet** | ✅ Alta (Telegram API) | ⚠️ Vulnerable si expuesto | ⚠️ Vulnerable si expuesto | ⚠️ Vulnerable si expuesto | ⚠️ Vulnerable si expuesto |
| **Fácil de usar** | ✅✅ Solo enviar .bin | ✅ UI web intuitiva | ⚠️ Requiere configuración | ⚠️ Complejidad alta | ✅ UI web intuitiva |
| **Multi-idioma** | ✅ 7 idiomas | ❌ Solo inglés | ❌ Solo inglés | ❌ Solo inglés | ❌ Solo inglés |
| **Recuperación factory** | ✅ Sí | ❌ No | ❌ No | ⚠️ Manual | ❌ No |

\* *Con port forwarding/VPN es posible acceso remoto, pero requiere configuración avanzada de red*

### Ventajas Únicas de KissOTA

#### 🌍 **Acceso Verdaderamente Global**
Otros sistemas OTA requieren:
- Conocer la IP del dispositivo
- Estar en la misma red LAN, o
- Configurar port forwarding (arriesgado), o
- Configurar VPN (complejo)

**KissOTA:** Solo necesitas Telegram. Actualiza desde cualquier parte del mundo sin configuración de red. Como está integrado en KissTelegram usa conexión SSL.

#### 🔒 **Seguridad sin Compromisos**
Exponer un servidor web HTTP a internet es peligroso:
- Vulnerable a ataques de fuerza bruta
- Posible vector de exploits web
- Requiere HTTPS para seguridad (certificados, etc.)

**KissOTA:** Usa la infraestructura segura de Telegram. Tu ESP32 nunca expone puertos al exterior.

#### 🛡️ **Recuperación Multinivel**
Otros sistemas con rollback (ESP-IDF, AsyncElegantOTA) solo tienen 2 particiones:
- Si ambas particiones fallan → dispositivo "bricked"
- Sin backup del firmware funcional

**KissOTA:** 3 niveles de seguridad:
1. **Nivel 1:** Rollback desde backup en LittleFS
2. **Nivel 2:** Boot desde partición factory original
3. **Nivel 3:** Detección de boot loops automática

#### 💾 **Ahorro de Espacio Flash**
Sistemas tradicionales (dual-bank OTA):
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 7 MB mínimo requerido
OTA_1:    3.5 MB  ┘
Total: 10.5 MB
```

**KissOTA (single-bank + backup):**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 7 MB total
Backup:   ~1.1 MB │  (en LittleFS, comprimible)
Total: ~8.1 MB   ┘
```
**Ganancia:** ~2.4 MB de flash libre para tu aplicación o datos.

#### 🚀 **PSRAM como Buffer**
Otros sistemas descargan directamente a flash:
- **Desgaste del flash:** Flash tiene ciclos limitados de escritura (~100K)
- **Riesgo:** Si falla descarga, flash ya fue escrito parcialmente

**KissOTA:**
- Descarga completa a PSRAM primero
- Verifica CRC32 en PSRAM
- Solo escribe a flash si todo está OK
- **PSRAM no tiene desgaste:** Infinitos ciclos de escritura

---

## Arquitectura del Sistema

### Estructura de Particiones

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Firmware original de fábrica
│  - Red de seguridad final          │
│  - Solo lectura en producción      │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Firmware en ejecución
│  - Firmware activo actual           │
│  - Se actualiza durante OTA         │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (restante ~8-9 MB)        │
│  - /ota_backup.bin (backup)         │ ← Backup del firmware anterior
│  - Archivos de configuración        │
│  - Datos persistentes               │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot flags (validación)          │ ← Estado de validación OTA
│  - Credenciales PIN/PUK             │
│  - Contador de arranques            │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Buffer de descarga temporal      │ ← Firmware descargado antes del flash
│  - No persistente (se borra)        │
└─────────────────────────────────────┘
```

### Flujo de Datos Durante OTA

```
Telegram API
    │
    ▼
[Descarga a PSRAM] ← 7-8 MB buffer temporal
    │
    ▼
[Verificación CRC32]
    │
    ▼
[Backup firmware actual → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash nuevo firmware → App Partition]
    │
    ▼
[Reinicio del ESP32]
    │
    ▼
[Validación 60 segundos] → /otaok o rollback automático
    │
    ▼
[Borra firmware anterior] -> Recupera el espacio de firmware anterior

```
---

## Ventajas sobre el OTA Estándar de Espressif

| Característica | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **Espacio Flash requerido** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (solo app) |
| **Backup del firmware** | ❌ No | ✅ Sí, en LittleFS |
| **Rollback automático** | ⚠️ Limitado | ✅ Completo + factory |
| **Validación manual** | ❌ No | ✅ Sí, 60 segundos |
| **Autenticación** | ❌ No | ✅ PIN/PUK |
| **Detección boot loops** | ❌ No | ✅ Sí, automática |
| **Integración Telegram** | ❌ No | ✅ Nativa |
| **Buffer de descarga** | Flash | PSRAM (no desgasta flash) |
| **Recuperación de emergencia** | ⚠️ Solo 2 particiones | ✅ 3 niveles (app/backup/factory) |

---

## Requisitos del Hardware

### Mínimo Requerido
- **MCU**: ESP32-S3 (otras variantes ESP32 pueden funcionar con adaptaciones)
- **PSRAM**: Mínimo depende del tamñao del firmware a reemplazar 2-8MB PSRAM (para buffer de descarga)
- **Flash**: Mínimo depende del tamaño del firmware a reemplazar 8-16-32MB
- **Conectividad**: WiFi funcional

### Configuración Recomendada
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (ideal)
- **ESP32-S3-DevKitC-1**: Con módulo N16R8 o superior

---

## Configuración de Particiones

Archivo `partitions.csv` recomendado (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Configuración en Arduino IDE:**
- Tools → Partition Scheme → Custom
- Apuntar al archivo `partitions.csv`

---

## Flujo del Proceso OTA

### Diagrama de Estados

```
┌─────────────┐
│  OTA_IDLE   │ ← Estado inicial
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Solicita PIN (3 intentos)
└──────┬──────┘
       │ PIN correcto
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, listo para recibir .bin
└──────┬───────┘
       │ Usuario envía .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Descarga a PSRAM (progreso en tiempo real)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Calcula CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Espera /otaconfirm del usuario
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Guarda firmware actual en LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Escribe nuevo firmware desde PSRAM
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← Reinicia ESP32
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60 segundos para /otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← Firmware validado ✅
└──────────────┘
       │ Timeout o /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Restaura backup automáticamente
└──────────────┘
```

### Timeouts Importantes

- **Autenticación PIN**: Ilimitado (hasta 3 intentos), si falla, PIN bloqueado y precisa PUK para restaurarlo
- **Recepción del .bin**: Espera hasta 7 minutos, si se supera cancela /ota
- **Confirmación**: Espera hasta 7 minutos, si se supera cancela /ota (espera /otaconfirm)
- **Proceso completo**: 7 minutos máximo desde /otaconfirm
- **Validación post-flash**: 60 segundos para /otaok

---

## Comandos Disponibles

### `/ota`
Inicia el proceso OTA.

**Uso:**
```
/ota
```

**Respuesta:**
- Si PIN no bloqueado: Solicita PIN
- Si PIN bloqueado: Solicita PUK

---

### `/otapin <código>`
Envía el código PIN (4-8 dígitos).

**Uso:**
```
/otapin 0000 (por defecto)
```

**Respuesta:**
- ✅ PIN correcto: Estado AUTHENTICATED, listo para recibir .bin
- ❌ PIN incorrecto: Reduce intentos restantes
- 🔒 Tras 3 fallos: Bloquea y solicita PUK

---

### `/otapuk <código>`
Desbloquea el sistema con el código PUK.

**Uso:**
```
/otapuk 12345678
```

**Respuesta:**
- ✅ PUK correcto: Desbloquea PIN y solicita nuevo PIN
- ❌ PUK incorrecto: Permanece bloqueado

---

### `/otaconfirm`
Confirma que deseas flashear el firmware descargado.

**Uso:**
```
/otaconfirm
```

**Precondiciones:**
- Firmware descargado y verificado
- Checksum CRC32 OK

**Acción:**
- Crea backup del firmware actual
- Flashea nuevo firmware
- Reinicia el ESP32

---

### `/otaok`
Valida que el nuevo firmware funciona correctamente.

**Uso:**
```
/otaok
```

**Precondiciones:**
- Debe enviarse dentro de 60 segundos tras el reinicio
- Solo disponible tras un flash OTA

**Acción:**
- Marca el firmware como válido
- Elimina el backup anterior
- Sistema vuelve a operación normal

⚠️ **IMPORTANTE**: Si no se envía `/otaok` en 60 segundos, se ejecutará rollback automático.

---

### `/otacancel`
Cancela el proceso OTA o fuerza rollback.

**Uso:**
```
/otacancel
```

**Comportamiento:**
- Durante descarga/validación: Cancela y limpia archivos temporales
- Durante validación post-flash: Ejecuta rollback inmediato
- Si no hay OTA activo: Informa que no hay proceso activo

---

## Características de Seguridad

### 1. Autenticación PIN/PUK

#### PIN y PUK (Se pueden cambiar remotamente con /changepin <viejo> <nuevo> o /changepuk <viejo> <nuevo>)

#### PIN (Personal Identification Number)
- **Longitud**: 4-8 dígitos
- **Intentos**: 3 intentos antes de bloqueo
- **Configuración**: Definido en `system_setup.h` Credenciales Fallback
- **Persistencia**: Se guarda seguro en NVS

#### PUK (PIN Unlock Key)
- **Longitud**: 8 dígitos
- **Función**: Desbloquear tras 3 fallos de PIN
- **Seguridad**: Solo el administrador debería conocerlo

**Ejemplo de flujo de bloqueo:**
```
Intento 1: PIN incorrecto → "❌ PIN incorrecto. 2 intentos restantes"
Intento 2: PIN incorrecto → "❌ PIN incorrecto. 1 intento restante"
Intento 3: PIN incorrecto → "🔒 PIN bloqueado. Use /otapuk [código]"
```

### 2. Verificación de Integridad

#### CRC32 Checksum
- **Algoritmo**: CRC32 IEEE 802.3
- **Cálculo**: Sobre todo el archivo .bin descargado
- **Validación**: Antes de permitir /otaconfirm
- **Rechazo**: Si CRC32 no coincide, descarga se elimina

**Ejemplo de salida:**
```
🔍 Verificando checksum...
🔍 CRC32: 0xF8CAACF6 (1.07 MB verificados)
✅ Checksum OK
```

### 3. Validación Post-Flash

#### Ventana de Validación (60 segundos)
Tras flashear el nuevo firmware:
1. ESP32 reinicia
2. Boot flags marcan `otaInProgress = true`
3. Usuario tiene 60 segundos para enviar `/otaok`
4. Si no responde → rollback automático

**Diagrama temporal:**
```
FLASH → REINICIO → [60s para /otaok] → ROLLBACK si timeout
                         ↓
                      /otaok → ✅ Firmware validado
```

### 4. Protección contra Modificación No Autorizada

- **Modo mantenimiento**: Durante OTA, otros comandos están limitados
- **Chat ID único**: Solo el chat_id configurado puede ejecutar OTA
- **Autenticación previa**: PIN/PUK obligatorios antes de cualquier acción

---

## Gestión de Errores y Recuperación

### Niveles de Recuperación

#### Nivel 1: Reintento Automático
**Escenarios:**
- Fallo en descarga desde Telegram (max 3 reintentos)
- Timeout de red
- Descarga interrumpida

**Acción:**
- Limpia PSRAM
- Reintenta descarga automáticamente
- Notifica al usuario del intento

---

#### Nivel 2: Rollback desde Backup
**Escenarios:**
- Usuario no envía `/otaok` en 60 segundos
- Usuario envía `/otacancel` durante validación
- Boot loop detectado (3+ arranques en 5 minutos)

**Proceso de Rollback:**
1. Detecta `bootFlags.otaInProgress == true`
2. Lee `bootFlags.backupPath` → `/ota_backup.bin`
3. Restaura backup desde LittleFS → App Partition
4. Reinicia ESP32
5. Limpia boot flags
6. Notifica al usuario por Telegram

**Código de ejemplo:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // No hay backup
  }

  // Lee /ota_backup.bin desde LittleFS
  // Escribe en App Partition
  // Reinicia
}
```

---

#### Nivel 3: Fallback a Factory
**Escenarios:**
- Rollback desde backup falla
- Archivo `/ota_backup.bin` corrupto
- Error crítico en flash

**Proceso:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Reinicia ESP32
3. Arranca desde firmware original de fábrica
4. Notifica error crítico al usuario

⚠️ **IMPORTANTE**: Este es el último recurso. El firmware factory debe ser estable y nunca modificarse en producción.

---

### Detección de Boot Loops

**Algoritmo:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minutos
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ arranques en 5 minutos");
      return true; // Ejecutar rollback
    }
  }
  return false;
}
```

**Protección:**
- Incrementa `bootFlags.bootCount` en cada arranque
- Si > 3 arranques en < 5 minutos → Rollback automático
- Al validar con `/otaok`, resetea contador

---

### Estados de Error

| Error | Código | Acción Automática |
|-------|--------|-------------------|
| **Descarga fallida** | `DOWNLOAD_FAILED` | Reintentar hasta 3 veces |
| **Checksum incorrecto** | `CHECKSUM_MISMATCH` | Eliminar descarga, cancelar OTA |
| **Fallo en backup** | `BACKUP_FAILED` | Cancelar OTA, no arriesgar |
| **Fallo en flash** | `FLASH_FAILED` | Cancelar OTA, mantener firmware actual |
| **Timeout validación** | `VALIDATION_TIMEOUT` | Rollback automático |
| **Boot loop** | `BOOT_LOOP_DETECTED` | Rollback automático |
| **Rollback fallido** | `ROLLBACK_FAILED` | Fallback a factory |
| **Sin backup** | `NO_BACKUP` | Mantener firmware actual, avisar |

---

## Uso Básico

### Actualización Completa Paso a Paso

#### 1. Preparar el Firmware
```bash
# Compilar el proyecto en Arduino IDE
# El .bin se genera en: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. Iniciar OTA
Desde Telegram:
```
/ota
```

Respuesta del bot:
```
🔐 AUTENTICACIÓN OTA

Introduzca el código PIN de 4-8 dígitos:
/otapin [código]

Intentos restantes: 3
```

#### 3. Autenticar con PIN
```
/otapin 0000
```

Respuesta:
```
✅ PIN correcto

Sistema listo para OTA.
Envíe el archivo .bin del nuevo firmware.
```

#### 4. Enviar el Firmware
- Adjunta el archivo `.bin` como documento (no como foto)
- Telegram lo sube y el bot lo descarga automáticamente

Respuesta durante descarga:
```
📥 Descargando firmware a PSRAM...
⏳ Progreso: 45%
```

#### 5. Verificación Automática
```
✅ Descarga completa: 1.07 MB en PSRAM
🔍 Verificando checksum...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VERIFICADO

Archivo: suite_kiss.ino.bin
Tamaño: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ CONFIRMACIÓN REQUERIDA
Para flashear el firmware:
/otaconfirm

Para cancelar:
/otacancel
```

#### 6. Confirmar Flash
```
/otaconfirm
```

Respuesta:
```
💾 Iniciando backup...
✅ Backup completo: 1123456 bytes

⚡ Flasheando firmware...
✅ FLASH COMPLETADO

Firmware escrito correctamente.
El dispositivo se reiniciará ahora.

Tras reiniciar tendrá 60 segundos para validar con /otaok
Si no valida se ejecutará rollback automático.
```

#### 7. Validación Post-Reinicio
Tras el reinicio (en 60 segundos):
```
/otaok
```

Respuesta:
```
✅ FIRMWARE VALIDADO

El nuevo firmware ha sido confirmado.
Sistema vuelve a operación normal.

Versión: 0.9.0
```

---

### Ejemplo de Rollback Manual

Si el firmware nuevo no funciona bien:

```
/otacancel
```

Respuesta:
```
⚠️ EJECUTANDO ROLLBACK

Restaurando firmware anterior desde backup...
✅ Firmware anterior restaurado
🔄 Reiniciando...

[Tras reinicio]
✅ ROLLBACK COMPLETADO

Sistema restaurado al firmware anterior.
```

---

## Configuración Avanzada

### Personalizar Timeouts

En `KissOTA.h`:

```cpp
// Timeout de validación (por defecto 60 segundos)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Timeout global del proceso OTA (por defecto 7 minutos)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### Activar/Desactivar WDT Durante OTA

En `system_setup.h`:

```cpp
// Desactivar WDT durante operaciones críticas
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Pausa watchdog
  // ... operación OTA ...
  KISS_INIT_WDT();   // Reactiva watchdog
#endif
```

### Cambiar Tamaño del Buffer PSRAM

En `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB por defecto
  // Ajustar según PSRAM disponible en tu ESP32
}
```

---
### Cambiar PIN/PUK por defecto

En `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Solución de Problemas

### Error: "❌ No hay PSRAM disponible"
**Causa:** ESP32 sin PSRAM o PSRAM no habilitada

**Solución:**
1. Verificar que el ESP32-S3 tenga PSRAM física
2. En Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Recompilar el proyecto

---

### Error: "❌ Error creando backup"
**Causa:** LittleFS sin espacio o no montado

**Solución:**
1. Verificar partición `spiffs` en `partitions.csv`
2. Formatear LittleFS si es necesario
3. Aumentar tamaño de partición spiffs

---

### Error: "🔥 BOOT LOOP: 3+ arranques en 5 minutos"
**Causa:** Firmware nuevo falla consistentemente

**Solución:**
- Automática: Rollback se ejecuta solo
- Manual: Esperar al rollback automático
- Preventiva: Probar firmware en otro ESP32 antes de OTA

---

### Firmware no se valida tras /otaok
**Causa:** Timeout de 60 segundos expirado

**Solución:**
- Enviar `/otaok` dejando más tiempo tras reinicio para permitir conexión estable
- Verificar conectividad WiFi tras reinicio
- Aumentar `BOOT_VALIDATION_TIMEOUT` si es necesario

---

## Código de Ejemplo de Integración

### Inicialización en `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Inicializar credenciales
  credentials = new KissCredentials();
  credentials->begin();

  // Inicializar bot Telegram
  bot = new KissTelegram(BOT_TOKEN);

  // Inicializar OTA
  ota = new KissOTA(bot, credentials);

  // Verificar si venimos de un OTA interrumpido
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Procesar comandos Telegram
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // Loop de OTA (gestiona timeouts y estados)
  ota->loop();
}
```

---

## Información Técnica Adicional

### Formato de Boot Flags (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (validación)
  uint32_t bootCount;          // Contador de arranques
  uint32_t lastBootTime;       // Timestamp último boot
  bool otaInProgress;          // true si esperando validación
  bool firmwareValid;          // true si firmware validado
  char backupPath[64];         // Ruta del backup (/ota_backup.bin)
};
```

### Tamaños Típicos

| Elemento | Tamaño Típico |
|----------|---------------|
| Firmware compilado | 1.0 - 1.5 MB |
| Backup en LittleFS | ~1.1 MB |
| Buffer PSRAM | 7-8 MB |
| Partición Factory | 3.5 MB |
| Partición App | 3.5 MB |

---

## Contribuciones y Soporte

**Autor:** Vicente Soriano
**Email:** victek@gmail.com
**Proyecto:** KissTelegram Suite

Para reportar bugs o solicitar características, contactar al autor.

---

## Preguntas Frecuentes (FAQ)

### ¿Por qué no usar AsyncElegantOTA o ArduinoOTA?

**Respuesta Corta:** KissOTA no requiere configuración de red y funciona globalmente desde el primer momento.

**Respuesta Completa:**

AsyncElegantOTA y ArduinoOTA son excelentes para desarrollo local, pero tienen limitaciones en producción:

1. **Acceso Remoto Complicado:**
   - Necesitas conocer la IP del dispositivo
   - Si está detrás de router/NAT, necesitas port forwarding
   - Port forwarding expone tu ESP32 a internet (riesgo de seguridad)
   - Alternativa: VPN (complejo de configurar para usuarios finales)

2. **Seguridad Limitada:**
   - Usuario/contraseña básico (vulnerable a fuerza bruta)
   - HTTP sin encriptación (a menos que configures HTTPS con certificados)
   - Servidor web expuesto = superficie de ataque amplia

3. **Sin Rollback Real:**
   - Solo tienen 2 particiones (OTA_0 y OTA_1)
   - Si ambas fallan, dispositivo "bricked"
   - No hay backup del firmware funcional conocido

**KissOTA soluciona esto:**
- ✅ Actualización global sin configurar nada (Telegram API)
- ✅ Seguridad robusta (PIN/PUK + infraestructura Telegram)
- ✅ Rollback multinivel (backup + factory + boot loop detection)
- ✅ No expone puertos al exterior

**¿Cuándo usar cada uno?**
- **AsyncElegantOTA:** Desarrollo local, prototipado rápido, LAN privada
- **KissOTA:** Producción, dispositivos remotos globales, seguridad crítica

---

### ¿Funciona sin PSRAM?

**Respuesta:** La versión actual de KissOTA **requiere PSRAM** para el buffer de descarga y verificación de válidez del archivo.

**Razones técnicas:**
- Firmware típico: 1-1.5 MB
- Buffer PSRAM: 7-8 MB disponibles
- RAM normal ESP32-S3: Solo ~70-105 KB libres

**Alternativas si no tienes PSRAM:**
1. **Descargar a LittleFS primero:**
   - Más lento (flash es más lento que PSRAM)
   - Desgasta flash innecesariamente
   - Requiere espacio libre en LittleFS

2. **Usar AsyncElegantOTA:**
   - No requiere PSRAM
   - Descarga directamente a partición OTA

3. **Contribuir un PR:**
   - Si implementas soporte para ESP32 sin PSRAM, bienvenido el PR

**Hardware recomendado:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 €uros
- ESP32-S3-DevKitC-1 con módulo N16R8

---

### ¿Puedo usar KissOTA sin Telegram?

**Respuesta:** Técnicamente sí, pero requiere trabajo de adaptación.

La arquitectura de KissOTA tiene dos capas:

1. **Core OTA (backend):**
   - Gestión de estados
   - Backup/rollback
   - Flash desde PSRAM
   - Validación CRC32
   - Boot flags
   - **Esta parte es independiente del transporte**

2. **Frontend Telegram:**
   - Autenticación PIN/PUK
   - Descarga de archivos desde Telegram API
   - Mensajes de progreso al usuario
   - **Esta parte depende de KissTelegram**

**Para usar otro transporte (HTTP, MQTT, Serial, etc.):**

```cpp
// Necesitarías implementar tu propio frontend
class KissOTACustom : public KissOTACore {
public:
  // Implementar métodos abstractos:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**¿Vale la pena el esfuerzo?**
- Si ya tienes KissTelegram: **No, usa KissOTA como está**
- Si no quieres Telegram: **Probablemente mejor usar AsyncElegantOTA**
- Si tienes un caso de uso específico (ej: MQTT industrial): **KissTelegram tiene una versión para Empresas, pregunta**

---

### ¿Qué pasa si pierdo conexión WiFi durante la descarga?

**Respuesta:** El sistema maneja desconexiones de forma segura.

**Escenarios:**

**1. Descarga a PSRAM interrumpida:**
```
Estado: DOWNLOADING → Timeout de red
Acción automática:
  1. Detecta timeout (tras 30 segundos sin datos)
  2. Limpia buffer PSRAM
  3. Reintenta descarga (máximo 3 intentos)
  4. Si 3 fallos: Cancela OTA, vuelve a IDLE
  5. Firmware actual NO tocado (seguro)
```

**2. Desconexión durante flash:**
```
Estado: FLASHING → WiFi se cae
Resultado:
  - Flash continúa (no depende de WiFi)
  - Datos ya están en PSRAM
  - Flash completa normalmente
  - Al reiniciar, esperará /otaok (60s)
  - Si no hay WiFi para enviar /otaok: Rollback automático
```

**3. Desconexión durante validación:**
```
Estado: VALIDATING → Sin WiFi
Resultado:
  - Timer de 60 segundos corriendo
  - Si WiFi vuelve antes de 60s: Puedes enviar /otaok
  - Si pasan 60s sin /otaok: Rollback automático
  - Sistema vuelve al firmware anterior (seguro)
```

**Timeout global:** 7 minutos desde /otaconfirm hasta completar todo. Si se excede, OTA se cancela automáticamente.

---

### ¿Es seguro flashear firmware más pequeño que el actual?

**Respuesta:** Sí, es completamente seguro.

**Explicación técnica:**

El proceso de flash siempre:
1. **Borra completamente la partición destino** antes de escribir
2. **Escribe el nuevo firmware** (sea del tamaño que sea)
3. **Marca el tamaño real** en los metadatos de la partición

**Ejemplo:**
```
Firmware actual:  1.5 MB
Firmware nuevo:   0.9 MB

Proceso:
1. Backup de 1.5 MB → LittleFS
2. Borrar partición app (3.5 MB completos)
3. Escribir 0.9 MB nuevos
4. Metadatos: size = 0.9 MB
5. Espacio restante en partición: Vacío/borrado
```

**No hay problema de "basura residual"** porque:
- ESP32 solo ejecuta hasta el final del firmware marcado
- El resto de la partición está borrado
- CRC32 se calcula solo sobre el tamaño real

**Casos de uso comunes:**
- Desactivar features para reducir tamaño
- Firmware de emergencia minimalista (~500 KB)
- Compilación optimizada con flags diferentes

---

### ¿Cómo reseteo el sistema si todo falla?

**Respuesta:** Tienes 3 opciones de recuperación.

#### Opción 1: Rollback Automático (Más Común)
Si el firmware nuevo falla, simplemente **no hagas nada**:
```
1. Firmware nuevo arranca pero falla
2. ESP32 reinicia (crash/watchdog)
3. Contador bootCount++ en NVS
4. Tras 3 reinicios en 5 minutos → Rollback automático
5. Sistema restaura firmware anterior desde /ota_backup.bin
```

#### Opción 2: Rollback Manual
Si tienes acceso a Telegram:
```
/otacancel
```
Fuerza rollback inmediato desde backup.

#### Opción 3: Boot en Factory (Emergencia)
Si todo lo demás falla:

**Vía Serial (requiere acceso físico):**
```cpp
// En setup(), detecta condición de emergencia
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin a GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Vía esptool (último recurso):**
```bash
# Flashear factory partition con firmware original
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Opción 4: Flash Completo (Borrón y Cuenta Nueva)
```bash
# Borra todo el flash
esptool.py --port COM13 erase_flash

# Flashea firmware fresco + particiones
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prevención:**
- ✅ Mantén siempre un firmware factory estable
- ✅ Prueba firmware nuevos en dispositivo de desarrollo primero
- ✅ No modifiques la partición factory en producción

---

### ¿Cuánto tiempo tarda una actualización OTA completa?

**Respuesta:** Aproximadamente 1-3 minutos total para firmware de ~1 MB si la WiFi es perfecta.

**Desglose temporal típico:**

| Fase | Duración Típica | Factores |
|------|----------------|----------|
| **Autenticación** | 10-30 segundos | Tiempo de usuario escribiendo PIN |
| **Subida a Telegram** | 5-15 segundos | Velocidad de internet del usuario |
| **Descarga a PSRAM** | 10-20 segundos | Velocidad WiFi del ESP32 + Telegram API |
| **Verificación CRC32** | 2-5 segundos | Tamaño del firmware |
| **Confirmación usuario** | 5-30 segundos | Tiempo de reacción del usuario |
| **Backup firmware actual** | 10-30 segundos | Velocidad de escritura LittleFS |
| **Flash nuevo firmware** | 5-10 segundos | Velocidad de escritura flash |
| **Reinicio** | 5-10 segundos | Tiempo de boot del ESP32 |
| **Validación /otaok** | 5-60 segundos | Tiempo de reacción del usuario |
| **TOTAL** | **~2-4 minutos** | Varía según condiciones |

**Factores que afectan la velocidad:**

**Más rápido:**
- ✅ WiFi fuerte (cerca del router)
- ✅ Firmware pequeño (<1 MB)
- ✅ Usuario experimentado (responde rápido)
- ✅ LittleFS no fragmentado. KissTelegram defragmenta el FS cada 3-5 minutos

**Más lento:**
- ⚠️ WiFi débil o congestionado
- ⚠️ Firmware grande (>1.5 MB)
- ⚠️ Usuario dudando en confirmar
- ⚠️ LittleFS muy lleno

**Timeout de seguridad:** 7 minutos máximo desde /otaconfirm. Si se excede, OTA se cancela.

---

### ¿Puedo actualizar múltiples ESP32 simultáneamente?

**Respuesta:** Sí, pero cada ESP32 necesita su propio bot de Telegram (diferente BOT_TOKEN).

**Limitación de Telegram:**
- Un bot Telegram solo puede procesar 1 conversación simultánea de forma confiable
- Telegram API tiene rate limits (~30 mensajes/segundo por bot)

**Opción 1: Un Bot por Dispositivo (Recomendado para Producción)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Ventajas:**
- ✅ OTAs completamente independientes
- ✅ Sin conflictos de mensajes
- ✅ Cada dispositivo tiene su propio chat

**Desventajas:**
- ⚠️ Más bots que gestionar
- ⚠️ Más tokens que configurar

---

**Opción 2: Un Bot, Múltiples Dispositivos Secuencialmente**
```cpp
// Todos usan el mismo BOT_TOKEN
// Pero actualízalos UNO A LA VEZ manualmente
```

**Proceso:**
1. Actualiza ESP32 #1 → Espera a completar
2. Actualiza ESP32 #2 → Espera a completar
3. etc.

**Ventajas:**
- ✅ Solo un bot que gestionar
- ✅ Más simple para pocos dispositivos

**Desventajas:**
- ⚠️ Uno por vez (lento para muchos dispositivos)
- ⚠️ Fácil equivocarse de dispositivo

---

**Opción 3: Gestión Centralizada (KissTelegram para Empresas)**
```
Sistema central con API propia
    ↓
Distribuye firmware a múltiples ESP32
    ↓
Cada ESP32 reporta progreso al sistema central
    ↓
Sistema central notifica al via Json API o usuario vía Telegram
```

**Características liked (Pero disponibles en Kisstelegram para empresas):**
- Backend web/cloud
- Base de datos de dispositivos
- Cola de trabajos OTA
- Dashboard de monitoreo

**Contribuciones bienvenidas** KissOTA tiene muchísimas posibilidades, pregunta o haz un PR.

---

## Changelog

### v0.9.0 (Actual)
- ✅ Sistema de versión simplificado
- ✅ Eliminada verificación de downgrade
- ✅ Limpieza de código obsoleto
- ✅ Mejoras en mensajes de log

### v0.1.0
- ✅ Implementación de backup/rollback automático
- ✅ Detección de boot loops
- ✅ Integración completa con Telegram

### v0.0.2
- ✅ Refactorización completa a PSRAM
- ✅ Eliminación de particiones OTA_0/OTA_1
- ✅ Sistema de validación de 60 segundos

### v0.0.1
- ✅ Versión inicial con OTA básico

---

**Documento actualizado:** 12/12/2025
**Versión del documento:** 0.9.0
**Autor** Vicente Soriano, victek@gmail.com**
**Licencia MIT**