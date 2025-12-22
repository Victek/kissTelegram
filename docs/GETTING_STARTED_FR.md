# Premiers Pas avec KissTelegram sur ESP32-S3

**Guide complet pour configurer votre ESP32-S3 de zéro jusqu'à votre premier message Telegram**

> ⚠️ **CRITIQUE**: Lisez ce guide complètement avant de télécharger un firmware. L'ESP32-S3 N16R8 nécessite un **processus de téléchargement en deux étapes** en raison des partitions personnalisées. Sauter des étapes causera des erreurs!

---

## Table des Matières

1. [Avant de Commencer](#avant-de-commencer)
2. [Créer Votre Bot Telegram](#créer-votre-bot-telegram)
3. [Configuration Matérielle](#configuration-matérielle)
4. [Configuration de l'IDE Arduino](#configuration-de-lide-arduino)
5. [Premier Téléchargement (Créer partitions avec Arduino IDE)](#premier-téléchargement)
6. [Fichiers de Configuration](#fichiers-de-configuration)
7. [Succès! Et Maintenant?](#succès-et-maintenant)

---

## Avant de Commencer

### Ce Dont Vous Avez Besoin

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Deux câbles USB-C** (pour alterner entre les ports bootloader et OTG)
- **Arduino IDE 2.x** ou supérieur
- **PC Windows** (ce guide est axé sur Windows, adaptez les chemins pour Linux/Mac)
- **Compte Telegram** sur votre téléphone

### Ce Qui Rend Ceci Différent

Votre nouvel ESP32-S3 N16R8 arrive avec une application de démonstration LED RGB intégrée. KissTelegram **remplace complètement la table de partition** pour maximiser votre flash de 16MB:

| Partition | Espressif par Défaut | KissTelegram Personnalisé |
|-----------|---------------------|---------------------------|
| Espace App | 1.5 MB | 4.5 MB (3x plus grand!) |
| Système de Fichiers | 5 MB | 13 MB (2.6x plus grand!) |
| Total Utilisé | 6.5 MB | 17.5 MB |

C'est pourquoi le processus en deux téléchargements est requis: **la table de partition change entre les téléchargements**.

---

## Créer Votre Bot Telegram

### Étape 1: Parler à BotFather

1. Ouvrez Telegram sur votre téléphone
2. Recherchez `@BotFather` (bot officiel, a la coche bleue)
3. Démarrez la conversation avec `/start`
4. Créez votre bot avec `/newbot`
5. Choisissez un nom (exemple: "Mon Assistant Maison")
6. Choisissez un nom d'utilisateur (doit se terminer par `bot`, exemple: "monassistant_maison_bot")
7. **Sauvegardez votre Token de Bot** - ressemble à: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Étape 2: Obtenez Votre ID de Chat

**Méthode 1: Utilisation d'un Bot (Plus Facile)**

1. Recherchez `@ChatIDHelperBot` dans Telegram
2. Démarrez la conversation avec `/start`
3. Il répondra avec votre **ID de Chat** (un numéro comme `123456789`)
4. **Sauvegardez ce numéro** - vous en aurez besoin dans la configuration

**Méthode 2: Utilisation d'un Navigateur Web**

1. Envoyez n'importe quel message à votre bot nouvellement créé
2. Ouvrez le navigateur et visitez:
   ```
   https://api.telegram.org/bot<VOTRE_TOKEN_BOT>/getUpdates
   ```
   (Remplacez `<VOTRE_TOKEN_BOT>` avec votre token actuel)
3. Recherchez `"chat":{"id":123456789` dans la réponse JSON
4. Ce numéro est votre **ID de Chat**

**✅ Vous avez maintenant:**
- Token de Bot: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- ID de Chat: `123456789`

Gardez-les en sécurité! Vous en aurez besoin bientôt.

---

## Configuration Matérielle

### Comprendre les Deux Ports USB-C

Votre ESP32-S3 N16R8 a **deux ports USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED Power    │
│  └─┘                 │
│  [USB-C]  ← PORT DROIT (Bootloader/Téléchargement)
│                      │
│                      │
│  [USB-C]  ← PORT GAUCHE (OTG/Fonctionnement Normal)
│                      │
└─────────────────────┘
```

**PORT DROIT (près de la LED Power):**
- Utilisé pour **téléchargement initial du firmware**
- Utilisé pour **mode bootloader**
- Utilisez celui-ci quand Arduino IDE dit "Connexion..."

**PORT GAUCHE (OTG):**
- Utilisé pour **fonctionnement normal** après le premier téléchargement
- Utilisé pour **deuxième téléchargement** (correction de partition)
- Utilisez celui-ci pour le Moniteur Série en fonctionnement normal

---

## Configuration de l'IDE Arduino

### Étape 1: Afficher les Fichiers Cachés (Windows)

1. Ouvrez l'**Explorateur de Fichiers**
2. Cliquez sur l'onglet **Affichage** → **Afficher** → Cochez:
   - ✅ Extensions de noms de fichiers
   - ✅ Éléments masqués
3. Dans l'onglet **Filtre**: **Tous les types de fichiers**

### Étape 2: Modifier boards.txt

1. Naviguez vers:
   ```
   C:\Users\<VOTRE_NOM_UTILISATEUR>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Remplacez `3.3.4` avec votre version du noyau ESP32 si différente)

2. Trouvez et ouvrez `boards.txt` (utilisez Notepad++ ou n'importe quel éditeur de texte)

3. Appuyez sur `Ctrl+F` et recherchez:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Immédiatement en dessous de cette ligne**, collez ces trois lignes:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Sauvegardez** et fermez `boards.txt`

6. Si Arduino IDE était ouvert, **fermez et redémarrez-le**

### Étape 3: Configurer Arduino IDE

1. **Ouvrez** votre dossier de sketch KissTelegram (avec `.ino`, `.h`, `.cpp`, et `partitions.csv`)

2. Dans Arduino IDE, allez à **Outils** → **Type de carte** → **4D Systems gen4-ESP32-S3R8n16**

3. **Configurez toutes les options du menu Outils:**

   | Paramètre | Valeur |
   |---------|-------|
   | **Type de carte** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Activé |
   | **Taille Flash** | 16MB (128Mb) |
   | **Schéma de Partition** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Vitesse de Téléchargement** | 921600 |
   | **Effacer Tout le Flash Avant le Téléchargement du Sketch** | **Activé** ⚠️ |

   ⚠️ **Paramètres critiques** - vérifiez deux fois!

4. **Outils** → **Moniteur Série** → Définir la vitesse à **115200**

---

## Premier Téléchargement (Problèmes Communs)

### Pourquoi Deux Téléchargements Sont Nécessaires

**Le Problème:**
- Premier téléchargement: Arduino utilise l'**ancienne table de partition** pour écrire le firmware
- ESP32 démarre: Trouve la **nouvelle table de partition** (de `partitions.csv`)
- **Discordance** entre où le firmware a été écrit vs où ESP32 le cherche
- Résultat: Erreurs de démarrage, erreurs de partition, plantages

**La Solution:**
Deux téléchargements garantissent que le firmware est écrit au **bon emplacement** défini par la nouvelle table de partition.

---

### Téléchargement #1: Flash Initial

1. **Connectez le port USB-C DROIT** (près de la LED Power) à votre PC

2. **Sélectionnez le port**: Outils → Port → Sélectionnez le port COM qui apparaît

3. **Vérifiez les paramètres**:
   - ✅ Effacer Tout le Flash Avant le Téléchargement du Sketch: **Activé**
   - ✅ Schéma de Partition: **Custom (4MB APP/12MB LtlFS)**
   

4. **Outils, Charger** ou (`Ctrl+U`) (Cliquez sur l'option que vous préférez)
   - ✅ Le firmware est téléchargé.
   - Prend 53.6 secondes ou beaucoup moins si vous utilisez une alimentation externe pour l'ESP32s3 

Continuez avec Téléchargement #2.

---

### Téléchargement #2: Téléchargement du Sketch

1. **Déconnectez le port USB-C DROIT**

2. **Connectez le port USB-C GAUCHE** (port OTG) à votre PC

3. **Sélectionnez le nouveau port**: Outils → Port → Sélectionnez le nouveau port COM
   - **Important**: Le numéro de port changera! Recherchez des données dans le Moniteur Série pour confirmer le bon port, par exemple, appuyez sur reset ESP32s3 jusqu'à ce que vous voyiez une réponse de données

4. **Vérifiez à nouveau les paramètres**:
   - ✅ Effacer Tout le Flash Avant le Téléchargement du Sketch: **Activé**
   - ✅ Schéma de Partition: **Custom (4MB APP/12MB LtlFS)**

5. **Appuyez à nouveau sur Téléverser** (`Ctrl+U`)

6. **Attendez ~2-3 minutes** (effacement + téléchargement, dépend si vous utilisez une alimentation externe)

7. **Ouvrez le Moniteur Série** - vous devriez maintenant voir (si vous avez correctement configuré les identifiants 
dans system_setup.h (le system_setup_template renommé)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi connecté
   ✅ Bot Telegram activé
   ✅ Système prêt
   ```

8. **Vérifiez Telegram** - vous recevrez un message de bienvenue:
   ```
   📦 Bonjour! KissTelegram est prêt.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Signal WiFi: -59 dBm (Bon)
   ✅ 0 messages en file d'attente
   ```

**Succès!** Votre ESP32-S3 exécute maintenant KissTelegram avec les bonnes partitions.

---

### Téléchargements Futurs

**Bonnes nouvelles:** Après les deux téléchargements initiaux, tous les téléchargements futurs fonctionnent normalement:

- Utilisez le **port USB-C GAUCHE** (OTG)
- **Pas besoin** de "Effacer Tout le Flash" plus (sauf si vous avez fait des modifications aux données NVRAM)
- Téléchargez une fois et ça fonctionne immédiatement

---

## Fichiers de Configuration

### system_setup.h (Requis Avant le Premier Téléchargement!)

**Avant de compiler:**

1. Naviguez vers votre dossier KissTelegram
2. Trouvez `system_setup_template.h`
3. **Renommez-le** en `system_setup.h`
4. **Ouvrez** `system_setup.h` et remplissez:

```cpp
// Votre Bot Telegram (de BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Votre ID de Chat (de @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Vos identifiants WiFi
#define KISS_FALLBACK_WIFI_SSID "NomDeVotreWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "MotDePasseDeVotreWiFi"

// Sécurité OTA (changez le PIN/PUK par défaut!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 chiffres
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 chiffres
```

5. **Sauvegardez** le fichier

**⚠️ Avertissement de Sécurité:** Changez le PIN par défaut (`0000`) et le PUK (`00000000`) pour vos propres secrets!

---

### lang.h (Optionnel: Choisissez Votre Langue)

KissTelegram supporte 7 langues pour les messages système:

```cpp
// Dans lang.h, décommentez UNE langue:

// #define LANG_CN  // 中文 (Chinois)
// #define LANG_DE  // Deutsch (Allemand)
// #define LANG_EN  // English (Anglais)
// #define LANG_FR  // Français (Français)
// #define LANG_IT  // Italiano (Italien)
// #define LANG_PT  // Português (Portugais)
// #define LANG_ES  // Español (Espagnol) - PAR DÉFAUT si tout est commenté
```

Choisissez votre langue (décommentez) **avant de compiler** pour des messages localisés.

---

## Succès! Et Maintenant?

### Vérifier Que Tout Fonctionne

1. **Envoyez `/status` à votre bot** dans Telegram - vous obtiendrez un rapport de statut détaillé:
   ```
   📦 KissTelegram v1.x.x
   🎯 FIABILITÉ DU SYSTÈME
   ✅ Système: FIABLE
   ✅ Messages envoyés: 2
   💾 Messages en attente: 0
   📡 Signal WiFi: -59 dBm (Bon)
   🔋 Temps de fonctionnement: 123 secondes
   💾 Mémoire libre: 223 KB
   ```

2. **Vérifiez le Moniteur Série** - ne devrait montrer aucune erreur

3. **Testez les commandes**:
   - `/start` - Message de bienvenue
   - `/help` - Commandes disponibles
   - `/status` - Statut du système (vérification de santé)

---

### Comprendre les Mises à Jour OTA

Une fois que KissTelegram fonctionne, vous pouvez mettre à jour le firmware **via Telegram** (pas de câble USB!):

1. Envoyez `/ota` à votre bot
2. Entrez le PIN: `/otapin 0000` (ou votre PIN personnalisé)
3. **Envoyez votre fichier firmware `.bin`** (glisser-déposer dans Telegram)
4. Le bot vérifie automatiquement la somme de contrôle
5. Confirmez: `/otaconfirm`
6. ESP32 redémarre avec le nouveau firmware
7. **Dans les 60 secondes**, envoyez `/otaok` pour confirmer que ça fonctionne
8. Si vous ne confirmez pas, ESP32 **revient automatiquement** au firmware précédent!

📖 **En savoir plus:** Voir `README_KissOTA_FR.md` pour la documentation OTA complète.

---

### Explorer le Code d'Exemple

L'exemple `suite_kiss.ino` démontre:

- ✅ Gestion WiFi avec surveillance de la qualité
- ✅ File d'attente de messages avec priorités
- ✅ Modes de gestion de l'énergie
- ✅ Gestion des commandes (`/start`, `/help`, `/status`, etc.)
- ✅ Mises à jour OTA via Telegram
- ✅ Récupération après plantage et persistance
- ✅ Connexions SSL/TLS sécurisées

**Conseil de pro:** Utilisez la commande `/status` comme votre **outil de surveillance de santé** - c'est votre fenêtre sur les internes de KissTelegram!

---

### Dépannage Courant

**Problème: "Port non trouvé" ou "Accès refusé"**
- Windows a verrouillé le port. Déconnectez USB, attendez 5s, reconnectez.
- Essayez un câble USB différent (certains sont seulement pour la charge, pas pour les données)

**Problème: "Délai d'attente pour le périphérique" pendant le téléchargement**
- Mauvais port USB! Rappelez-vous: port DROIT pour premier téléchargement, port GAUCHE pour deuxième
- Maintenez le bouton BOOT sur ESP32 pendant que vous cliquez sur Téléverser, relâchez après l'apparition de "Connexion..."

**Problème: Le Moniteur Série affiche des caractères aléatoires**
- Mauvais débit en bauds. Définir à **115200** dans le menu déroulant du Moniteur Série

**Problème: Le bot ne répond pas dans Telegram**
- Vérifiez que `system_setup.h` a le bon Token de Bot et ID de Chat
- Vérifiez que les identifiants WiFi sont corrects
- Ouvrez le Moniteur Série et recherchez les messages de connexion WiFi

**Problème: Erreur de compilation "La table de partition ne rentre pas"**
- N'a pas ajouté la partition personnalisée à `boards.txt` correctement
- Ou n'a pas sélectionné "Custom (4MB APP/12MB LtlFS)" dans Outils → Schéma de Partition

---

### Obtenir Plus d'Aide

- 📧 **Email**: victek@gmail.com
- 📖 **Documentation**: Voir tous les fichiers `README_*.md` dans votre dossier KissTelegram
- 🐛 **Rapports de Bugs**: Issues GitHub (lien dans README.md principal)
- 💡 **Demandes de Fonctionnalités**: Également bienvenues par email ou GitHub!

---

## Résumé: Le Processus Complet

```
1. Obtenir Token de Bot + ID de Chat de Telegram ✅
2. Modifier boards.txt (ajouter partition personnalisée) ✅
3. Configurer Arduino IDE (Partition personnalisée, Effacement activé) ✅
4. Modifier system_setup.h (identifiants) ✅
5. Connecter port USB DROIT ✅
6. Téléchargement #1 (nouvelles partitions)✅
7. Déconnecter DROIT, connecter port USB GAUCHE ✅
8. Téléchargement #2 (Télécharger Sketch KissTelegram) ✅
9. Recevoir message de bienvenue dans Telegram ✅
10. Envoyer /status pour vérifier que tout fonctionne ✅
```

**Vous êtes prêt à construire des projets incroyables avec KissTelegram!** 🎉
