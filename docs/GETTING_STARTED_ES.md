# Primeros Pasos con KissTelegram en ESP32-S3

**Guía completa para configurar tu ESP32-S3 desde cero hasta el primer mensaje de Telegram**

> ⚠️ **CRÍTICO**: Lee esta guía completamente antes de subir ningún firmware. El ESP32-S3 N16R8 requiere un **proceso de subida en dos pasos** debido a las particiones personalizadas. ¡Saltarte pasos causará errores!

---

## Tabla de Contenidos

1. [Antes de Empezar](#antes-de-empezar)
2. [Crear tu Bot de Telegram](#crear-tu-bot-de-telegram)
3. [Configuración del Hardware](#configuración-del-hardware)
4. [Configuración del Arduino IDE](#configuración-del-arduino-ide)
5. [Primera Subida (Crear particiones con esptool)](#primera-subida)
6. [Archivos de Configuración](#archivos-de-configuración)
7. [¡Éxito! ¿Qué Sigue?](#éxito-qué-sigue)

---

## Antes de Empezar

### Lo Que Necesitas

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Dos cables USB-C** (para alternar entre puertos bootloader y OTG)
- **Arduino IDE 2.x** o superior
- **PC con Windows** (esta guía está enfocada a Windows, adapta las rutas para Linux/Mac)
- **Cuenta de Telegram** en tu teléfono

### Lo Que Hace Esto Diferente

Tu nuevo ESP32-S3 N16R8 llega con una app demo del LED RGB integrado. KissTelegram **reemplazará completamente la tabla de particiones** para maximizar tus 16MB de flash:

| Partición | Default Espressif | KissTelegram Custom |
|-----------|-------------------|---------------------|
| Espacio App | 1.5 MB | 4.5 MB (¡3x más grande!) |
| Sistema de Archivos | 5 MB | 13 MB (¡2.6x más grande!) |
| Total Usado | 6.5 MB | 17.5 MB |

Por esto se requiere el proceso de dos subidas: **la tabla de particiones cambia entre subidas**.

---

## Crear tu Bot de Telegram

### Paso 1: Hablar con BotFather

1. Abre Telegram en tu teléfono
2. Busca `@BotFather` (bot oficial, tiene marca de verificación azul)
3. Inicia conversación con `/start`
4. Crea tu bot con `/newbot`
5. Elige un nombre (ejemplo: "Mi Asistente Doméstico")
6. Elige un nombre de usuario (debe terminar en `bot`, ejemplo: "miasistente_domestico_bot")
7. **Guarda tu Bot Token** - se ve así: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Paso 2: Obtener tu Chat ID

**Método 1: Usando un Bot (Más Fácil)**

1. Busca `@ChatIDHelperBot` en Telegram
2. Inicia conversación con `/start`
3. Te responderá con tu **Chat ID** (un número como `123456789`)
4. **Guarda este número** - lo necesitarás en la configuración

**Método 2: Usando Navegador Web**

1. Envía cualquier mensaje a tu bot recién creado
2. Abre el navegador y visita:
   ```
   https://api.telegram.org/bot<TU_BOT_TOKEN>/getUpdates
   ```
   (Reemplaza `<TU_BOT_TOKEN>` con tu token real)
3. Busca `"chat":{"id":123456789` en la respuesta JSON
4. Ese número es tu **Chat ID**

**✅ Ahora tienes:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

¡Guárdalos bien! Los necesitarás pronto.

---

## Configuración del Hardware

### Entender los Dos Puertos USB-C

Tu ESP32-S3 N16R8 tiene **dos puertos USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED Power    │
│  └─┘                 │
│  [USB-C]  ← PUERTO DERECHO (Bootloader/Subida)
│                      │
│                      │
│  [USB-C]  ← PUERTO IZQUIERDO (OTG/Operación Normal)
│                      │
└─────────────────────┘
```

**PUERTO DERECHO (cerca del LED Power):**
- Usado para **subida inicial del firmware**
- Usado para **modo bootloader**
- Usa este cuando Arduino IDE diga "Connecting..."

**PUERTO IZQUIERDO (OTG):**
- Usado para **operación normal** después de la primera subida
- Usado para **segunda subida** (arreglo de particiones)
- Usa este para el Monitor Serie en operación normal

---

## Configuración del Arduino IDE

### Paso 1: Mostrar Archivos Ocultos (Windows)

1. Abre el **Explorador de Archivos**
2. Click en pestaña **Ver** → **Mostrar** → Marca:
   - ✅ Extensiones de nombre de archivo
   - ✅ Elementos ocultos
3. En pestaña **Filtro**: **Todos los tipos de archivo**

### Paso 2: Modificar boards.txt

1. Navega a:
   ```
   C:\Users\<TU_NOMBRE_USUARIO>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Reemplaza `3.3.4` con tu versión del core ESP32 si es diferente)

2. Encuentra y abre `boards.txt` (usa Notepad++ o cualquier editor de texto)

3. Presiona `Ctrl+F` y busca:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Inmediatamente debajo de esa línea**, pega estas tres líneas:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Guarda** y cierra `boards.txt`

6. Si Arduino IDE estaba abierto, **cierra y reinícialo**

### Paso 3: Configurar Arduino IDE

1. **Abre** tu carpeta de sketch KissTelegram (con `.ino`, `.h`, `.cpp`, y `partitions.csv`)

2. En Arduino IDE, ve a **Herramientas** → **Placa** → **4D Systems gen4-ESP32-S3R8n16**

3. **Herramientas** → **Reload Board Data** (verás confirmación en la parte inferior)

4. **Configura todas las opciones del menú Herramientas:**

   | Ajuste | Valor |
   |---------|-------|
   | **Placa** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Ajustes críticos** - ¡verifica dos veces!

5. **Herramientas** → **Monitor Serie** → Configura la velocidad a **115200**

---

## Primera Subida (Problemas Comunes)

### Por Qué Se Necesitan Dos Subidas

**El Problema:**
- Primera subida: Arduino usa la **tabla de particiones antigua** para escribir el firmware
- ESP32 arranca: Encuentra la **nueva tabla de particiones** (de `partitions.csv`)
- **Desajuste** entre donde se escribió el firmware vs donde el ESP32 lo busca
- Resultado: Errores de arranque, errores de partición, crashes

**La Solución:**
Dos subidas aseguran que el firmware se escriba en la **ubicación correcta** definida por la nueva tabla de particiones.

---

### Subida #1: Flash Inicial (Graba el boot loader)

1. **Conecta el puerto USB-C DERECHO** (cerca del LED Power) a tu PC

2. **Selecciona el puerto**: Herramientas → Puerto → Selecciona el puerto COM que aparece

3. **Verifica los ajustes**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**
   

4. **Herramientas, Grabar el Bootloader** ( Presiona esta opción)
   - ✅ Herramientas ➡️, y al final del desplegable encuentras 'Grabar el Bootloader'
   - ✅ Pulsa aquí y grabará la nueva partición, está usando esptool
   - Tarda 53.6 segundos y ya tienes la nueva partición para KissTelegram 

Continúa con la Subida #2.

---

### Subida #2: Subida del sketch

1. **Desconecta el puerto USB-C DERECHO**

2. **Conecta el puerto USB-C IZQUIERDO** (puerto OTG) a tu PC

3. **Selecciona el nuevo puerto**: Herramientas → Puerto → Selecciona el nuevo puerto COM
   - **Importante**: ¡El número de puerto cambiará! Busca datos en el Monitor Serie para confirmar el puerto correcto, por ejemplo, pulsa el reset del ESP32s3 hasta que veas datos como respuesta

4. **Verifica los ajustes de nuevo**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Presiona Subir de nuevo** (`Ctrl+U`)

6. **Espera ~2-3 minutos** (borrando + subiendo)

7. **Abre el Monitor Serie** - ahora deberías ver (si has ajustado correctamente las credenciales 
en system_setup.h (el system_setup_template que has renombrado)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi conectado
   ✅ Bot de Telegram habilitado
   ✅ Sistema listo
   ```

8. **Verifica Telegram** - recibirás el mensaje de bienvenida:
   ```
   📦 ¡Hola! KissTelegram está listo.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Señal WiFi: -59 dBm (Buena)
   ✅ 0 mensajes en cola
   ```

**¡Éxito!** Tu ESP32-S3 ahora ejecuta KissTelegram con las particiones correctas.

---

### Subidas Futuras

**Buenas noticias:** Después de las dos subidas iniciales, todas las futuras subidas funcionan normalmente:

- Usa el **puerto USB-C IZQUIERDO** (OTG)
- **No necesitas** "Erase All Flash" nunca más (a menos que hayas hecho cambios en los datos de la NVRAM)
- Sube una vez y funciona inmediatamente

---

## Archivos de Configuración

### system_setup.h (¡Requerido Antes de la Primera Subida!)

**Antes de compilar:**

1. Navega a tu carpeta KissTelegram
2. Encuentra `system_setup_template.h`
3. **Renómbralo** a `system_setup.h`
4. **Abre** `system_setup.h` y completa:

```cpp
// Tu Bot de Telegram (de BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Tu Chat ID (de @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Tus credenciales WiFi
#define KISS_FALLBACK_WIFI_SSID "NombreDeTuWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "ContraseñaDeTuWiFi"

// Seguridad OTA (¡cambia el PIN/PUK por defecto!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 dígitos
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 dígitos
```

5. **Guarda** el archivo

**⚠️ Advertencia de Seguridad:** ¡Cambia el PIN por defecto (`0000`) y el PUK (`00000000`) por tus propios secretos!

---

### lang.h (Opcional: Elige Tu Idioma)

KissTelegram soporta 7 idiomas para mensajes del sistema:

```cpp
// En lang.h, descomenta UN idioma:

// #define LANG_CN  // 中文 (Chino)
// #define LANG_DE  // Deutsch (Alemán)
// #define LANG_EN  // English (Inglés)
// #define LANG_FR  // Français (Francés)
// #define LANG_IT  // Italiano (Italiano)
// #define LANG_PT  // Português (Portugués)
// #define LANG_ES  // Español (Español) - POR DEFECTO si todos están comentados
```

Elige tu idioma **antes de compilar** para mensajes localizados.

---

## ¡Éxito! ¿Qué Sigue?

### Verificar Que Todo Funciona

1. **Envía `/estado` a tu bot** en Telegram - obtendrás un reporte de estado detallado:
   ```
   📦 KissTelegram v1.x.x
   🎯 FIABILIDAD DEL SISTEMA
   ✅ Sistema: FIABLE
   ✅ Mensajes enviados: 2
   💾 Mensajes pendientes: 0
   📡 Señal WiFi: -59 dBm (Buena)
   🔋 Tiempo activo: 123 segundos
   💾 Memoria libre: 223 KB
   ```

2. **Verifica el Monitor Serie** - no debería mostrar errores

3. **Prueba los comandos**:
   - `/start` - Mensaje de bienvenida
   - `/help` - Comandos disponibles
   - `/estado` - Estado del sistema (verificación de salud)

---

### Entender las Actualizaciones OTA

Una vez que KissTelegram esté funcionando, puedes actualizar el firmware **vía Telegram** (¡sin cable USB!):

1. Envía `/ota` a tu bot
2. Introduce el PIN: `/otapin 0000` (o tu PIN personalizado)
3. **Envía tu archivo de firmware `.bin`** (arrastra y suelta en Telegram)
4. El bot verifica el checksum automáticamente
5. Confirma: `/otaconfirm`
6. El ESP32 reinicia con el nuevo firmware
7. **Dentro de 60 segundos**, envía `/otaok` para confirmar que funciona
8. Si no confirmas, ¡el ESP32 **vuelve automáticamente** al firmware anterior!

📖 **Leer más:** Ver `README_KissOTA_ES.md` para documentación completa de OTA.

---

### Explorar el Código de Ejemplo

El ejemplo `suite_kiss.ino` demuestra:

- ✅ Gestión WiFi con monitoreo de calidad
- ✅ Cola de mensajes con prioridades
- ✅ Modos de gestión de energía
- ✅ Manejo de comandos (`/start`, `/help`, `/estado`, etc.)
- ✅ Actualizaciones OTA vía Telegram
- ✅ Recuperación de crashes y persistencia
- ✅ Conexiones seguras SSL/TLS

**Consejo profesional:** ¡Usa el comando `/estado` como tu **herramienta de monitoreo de salud** - es tu ventana a los internos de KissTelegram!

---

### Solución de Problemas Comunes

**Problema: "Puerto no encontrado" o "Acceso denegado"**
- Windows bloqueó el puerto. Desconecta USB, espera 5s, vuelve a conectar.
- Prueba un cable USB diferente (algunos son solo para carga, no para datos)

**Problema: "Timeout esperando dispositivo" durante la subida**
- ¡Puerto USB incorrecto! Recuerda: puerto DERECHO para primera subida, puerto IZQUIERDO para segunda
- Mantén presionado el botón BOOT en el ESP32 mientras haces click en Subir, suéltalo después de que aparezca "Connecting..."

**Problema: Monitor Serie muestra caracteres basura**
- Velocidad de baudios incorrecta. Configúrala a **115200** en el menú desplegable del Monitor Serie

**Problema: El bot no responde en Telegram**
- Verifica que `system_setup.h` tiene el Bot Token y Chat ID correctos
- Verifica que las credenciales WiFi son correctas
- Abre el Monitor Serie y busca mensajes de conexión WiFi

**Problema: Error de compilación "Tabla de particiones no cabe"**
- No añadiste la partición personalizada a `boards.txt` correctamente
- O no seleccionaste "Custom (4MB APP/12MB LtlFS)" en Herramientas → Partition Scheme

---

### Obtener Más Ayuda

- 📧 **Email**: victek@gmail.com
- 📖 **Documentación**: Ver todos los archivos `README_*.md` en tu carpeta KissTelegram
- 🐛 **Reportes de Bugs**: GitHub issues (enlace en README.md principal)
- 💡 **Solicitudes de Características**: ¡También bienvenidas vía email o GitHub!

---

## Resumen: El Proceso Completo

```
1. Obtener Bot Token + Chat ID de Telegram ✅
2. Modificar boards.txt (añadir partición personalizada) ✅
3. Configurar Arduino IDE (Partición custom, Erase habilitado) ✅
4. Editar system_setup.h (credenciales) ✅
5. Conectar puerto USB DERECHO ✅
6. Subida #1 (esperar errores) ✅
7. Desconectar DERECHO, conectar puerto USB IZQUIERDO ✅
8. Subida #2 (¡errores desaparecidos!) ✅
9. Recibir mensaje de bienvenida en Telegram ✅
10. Enviar /estado para verificar que todo funciona ✅
```

**¡Estás listo para construir proyectos increíbles con KissTelegram!** 🎉

---

**¡Feliz programación!**

*Vicente Soriano - victek@gmail.com*
