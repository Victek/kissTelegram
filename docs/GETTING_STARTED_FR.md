# Démarrage avec KissTelegram sur ESP32-S3

**Un guide complet pour configurer votre ESP32-S3 de zéro à premier message Telegram**

> ⚠️ **CRITIQUE**: Lisez ce guide complètement avant de télécharger tout micrologiciel. L'ESP32-S3 N16R8 nécessite un **processus de téléchargement en deux étapes** en raison de partitions personnalisées. Sauter des étapes causera des erreurs!

---

## Table des matières

1. [Avant de commencer](#avant-de-commencer)
2. [Créer votre bot Telegram](#créer-votre-bot-telegram)
3. [Configuration matérielle](#configuration-matérielle)
4. [Configuration Arduino IDE](#configuration-arduino-ide)
5. [Premier téléchargement (Problèmes courants)](#premier-téléchargement-problèmes-courants)
6. [Fichiers de configuration](#fichiers-de-configuration)
7. [Succès! Et après?](#succès-et-après)

---

## Avant de commencer

### Ce dont vous avez besoin

- **ESP32-S3 N16R8** (16 MB Flash / 8 MB PSRAM)
- **Deux câbles USB-C** (pour alterner entre les ports bootloader et OTG)
- **Arduino IDE 2.x** ou plus récent
- **PC Windows** (ce guide est centré sur Windows, adaptez les chemins pour Linux/Mac)
- **Compte Telegram** sur votre téléphone

### Ce qui rend cela différent

Votre nouvel ESP32-S3 N16R8 arrive avec un programme démo LED RGB. KissTelegram va **complètement remplacer la table de partitions** pour maximiser votre flash 16 MB:

| Partition | Par défaut Espressif | KissTelegram personnalisé |
|-----------|----------------------|---------------------------|
| Espace App | 1,5 MB | 4,5 MB (3x plus grand!) |
| Système de fichiers | 5 MB | 13 MB (2,6x plus grand!) |
| Total utilisé | 6,5 MB | 17,5 MB |

C'est pourquoi le processus de double téléchargement est nécessaire: **la table de partitions change entre les téléchargements**.

---

## Créer votre bot Telegram

### Étape 1: Parler à BotFather

1. Ouvrez Telegram sur votre téléphone
2. Recherchez `@BotFather` (bot officiel, a une coche bleue de vérification)
3. Démarrez une conversation avec `/start`
4. Créez votre bot avec `/newbot`
5. Choisissez un nom (exemple: "Mon Assistant Maison")
6. Choisissez un nom d'utilisateur (doit se terminer par `bot`, exemple: "mon_assistant_maison_bot")
7. **Enregistrez votre Bot Token** - ressemble à: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Étape 2: Obtenir votre ID de conversation

**Méthode 1: Utiliser un bot (La plus facile)**

1. Recherchez `@userinfobot` dans Telegram
2. Démarrez une conversation avec `/start`
3. Il répondra avec votre **ID de conversation** (un nombre comme `123456789`)
4. **Enregistrez ce numéro** - vous en aurez besoin dans la configuration

**Méthode 2: Utiliser le navigateur Web**

1. Envoyez n'importe quel message à votre bot nouvellement créé
2. Ouvrez le navigateur et visitez:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   (Remplacez `<YOUR_BOT_TOKEN>` par votre jeton réel)
3. Recherchez `"chat":{"id":123456789` dans la réponse JSON
4. Ce numéro est votre **ID de conversation**

**✅ Maintenant vous avez:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- ID de conversation: `123456789`

Conservez-les en sécurité! Vous en aurez besoin bientôt.

---

## Configuration matérielle

### Comprendre les deux ports USB-C

Votre ESP32-S3 N16R8 a **deux ports USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← Power LED    │
│  └─┘                 │
│  [USB-C]  ← RIGHT PORT (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← LEFT PORT (OTG/Normal Operation)
│                      │
└─────────────────────┘
```

**PORT DROIT (près du LED d'alimentation):**
- Utilisé pour le **téléchargement initial du micrologiciel**
- Utilisé pour le **mode bootloader**
- Utilisez ceci quand Arduino IDE dit "Connexion..."

**PORT GAUCHE (OTG):**
- Utilisé pour l'**opération normale** après le premier téléchargement
- Utilisé pour le **second téléchargement** (correction de partition)
- Utilisez ceci pour Serial Monitor en opération normale

---

## Configuration Arduino IDE

### Étape 1: Afficher les fichiers cachés (Windows)

1. Ouvrez l'**Explorateur de fichiers**
2. Cliquez sur l'onglet **Affichage** → **Afficher** → Cochez:
   - ✅ Extensions de nom de fichier
   - ✅ Éléments masqués
3. Dans l'onglet **Filtre**: **Tous les types de fichiers**

### Étape 2: Modifier boards.txt

1. Accédez à:
   ```
   C:\Users\<YOUR_USERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Remplacez `3.3.4` par votre version de noyau ESP32 si différente)

2. Trouvez et ouvrez `boards.txt` (utilisez Notepad++ ou n'importe quel éditeur de texte)

3. Appuyez sur `Ctrl+F` et recherchez:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Immédiatement sous cette ligne**, collez ces trois lignes:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Enregistrez** et fermez `boards.txt`

6. Si Arduino IDE était ouvert, **fermez-la et redémarrez-la**

### Étape 3: Configurer Arduino IDE

1. **Ouvrez** votre dossier de croquis KissTelegram (avec `.ino`, `.h`, `.cpp`, et `partitions.csv`)

2. Dans Arduino IDE, allez à **Outils** → **Carte** → **4D Systems gen4-ESP32-S3R8n16**

3. **Outils** → **Recharger les données de la carte** (vous verrez une confirmation en bas de l'écran)

4. **Configurez toutes les options du menu Outils:**

   | Paramètre | Valeur |
   |-----------|--------|
   | **Carte** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Activé |
   | **Taille Flash** | 16 MB (128 Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Vitesse de téléchargement** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Activé** ⚠️ |

   ⚠️ **Paramètres critiques** - vérifiez-les deux fois!

5. **Outils** → **Serial Monitor** → Définissez la vitesse en bauds à **115200**

---

## Premier téléchargement (Problèmes courants)

### Pourquoi deux téléchargements sont nécessaires

**Le problème:**
- Premier téléchargement: Arduino utilise la **vieille table de partitions** pour écrire le micrologiciel
- ESP32 démarre: Trouve la **nouvelle table de partitions** (depuis `partitions.csv`)
- **Désaccord** entre l'endroit où le micrologiciel a été écrit et où ESP32 le cherche
- Résultat: Erreurs de démarrage, erreurs de partition, plantages

**La solution:**
Deux téléchargements garantissent que le micrologiciel est écrit au **bon emplacement** défini par la nouvelle table de partitions.

---

### Téléchargement #1: Flash initial (Attendez-vous à des erreurs!)

1. **Connectez le port USB-C DROIT** (près du LED d'alimentation) à votre PC

2. **Sélectionnez le port**: Outils → Port → Sélectionnez le port COM qui apparaît

3. **Vérifiez les paramètres**:
   - ✅ Erase All Flash Before Sketch Upload: **Activé**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

4. **Appuyez sur Télécharger** (`Ctrl+U` ou bouton ➡️)

5. **Attendez ~2-3 minutes** (effacement + téléchargement)

6. **Résultat attendu**:
   ```
   ✅ Upload successful
   ```

7. **Ouvrez Serial Monitor** - vous verrez des erreurs comme:
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **Si vous avez configuré `system_setup.h` correctement**: Vous pouvez recevoir le **premier message Telegram** (mais Serial affichera des erreurs)

**Ne paniquez pas!** Ces erreurs sont **attendues** et **normales**. Continuez vers le téléchargement #2.

---

### Téléchargement #2: Correction de partition (Les erreurs disparaissent)

1. **Déconnectez le port USB-C DROIT**

2. **Connectez le port USB-C GAUCHE** (port OTG) à votre PC

3. **Sélectionnez le nouveau port**: Outils → Port → Sélectionnez le nouveau port COM
   - **Important**: Le numéro de port changera! Regardez dans Serial Monitor pour les données afin de confirmer le port correct.

4. **Vérifiez à nouveau les paramètres**:
   - ✅ Erase All Flash Before Sketch Upload: **Activé**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Appuyez sur Télécharger à nouveau** (`Ctrl+U`)

6. **Attendez ~2-3 minutes** (effacement + téléchargement)

7. **Ouvrez Serial Monitor** - vous devriez maintenant voir:
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi connected
   ✅ Telegram bot enabled
   ✅ System ready
   ```

8. **Vérifiez Telegram** - vous recevrez le message de bienvenue:
   ```
   📦 Hello! KissTelegram is ready.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi Signal: -59 dBm (Good)
   ✅ 0 messages queued
   ```

**Succès!** Votre ESP32-S3 exécute maintenant KissTelegram avec les bonnes partitions.

---

### Téléchargements futurs

**Bonne nouvelle:** Après les deux premiers téléchargements, tous les téléchargements futurs fonctionnent normalement:

- Utilisez le **port USB-C GAUCHE** (OTG)
- **Pas besoin** d'«Erase All Flash» plus (sauf si vous voulez un démarrage propre)
- Téléchargez une fois et ça fonctionne immédiatement

---

## Fichiers de configuration

### system_setup.h (Requis avant le premier téléchargement!)

**Avant de compiler:**

1. Accédez à votre dossier KissTelegram
2. Trouvez `system_setup_template.h`
3. **Renommez**-le en `system_setup.h`
4. **Ouvrez** `system_setup.h` et remplissez:

```cpp
// Your Telegram Bot (from BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Your Chat ID (from @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Your WiFi credentials
#define KISS_FALLBACK_WIFI_SSID "YourWiFiName"
#define KISS_FALLBACK_WIFI_PASSWORD "YourWiFiPassword"

// OTA Security (change default PIN/PUK!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 digits
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 digits
```

5. **Enregistrez** le fichier

**⚠️ Avertissement de sécurité:** Changez le PIN par défaut (`0000`) et PUK (`00000000`) par vos propres secrets!

---

### lang.h (Optionnel: Choisir votre langue)

KissTelegram supporte 7 langues pour les messages système:

```cpp
// In lang.h, uncomment ONE language:

// #define LANG_CN  // 中文 (Chinese)
// #define LANG_DE  // Deutsch (German)
// #define LANG_EN  // English
// #define LANG_FR  // Français (French)
// #define LANG_IT  // Italiano (Italian)
// #define LANG_PT  // Português (Portuguese)
// #define LANG_ES  // Español (Spanish) - DEFAULT if all commented
```

Choisissez votre langue **avant de compiler** pour des messages localisés.

---

## Succès! Et après?

### Vérifier que tout fonctionne

1. **Envoyez `/estado` à votre bot** dans Telegram - vous obtiendrez un rapport d'état détaillé:
   ```
   📦 KissTelegram v1.x.x
   🎯 SYSTEM RELIABILITY
   ✅ System: RELIABLE
   ✅ Messages sent: 2
   💾 Messages pending: 0
   📡 WiFi Signal: -59 dBm (Good)
   🔋 Uptime: 123 seconds
   💾 Free memory: 223 KB
   ```

2. **Vérifiez Serial Monitor** - ne devrait montrer aucune erreur

3. **Testez les commandes**:
   - `/start` - Message de bienvenue
   - `/help` - Commandes disponibles
   - `/estado` - État du système (vérification de santé)

---

### Comprendre les mises à jour OTA

Une fois KissTelegram en cours d'exécution, vous pouvez mettre à jour le micrologiciel **via Telegram** (pas de câble USB!):

1. Envoyez `/ota` à votre bot
2. Entrez PIN: `/otapin 0000` (ou votre PIN personnalisé)
3. **Envoyez votre fichier firmware `.bin`** (glisser-déposer dans Telegram)
4. Le bot vérifie automatiquement la somme de contrôle
5. Confirmez: `/otaconfirm`
6. ESP32 redémarre avec le nouveau micrologiciel
7. **Dans les 60 secondes**, envoyez `/otaok` pour confirmer que ça fonctionne
8. Si vous ne confirmez pas, ESP32 **revient automatiquement** au micrologiciel précédent!

📖 **Lisez plus:** Voir `README_KissOTA_EN.md` (ou votre langue) pour la documentation OTA complète.

---

### Explorez le code d'exemple

L'exemple `suite_kiss.ino` démontre:

- ✅ Gestion WiFi avec surveillance de qualité
- ✅ File d'attente de messages avec priorités
- ✅ Modes de gestion de l'alimentation
- ✅ Gestion des commandes (`/start`, `/help`, `/estado`, etc.)
- ✅ Mises à jour OTA via Telegram
- ✅ Récupération des plantages et persistance
- ✅ Connexions SSL/TLS sécurisées

**Conseil pro:** Utilisez la commande `/estado` comme votre **outil de surveillance de santé** - c'est votre fenêtre sur les internals de KissTelegram!

---

### Dépannage courant

**Problème: "Port not found" ou "Access denied"**
- Windows a bloqué le port. Débranchez l'USB, attendez 5s, rebranchez.
- Essayez un autre câble USB (certains sont charge uniquement, pas de données)

**Problème: "Timeout waiting for device" lors du téléchargement**
- Mauvais port USB! Souvenez-vous: port DROIT pour le premier téléchargement, port GAUCHE pour le second
- Maintenez le bouton BOOT sur ESP32 en cliquant sur Télécharger, relâchez après l'apparition de "Connexion..."

**Problème: Serial Monitor affiche des caractères de charabia**
- Mauvaise vitesse en bauds. Définissez sur **115200** dans la liste déroulante Serial Monitor

**Problème: Le bot ne répond pas dans Telegram**
- Vérifiez que `system_setup.h` a le Bot Token et l'ID de conversation corrects
- Vérifiez que les identifiants WiFi sont corrects
- Ouvrez Serial Monitor et recherchez les messages de connexion WiFi

**Problème: Erreur de compilation "Partition table does not fit"**
- Vous n'avez pas ajouté la partition personnalisée à `boards.txt` correctement
- Ou n'avez pas sélectionné "Custom (4MB APP/12MB LtlFS)" dans Outils → Partition Scheme

---

### Obtenir plus d'aide

- 📧 **Email**: victek@gmail.com
- 📖 **Documentation**: Voir tous les fichiers `README_*.md` dans votre dossier KissTelegram
- 🐛 **Rapports de bugs**: Problèmes GitHub (lien dans le README.md principal)
- 💡 **Demandes de fonctionnalités**: Également bienvenues via email ou GitHub!

---

## Résumé: Le processus complet

```
1. Obtenir Bot Token + ID de conversation depuis Telegram ✅
2. Modifier boards.txt (ajouter partition personnalisée) ✅
3. Configurer Arduino IDE (partition personnalisée, Erase activé) ✅
4. Éditer system_setup.h (identifiants) ✅
5. Connecter port USB DROIT ✅
6. Téléchargement #1 (attendez-vous à des erreurs) ✅
7. Déconnecter DROIT, connecter port USB GAUCHE ✅
8. Téléchargement #2 (erreurs disparues!) ✅
9. Recevoir message de bienvenue dans Telegram ✅
10. Envoyer /estado pour vérifier que tout fonctionne ✅
```

**Vous êtes prêt à construire des projets incroyables avec KissTelegram!** 🎉

---

**Code heureux!**

*Vicente Soriano - victek@gmail.com*
