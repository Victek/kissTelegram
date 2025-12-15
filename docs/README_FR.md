# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Français** | [Documentation](docs/GETTING_STARTED_FR.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **PREMIÈRE UTILISATION DE L'ESP32-S3 AVEC KISSTELEGRAM?**
> **LISEZ CECI D'ABORD:** [**GETTING_STARTED_FR.md**](docs/GETTING_STARTED_FR.md)
> L'ESP32-S3 nécessite un **processus de téléchargement en deux étapes** en raison des partitions personnalisées. Ignorer ce guide causera des erreurs de démarrage et de mauvaises partitions!

---

## Une Bibliothèque Robuste de Niveau Entreprise pour les Bots Telegram sur ESP32-S3

KissTelegram est la **seule bibliothèque Telegram pour ESP32** construite de zéro pour les applications critiques. Contrairement à d'autres bibliothèques qui dépendent de la classe `String` d'Arduino (causant fragmentation de mémoire et fuites), KissTelegram utilise des tableaux purs `char[]` pour une stabilité inébranlable.

### Pourquoi KissTelegram?

- Fatigué de projets perdus à cause de bibliothèques faibles, fuites de mémoire, solutions de dernière minute, manque de support, paroles vides, termes qui ne fonctionnent pas, redémarrages....

- C'était ma vision et mon expérience avec d'autres bibliothèques et voici le résultat avec KissTelegram:

- **Zéro Perte de Messages**: File persistante sur LittleFS qui survit aux crashes, redémarrages et pannes WiFi
- **Pas de Fuites Mémoire**: Implémentation pure `char[]`, pas de fragmentation String
- **Sécurité SSL/TLS**: Connexions sécurisées à l'API Telegram avec validation de certificat (jusqu'en 2035)
- **Gestion Intelligente de l'Énergie**: 6 modes d'alimentation (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Priorités de Messages**: CRITICAL, HIGH, NORMAL, LOW avec gestion intelligente de la file
- **Mode Turbo**: Traitement par lots pour grandes files de messages (0,9 msg/s)
- **i18n Multilingue**: Sélection de langue à la compilation pour l'envoi de messages (7 langues, zéro surcharge à l'exécution)
- **OTA Entreprise**: Mises à jour firmware double démarrage avec retour automatique et gestion de sécurité
- **Utilisation Flash à 100%**: Schéma de partition personnalisé maximisant le flash 16MB de l'ESP32-S3
- **Plus Sûr que l'OTA d'Espressif**: Authentification PIN/PUK, vérification checksum, fenêtre de validation de 60s
- **Indépendant des bibliothèques externes**: Tout construit de zéro, propre parseur JSON pour les bibliothèques KissTelegram.

---

## Exigences Matérielles

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

## Schéma de Partition Personnalisé

KissTelegram inclut un `partitions.csv` optimisé qui maximise l'utilisation du flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB de stockage SPIFFS** - C'est 8MB de plus que les schémas par défaut d'Espressif!

Pour utiliser ce schéma de partition:
1. Copiez `partitions.csv` dans votre répertoire de projet
2. Dans Arduino IDE: Tools ->Partition Scheme ->Custom
3. Dans PlatformIO: `board_build.partitions = partitions.csv`

---

## Démarrage Rapide

### Exemple de Base

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

### Exemple de Mise à Jour OTA

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
2. Entrez le PIN avec `/otapin YOUR_PIN`
3. Téléchargez le fichier firmware `.bin`
4. Le bot vérifie automatiquement le checksum
5. Confirmez avec `/otaconfirm`
6. Après redémarrage, validez avec `/otaok` dans les 60 secondes
7. Retour automatique si la validation échoue!

- Lisez Readme_KissOTA.md dans votre langue préférée pour en savoir plus sur la solution.

---

## Fonctionnalités Clés Expliquées

### 1. File de Messages Persistante

Les messages sont stockés sur LittleFS avec suppression automatique par lots:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Survit aux crashes, déconnexions WiFi, redémarrages
- Réessai automatique des envois échoués
- Suppression intelligente par lots (tous les 10 messages + quand la file est vide)
- Garantie zéro perte de messages

### 2. Gestion de l'Énergie

6 modes d'alimentation intelligents s'adaptent aux besoins de votre application:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Phase de démarrage initiale (10s)
- **POWER_LOW**: Activité minimale, interrogation lente
- **POWER_IDLE**: Pas d'activité récente, vérifications réduites
- **POWER_ACTIVE**: Opération normale
- **POWER_TURBO**: Traitement par lots à haute vitesse (intervalles de 50ms)
- **POWER_MAINTENANCE**: Override manuel pour mises à jour
- **Timing de décroissance pour transitions douces**

### 3. Priorités de Messages

Quatre niveaux de priorité assurent que les messages critiques sont envoyés en premier, sautant par-dessus ceux de priorité inférieure:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La file traite: **CRITICAL /HIGH /NORMAL /LOW**
Processus internes: **OTAMODE /MAINTENANCEMODE**

### 4. Sécurité SSL/TLS

Connexions sécurisées avec validation de certificat (jusqu'en 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Basculement automatique entre sécurisé et non sécurisé
- Vérifications ping périodiques pour maintenir la connexion
- Code de connexion réutilisable économise l'overhead de connexion pour performance maximale

### 5. Mode Turbo

Activé automatiquement lors de l'envoi de lots avec la commande /llenar:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Traite 10 messages par cycle
- Intervalles de 50ms entre les lots
- Atteint un débit de 0,9 msg/s
- Se désactive automatiquement quand la file est envoyée

### 6. Modes d'Opération

Profils préconfigurés pour différents scénarios:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Par défaut (interrogation: 10s, réessai: 3)
- **MODE_PERFORMANCE**: Rapide (interrogation: 5s, réessai: 2)
- **MODE_POWERSAVE**: Lent (interrogation: 30s, réessai: 2)
- **MODE_RELIABILITY**: Robuste (interrogation: 15s, réessai: 5)

### 7. Diagnostics

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
- Qualité de connexion
- Historique du mode d'alimentation
- Utilisation du stockage
- Temps de fonctionnement

---

## 8. Gestion WiFi
- Gestionnaire WiFi intégré active d'autres tâches uniquement une fois que le WiFi est stable
- Prévient les conditions de course
- Récupère les messages en cours vers le stockage FS jusqu'à ce que la connexion soit rétablie, peut contenir jusqu'à 3500 msg (par défaut mais facilement extensible, dépend de l'espace que vous voulez utiliser)
- Surveillance de la qualité de connexion (EXCELLENT, GOOD, FAIR, POOR, DEAD) et niveau de sortie RSSI
- Vous n'avez qu'à vous occuper de votre sketch, ajoutez votre code et KissTelegram gère les tâches critiques, WiFi, SSL, Messages, OTA, Gestion Énergie, Priorités.. tout le travail que vous économisez....

---

## 9. Fonctionnalité Clé: Commande `/estado`

**L'outil de débogage le plus puissant que vous inclurez jamais dans votre sketch**

Envoyez `/estado` à votre bot et obtenez un **rapport complet de santé de votre sketch** à ce moment, (disponible en 7 langues):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ FIABILITÉ SYSTÈME
✓ Système: FIABLE
✓ Messages envoyés: 940
📨 Messages en attente: 70
✓ Messages perdus: 0
🗑️ Rejets (file pleine): 0

⚠️ ADVERSITÉS EXTERNES
⚠️ Erreurs totales: 0
🔄 Récupérés (fallback): 0
📡 Chutes WiFi: 0

📊 INFORMATIONS TECHNIQUES
⏱️ Temps de fonctionnement: 0h 0m
🧠 RAM libre: 223960 bytes
💾 PSRAM libre: 1027820 bytes
💽 FS libre: 13549568 bytes
📦 Max. dans FS: 3500 Messages
⚡ Mode Énergie: 3 
📶 Signal WiFi: -64 dBm (Moyen)
🔒 SSL: SÉCURISÉ
🚀 Turbo: INACTIF
🤖 Auto-messages: OUI

```

**Pourquoi `/estado` est essentiel:**
- Vérification instantanée de la santé du système
- Surveillance de la qualité WiFi (diagnostiquer les problèmes de connectivité)
- Détection de fuites mémoire (surveiller le heap libre)
- État de la file de messages (voir les messages en attente/échoués)
- Suivi du temps de fonctionnement (surveillance de stabilité)
- Votre premier outil de diagnostic

**Astuce pro:** Faites de `/estado` votre premier message après chaque mise à jour firmware pour vérifier que tout fonctionne!

---

## 10. NTP
- Code propre pour synchroniser/resynchroniser pour SSL. GNSS, LTE et Scheduler (Édition Entreprise)
---

## 11. Documentation (7 Langues)

- **[GETTING_STARTED_FR.md](docs/GETTING_STARTED_FR.md)** - **COMMENCEZ ICI!** Guide complet depuis la réception de l'ESP32-S3 jusqu'au premier message envoyé à Telegram
- **[README_FR.md](docs/README_FR.md)** (ce fichier) - Aperçu des fonctionnalités, démarrage rapide, référence API
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Comparaison technique avec 6 bibliothèques Telegram (anglais uniquement, mais auto-explicatif)
- **[README_KissOTA_XX.md](docs/README_KissOTA_FR.md)** - Grande valeur car détaille les étapes du système de mise à jour OTA (7 langues: EN, ES, FR, IT, DE, PT, CN)

**Choisissez votre langue:** Tous les messages du constructeur envoyés à Telegram sont affichés en 7 langues via la sélection de langue pendant la compilation (lang.h).


## Avantages de Sécurité OTA

KissTelegram OTA est **beaucoup plus sûr que l'architecture d'Espressif et économise de l'espace sur votre ESP32S3**:

| Fonctionnalité | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentification | PIN + PUK | Aucune |
| Confirmation Checksum | CRC32 automatique | Manuel |
| Sauvegarde et Retour | Automatique | Manuel |
| Fenêtre de Validation | 60s avec `/otaok` | Aucune |
| Détection Boucle Démarrage | Oui | Non |
| Intégration Telegram | Native | Nécessite code personnalisé |
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

Consultez le .ino inclus dans la bibliothèque pour explorer quelques scénarios et fonctionnalités et mon style de codage dans KissTelegram. Encore mieux, décommentez votre langue dans [lang.h] pour recevoir des messages des principaux constructeurs (.cpp) dans votre langue locale, si toutes les langues sont commentées les messages sont en espagnol, la langue par défaut:

Les conventions de code sont en anglais, mais les pensées et commentaires sont en espagnol, ma langue maternelle, utilisez votre traducteur en ligne, le code est facile, dans le code se trouve ma vision et le concept de KissTelegram...

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
## Configuration de Base
- Renommez system_setup_template.h en system_setup.h dans votre dossier KissTelegram pour commencer la compilation.
- Remplacez les lignes suivantes par vos identifiants.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licence

Ce projet est sous licence MIT - voir le fichier [LICENSE](LICENSE) pour plus de détails.

---

## Architecture, Vision, Concept, Solutions et Design (et responsable de tout dysfonctionnement, c'est moi...)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Contributeurs**
- Nombreux assistants IA en Traductions, Code, Dépannage et beaucoup d'heures à essayer de les empêcher de réinventer la roue.....

---


## Contribuer

Les contributions sont les bienvenues! N'hésitez pas à soumettre une Pull Request ou à m'envoyer un email, mais je préfère un PR pour que d'autres puissent trouver votre question.

---

## Support

Si vous trouvez cette bibliothèque utile, veuillez considérer:
- Mettre une étoile à ce dépôt
- Signaler les bugs via GitHub Issues
- Partager vos projets utilisant KissTelegram
- Parler à vos connaissances des fonctionnalités et solutions de cette bibliothèque
- Proposer des cas d'utilisation et expériences avec KissTelegram

---
