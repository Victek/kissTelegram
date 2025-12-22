# Primeros Pasos con KissTelegram en ESP32-S3

**Guía completa para configurar tu ESP32-S3 desde cero hasta tu primer mensaje de Telegram**

> ⚠️ **CRÍTICO**: Lee esta guía completamente antes de subir cualquier firmware. El ESP32-S3 N16R8 requiere un **proceso de subida en dos pasos** debido a las particiones personalizadas. ¡Saltarse pasos causará errores!

---

## Tabla de Contenidos

1. [Antes de Empezar](#antes-de-empezar)
2. [Crear Tu Bot de Telegram](#crear-tu-bot-de-telegram)
3. [Configuración de Hardware](#configuración-de-hardware)
4. [Configuración del IDE Arduino](#configuración-del-ide-arduino)
5. [Primera Subida (Crear particiones con Arduino IDE)](#primera-subida)
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

Tu nuevo ESP32-S3 N16R8 llega con una aplicación de demostración de LED RGB incorporada. KissTelegram **reemplaza completamente la tabla de particiones** para maximizar tu flash de 16MB:

| Partición | Espressif por Defecto | KissTelegram Personalizado |
|-----------|----------------------|----------------------------|
| Espacio App | 1.5 MB | 4.5 MB (¡3x más grande!) |
| Sistema de Archivos | 5 MB | 13 MB (¡2.6x más grande!) |
| Total Usado | 6.5 MB | 17.5 MB |

Por eso se requiere el proceso de dos subidas: **la tabla de particiones cambia entre subidas**.

---

## Crear Tu Bot de Telegram

### Paso 1: Habla con BotFather

1. Abre Telegram en tu teléfono
2. Busca `@BotFather` (bot oficial, tiene marca azul de verificación)
3. Inicia conversación con `/start`
4. Crea tu bot con `/newbot`
5. Elige un nombre (ejemplo: "Mi Asistente Doméstico")
6. Elige un nombre de usuario (debe terminar en `bot`, ejemplo: "miasistente_domestico_bot")
7. **Guarda tu Token del Bot** - se ve así: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Paso 2: Obtén Tu Chat ID

**Método 1: Usando un Bot (Más Fácil)**

1. Busca `@ChatIDHelperBot` en Telegram
2. Inicia conversación con `/start`
3. Te responderá con tu **Chat ID** (un número como `123456789`)
4. **Guarda este número** - lo necesitarás en la configuración

**Método 2: Usando Navegador Web**

1. Envía cualquier mensaje a tu bot recién creado
2. Abre el navegador y visita:
   ```
   https://api.telegram.org/bot<TU_TOKEN_BOT>/getUpdates
   ```
   (Reemplaza `<TU_TOKEN_BOT>` con tu token actual)
3. Busca `"chat":{"id":123456789` en la respuesta JSON
4. Ese número es tu **Chat ID**

**✅ Ahora tienes:**
- Token del Bot: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

¡Guárdalos bien! Los necesitarás pronto.

---

## Configuración de Hardware

### Entendiendo los Dos Puertos USB-C

Tu ESP32-S3 N16R8 tiene **dos puertos USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED de Energía
│  └─┘                 │
│  [USB-C]  ← PUERTO DERECHO (Bootloader/Subida)
│                      │
│                      │
│  [USB-C]  ← PUERTO IZQUIERDO (OTG/Operación Normal)
│                      │
└─────────────────────┘
```

**PUERTO DERECHO (cerca del LED de Energía):**
- Usado para **subida inicial de firmware**
- Usado para **modo bootloader**
- Usa este cuando Arduino IDE diga "Conectando..."

**PUERTO IZQUIERDO (OTG):**
- Usado para **operación normal** después de la primera subida
- Usado para **segunda subida** (corrección de particiones)
- Usa este para el Monitor Serie en operación normal

---

## Configuración del IDE Arduino

### Paso 1: Mostrar Archivos Ocultos (Windows)

1. Abre el **Explorador de Archivos**
2. Haz clic en la pestaña **Ver** → **Mostrar** → Marca:
   - ✅ Extensiones de nombre de archivo
   - ✅ Elementos ocultos
3. En la pestaña **Filtro**: **Todos los tipos de archivo**

### Paso 2: Modificar boards.txt

1. Navega a:
   ```
   C:\Users\<TU_NOMBRE_USUARIO>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Reemplaza `3.3.4` con tu versión del núcleo ESP32 si es diferente)

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

6. Si Arduino IDE estaba abierto, **ciérralo y reinícialo**

### Paso 3: Configurar Arduino IDE

1. **Abre** tu carpeta de sketch KissTelegram (con `.ino`, `.h`, `.cpp`, y `partitions.csv`)

2. En Arduino IDE, ve a **Herramientas** → **Placa** → **4D Systems gen4-ESP32-S3R8n16**

3. **Configura todas las opciones del menú Herramientas:**

   | Ajuste | Valor |
   |---------|-------|
   | **Placa** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Habilitado |
   | **Tamaño Flash** | 16MB (128Mb) |
   | **Esquema de Particiones** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Velocidad de Subida** | 921600 |
   | **Borrar Todo el Flash Antes de Subir Sketch** | **Habilitado** ⚠️ |

   ⚠️ **Ajustes críticos** - ¡revisa dos veces!

4. **Herramientas** → **Monitor Serie** → Establece velocidad a **115200**

---

## Primera Subida (Problemas Comunes)

### Por Qué Se Necesitan Dos Subidas

**El Problema:**
- Primera subida: Arduino usa la **tabla de particiones antigua** para escribir firmware
- ESP32 arranca: Encuentra la **nueva tabla de particiones** (de `partitions.csv`)
- **Desajuste** entre dónde se escribió el firmware vs dónde ESP32 lo busca
- Resultado: Errores de arranque, errores de particiones, fallos

**La Solución:**
Dos subidas aseguran que el firmware se escriba en la **ubicación correcta** definida por la nueva tabla de particiones.

---

### Subida #1: Flash Inicial

1. **Conecta el puerto USB-C DERECHO** (cerca del LED de Energía) a tu PC

2. **Selecciona puerto**: Herramientas → Puerto → Selecciona el puerto COM que aparece

3. **Verifica ajustes**:
   - ✅ Borrar Todo el Flash Antes de Subir Sketch: **Habilitado**
   - ✅ Esquema de Particiones: **Custom (4MB APP/12MB LtlFS)**
   

4. **Herramientas, Cargar** o (`Ctrl+U`) (Haz clic en la opción que prefieras)
   - ✅ El firmware se sube.
   - Toma 53.6 segundos o mucho menos si usas una fuente de alimentación externa al ESP32s3 

Continúa con Subida #2.

---

### Subida #2: Subida de Sketch

1. **Desconecta el puerto USB-C DERECHO**

2. **Conecta el puerto USB-C IZQUIERDO** (puerto OTG) a tu PC

3. **Selecciona nuevo puerto**: Herramientas → Puerto → Selecciona el nuevo puerto COM
   - **Importante**: ¡El número de puerto cambiará! Busca datos en el Monitor Serie para confirmar el puerto correcto, por ejemplo, presiona reset del ESP32s3 hasta que veas respuesta de datos

4. **Verifica ajustes otra vez**:
   - ✅ Borrar Todo el Flash Antes de Subir Sketch: **Habilitado**
   - ✅ Esquema de Particiones: **Custom (4MB APP/12MB LtlFS)**

5. **Presiona Subir de nuevo** (`Ctrl+U`)

6. **Espera ~2-3 minutos** (borrando + subiendo, depende si usas fuente de alimentación externa)

7. **Abre Monitor Serie** - ahora deberías ver (si has configurado correctamente las credenciales 
en system_setup.h (el system_setup_template renombrado)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi conectado
   ✅ Bot de Telegram habilitado
   ✅ Sistema listo
   ```

8. **Revisa Telegram** - recibirás mensaje de bienvenida:
   ```
   📦 ¡Hola! KissTelegram está listo.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Señal WiFi: -59 dBm (Buena)
   ✅ 0 mensajes en cola
   ```

**¡Éxito!** Tu ESP32-S3 ahora ejecuta KissTelegram con las particiones correctas.

---

### Subidas Futuras

**Buenas noticias:** Después de las dos subidas iniciales, todas las subidas futuras funcionan normalmente:

- Usa **puerto USB-C IZQUIERDO** (OTG)
- **No necesitas** "Borrar Todo el Flash" más (a menos que hayas hecho cambios en datos NVRAM)
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

**⚠️ Advertencia de Seguridad:** ¡Cambia el PIN por defecto (`0000`) y PUK (`00000000`) por tus propios secretos!

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
// #define LANG_ES  // Español (Español) - POR DEFECTO si todo está comentado
```

Elige tu idioma (descomenta) **antes de compilar** para mensajes localizados.

---

## ¡Éxito! ¿Qué Sigue?

### Verifica Que Todo Funciona

1. **Envía `/status` a tu bot** en Telegram - obtendrás un reporte de estado detallado:
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

2. **Revisa Monitor Serie** - no debe mostrar errores

3. **Prueba comandos**:
   - `/start` - Mensaje de bienvenida
   - `/help` - Comandos disponibles
   - `/status` - Estado del sistema (chequeo de salud)

---

### Entendiendo las Actualizaciones OTA

Una vez que KissTelegram está ejecutándose, puedes actualizar el firmware **vía Telegram** (¡sin cable USB!):

1. Envía `/ota` a tu bot
2. Introduce PIN: `/otapin 0000` (o tu PIN personalizado)
3. **Envía tu archivo de firmware `.bin`** (arrastra y suelta en Telegram)
4. El bot verifica el checksum automáticamente
5. Confirma: `/otaconfirm`
6. ESP32 reinicia con el nuevo firmware
7. **Dentro de 60 segundos**, envía `/otaok` para confirmar que funciona
8. Si no confirmas, ¡ESP32 **automáticamente revierte** al firmware anterior!

📖 **Lee más:** Ver `README_KissOTA_ES.md` para documentación completa de OTA.

---

### Explora Código de Ejemplo

El ejemplo `suite_kiss.ino` demuestra:

- ✅ Gestión WiFi con monitoreo de calidad
- ✅ Cola de mensajes con prioridades
- ✅ Modos de gestión de energía
- ✅ Manejo de comandos (`/start`, `/help`, `/status`, etc.)
- ✅ Actualizaciones OTA vía Telegram
- ✅ Recuperación de fallos y persistencia
- ✅ Conexiones SSL/TLS seguras

**Consejo profesional:** Usa el comando `/status` como tu **herramienta de monitoreo de salud** - ¡es tu ventana a los internos de KissTelegram!

---

### Solución de Problemas Comunes

**Problema: "Puerto no encontrado" o "Acceso denegado"**
- Windows bloqueó el puerto. Desconecta USB, espera 5s, reconecta.
- Prueba diferente cable USB (algunos son solo carga, no datos)

**Problema: "Tiempo agotado esperando dispositivo" durante subida**
- ¡Puerto USB equivocado! Recuerda: puerto DERECHO para primera subida, puerto IZQUIERDO para segunda
- Mantén presionado botón BOOT en ESP32 mientras haces clic en Subir, suelta después de que aparezca "Conectando..."

**Problema: Monitor Serie muestra caracteres basura**
- Velocidad de baudios incorrecta. Establece a **115200** en el desplegable del Monitor Serie

**Problema: El bot no responde en Telegram**
- Verifica que `system_setup.h` tiene el Token del Bot y Chat ID correctos
- Verifica que las credenciales WiFi son correctas
- Abre Monitor Serie y busca mensajes de conexión WiFi

**Problema: Error de compilación "La tabla de particiones no cabe"**
- No agregaste la partición personalizada a `boards.txt` correctamente
- O no seleccionaste "Custom (4MB APP/12MB LtlFS)" en Herramientas → Esquema de Particiones

---

### Obtén Más Ayuda

- 📧 **Email**: victek@gmail.com
- 📖 **Documentación**: Ver todos los archivos `README_*.md` en tu carpeta KissTelegram
- 🐛 **Reportes de Errores**: Incidencias en GitHub (enlace en README.md principal)
- 💡 **Solicitudes de Funcionalidades**: ¡También bienvenidas vía email o GitHub!

---

## Resumen: El Proceso Completo

```
1. Obtener Token del Bot + Chat ID de Telegram ✅
2. Modificar boards.txt (agregar partición personalizada) ✅
3. Configurar Arduino IDE (Partición personalizada, Borrar habilitado) ✅
4. Editar system_setup.h (credenciales) ✅
5. Conectar puerto USB DERECHO ✅
6. Subida #1 (nuevas particiones)✅
7. Desconectar DERECHO, conectar puerto USB IZQUIERDO ✅
8. Subida #2 (Subir Sketch KissTelegram) ✅
9. Recibir mensaje de bienvenida en Telegram ✅
10. Enviar /status para verificar que todo funciona ✅
```

**¡Estás listo para construir proyectos increíbles con KissTelegram!** 🎉
