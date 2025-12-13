# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Français** | [Documentation](GETTING_STARTED_FR.md) | [Benchmarks](BENCHMARK.md)

---

> 🚨 **PREMIÈRE UTILISATION D'ESP32-S3 AVEC KISSTELEGRAM?**
> **LISEZ CECI D'ABORD:** [**GETTING_STARTED_FR.md**](GETTING_STARTED_FR.md) ⚠️
> ESP32-S3 nécessite un **processus de chargement en deux étapes** en raison des partitions personnalisées. Ignorer ce guide causera des erreurs de démarrage!

---

## Une bibliothèque Telegram Bot robuste et de niveau entreprise pour ESP32-S3

KissTelegram est la **seule bibliothèque Telegram ESP32** conçue de zéro pour les applications critiques. Contrairement aux autres bibliothèques qui s'appuient sur la classe Arduino `String` (causant la fragmentation et les fuites de mémoire), KissTelegram utilise des tableaux `char[]` purs pour une stabilité inébranlable.

### Pourquoi KissTelegram?

- Frustré des projets perdus par les bibliothèques faibles, les fuites mémoire, les solutions de dernière minute, le manque de support, les mots à la mode, les redémarrages....

- Ma vision, maintenant les faits:

- **Zéro perte de messages**: La file d'attente persistante LittleFS survit aux plantages, redémarrages et défaillances WiFi
- **Aucune fuite mémoire**: Implémentation pure `char[]`, pas de fragmentation String
- **Sécurité SSL/TLS**: Connexions sécurisées à l'API Telegram avec validation de certificat
- **Gestion intelligente de l'énergie**: 6 modes de puissance (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Priorités de message**: CRITICAL, HIGH, NORMAL, LOW avec gestion intelligente de la file d'attente
- **Mode Turbo**: Traitement par lot pour les grandes files de messages (0,9 msg/s)
- **i18n multilingue**: Sélection de langue au moment de la compilation (7 langues, aucun surcoût à l'exécution)
- **OTA entreprise**: Mise à jour micrologicielle double démarrage avec restauration automatique et passerelle de sécurité
- **Utilisation 100% Flash**: Schéma de partition personnalisé maximise le flash ESP32-S3 16MB
- **Plus sécurisé que l'OTA Espressif**: Authentification PIN/PUK, vérification de checksum, fenêtre de validation 60s
- **Indépendant des bibliothèques externes**: Tout fait de zéro, parseur Json personnalisé.

---

## Configuration matérielle requise

- **ESP32-S3** avec **16MB Flash** / **8MB PSRAM**
- Connectivité WiFi
- Arduino IDE ou PlatformIO

---

## Installation

### Arduino IDE

1. Téléchargez ce dépôt en ZIP
2. Ouvrez Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Sélectionnez le fichier téléchargé

### PlatformIO

Ajoutez à votre `platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Schéma de partition personnalisé

KissTelegram inclut un `partitions.csv` optimisé qui maximise l'utilisation du flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB de stockage SPIFFS** - c'est 8MB de plus que les schémas par défaut d'Espressif!

Pour utiliser ce schéma de partition:
1. Copiez `partitions.csv` dans votre répertoire de projet
2. Dans Arduino IDE: Tools → Partition Scheme → Custom
3. Dans PlatformIO: `board_build.partitions = partitions.csv`

---

## Démarrage rapide

### Exemple basique

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

### Exemple de mises à jour OTA

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

**Processus OTA:**
1. Envoyez `/ota` à votre bot
2. Entrez PIN avec `/otapin YOUR_PIN`
3. Téléchargez le fichier micrologiciel `.bin`
4. Le bot vérifie automatiquement le checksum
5. Confirmez avec `/otaconfirm`
6. Après redémarrage, validez avec `/otaok` dans les 60 secondes
7. Restauration automatique en cas d'échec de la validation!

- Lisez Readme_KissOTA.md dans votre langue préférée pour en savoir plus sur la solution.

---

## Explications des fonctionnalités clés

### 1. File d'attente de messages persistante

Les messages sont stockés dans LittleFS avec suppression automatique par lot:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Survit aux plantages, déconnexions WiFi, redémarrages
- Nouvelles tentatives automatiques des envois échoués
- Suppression intelligente par lot (tous les 10 messages + quand la file est vide)
- Garantie de zéro perte de messages

### 2. Gestion de l'énergie

6 modes de puissance intelligents s'adaptent aux besoins de votre application:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Phase de démarrage initial (10s)
- **POWER_LOW**: Activité minimale, interrogation lente
- **POWER_IDLE**: Pas d'activité récente, vérifications réduites
- **POWER_ACTIVE**: Fonctionnement normal
- **POWER_TURBO**: Traitement par lot à haute vitesse (intervalles de 50ms)
- **POWER_MAINTENANCE**: Remplacement manuel pour les mises à jour
- **Décroissance temporelle pour un passage en douceur**

### 3. Priorités de message

Quatre niveaux de priorité garantissent que les messages critiques sont envoyés en premier:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La file d'attente traite: **CRITICAL → HIGH → NORMAL → LOW**
Processus internes: **OTAMODE → MAINTENANCEMODE**

### 4. Sécurité SSL/TLS

Connexions sécurisées avec validation de certificat:

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Repli automatique secure/insecure
- Vérifications périodiques pour maintenir la connexion
- Code de connexion réutilisable sauvegarde l'en-tête de connexion pour le plus haut débit

### 5. Mode Turbo

S'active automatiquement quand la file > 100 messages:

```cpp
bot.enableTurboMode();  // Activation manuelle
```

- Traite 10 messages par cycle
- Intervalles de 50ms entre les lots
- Réalise 15-20 msg/s de débit
- Désactivation automatique quand la file est envoyée

### 6. Modes de fonctionnement

Profils préconfigurés pour différents scénarios:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Par défaut (interrogation: 10s, nouvelle tentative: 3)
- **MODE_PERFORMANCE**: Rapide (interrogation: 5s, nouvelle tentative: 2)
- **MODE_POWERSAVE**: Lent (interrogation: 30s, nouvelle tentative: 2)
- **MODE_RELIABILITY**: Robuste (interrogation: 15s, nouvelle tentative: 5)

### 7. Diagnostiques

Surveillance et débogage complets:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Affiche:
- Mémoire libre (heap/PSRAM)
- Statistiques de la file de messages
- Qualité de la connexion
- Historique du mode de puissance
- Utilisation du stockage
- Uptime

---

## 8. Gestion WiFi
- WiFi Manager intégré déclenche les autres tâches jusqu'à ce que WiFi soit stable
- Prévient les conditions de course
- Redirige les messages en attente vers le stockage FS jusqu'à rétablissement de la connexion, peut stocker jusqu'à 3500 msg (par défaut mais facilement extensible)
- Surveillance de la qualité de connexion (EXCELLENT, GOOD, FAIR, POOR, DEAD) et niveau de sortie RSSI
- Vous n'avez qu'à vous occuper de votre croquis, utilisez KissTelegram pour le reste

---

## 9. Fonctionnalité phare: Commande `/estado`

**L'outil de débogage le plus puissant que vous n'ayez jamais utilisé:**

Envoyez `/estado` à votre bot et recevez un **rapport de santé complet** en quelques secondes:

```
📦 KissTelegram v1.x.x
🎯 SYSTEM RELIABILITY
✅ System: RELIABLE
✅ Messages sent: 5,234
💾 Messages pending: 0
📡 WiFi Signal: -59 dBm (Good)
🔌 WiFi reconnections: 2
⏱️ Uptime: 86,400 seconds (24h)
💾 Free memory: 223 KB
📊 Queue statistics: All systems operational
```

**Pourquoi `/estado` est essentiel:**
- ✅ Vérification instantanée de la santé du système
- ✅ Surveillance de la qualité WiFi (diagnostic des problèmes de connectivité)
- ✅ Détection des fuites mémoire (surveillance du heap libre)
- ✅ État de la file de messages (voir messages en attente/échoués)
- ✅ Suivi de l'uptime (surveillance de la stabilité)
- ✅ Disponible en 7 langues
- ✅ Votre premier outil lors du débogage de problèmes

**Pro tip:** Faites de `/estado` votre premier message après chaque mise à jour du micrologiciel pour vérifier que tout fonctionne!

---

## 10. NTP
- Code propre pour la synchronisation/resynchronisation pour SSL et Scheduler (Édition Entreprise)
---

## 11. Documentation (7 langues)

- **[GETTING_STARTED_FR.md](GETTING_STARTED_FR.md)** - **COMMENCEZ ICI!** Guide complet du déballage ESP32-S3 au premier message Telegram
- **[README_FR.md](README_FR.md)** (ce fichier) - Aperçu des fonctionnalités, démarrage rapide, référence API
- **[BENCHMARK.md](BENCHMARK.md)** - Comparaison technique avec 6 autres bibliothèques Telegram (anglais uniquement)
- **[README_KissOTA_XX.md](README_KissOTA_FR.md)** - Documentation du système de mise à jour OTA (7 langues: EN, ES, FR, IT, DE, PT, CN)

**Choisissez votre langue:** Tous les messages système KissTelegram supportent 7 langues via sélection au moment de la compilation.


## Avantages de sécurité OTA

KissTelegram OTA est **plus sécurisé que l'implémentation d'Espressif**:

| Fonctionnalité | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentification | PIN + PUK | Aucune |
| Vérification du checksum | CRC32 automatique | Manuel |
| Sauvegarde et restauration | Automatique | Manuel |
| Fenêtre de validation | 60s avec `/otaok` | Aucune |
| Détection de boucle de démarrage | Oui | Non |
| Intégration Telegram | Natif | Nécessite du code personnalisé |
| Optimisation Flash | 13MB SPIFFS | 5MB SPIFFS |

---

## Référence API

### Initialisation

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Messagerie

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configuration

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Surveillance

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### Stockage

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Exemples

Consultez les .ino inclus dans la bibliothèque pour explorer quelques scénarios et fonctionnalités et mon style de code de KissTelegram. Mieux, décommentez votre langue dans [lang.h] pour recevoir les messages des constructeurs principaux (.cpp) dans votre langue locale, si toutes les langues sont commentées, vous obtenez des messages en espagnol, la langue par défaut:

Les conventions de code sont en anglais, mais les réflexions et commentaires dans ma langue maternelle, utilisez votre traducteur en ligne, le code est facile, derrière le code c'est beaucoup plus compliqué ...

````cpp

// =========================================================================
// LANGUAGE SELECTION - Uncomment ONE language
// =========================================================================
// #define LANG_CN  // 中文
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Français
// #define LANG_IT  // Italiano
// #define LANG_PT  // Português
````

---
## Paramètres de configuration basiques
- Renommez system_setup_template.h en system_setup.h dans votre dossier KissTelegram pour commencer la compilation.
- Remplacez les lignes suivantes par vos paramètres.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licence

Ce projet est sous licence MIT - voir le fichier [LICENSE](LICENSE) pour les détails.

---

## Architecture, Vision, Concept, Solutions && Design (et le responsable de tout dysfonctionnement)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Contributeurs**
- De nombreux assistants IA dans les traductions, le code, le dépannage et les blagues.

---


## Contribution

Les contributions sont bienvenues! N'hésitez pas à soumettre une Pull Request.

---

## Support

Si vous trouvez cette bibliothèque utile, veuillez considérer:
- D'ajouter une étoile à ce dépôt
- De signaler les bugs via les GitHub Issues
- De partager vos projets utilisant KissTelegram

---

