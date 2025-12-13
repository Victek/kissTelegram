# KissOTA - Système de Mise à Jour OTA pour ESP32

## Table des Matières
- [Description Générale](#description-générale)
- [Caractéristiques Principales](#caractéristiques-principales)
- [Comparatif avec d'Autres Systèmes OTA](#comparatif-avec-dautres-systèmes-ota)
- [Architecture du Système](#architecture-du-système)
- [Avantages par Rapport à l'OTA Standard d'Espressif](#avantages-par-rapport-à-lota-standard-despressif)
- [Exigences Matérielles](#exigences-matérielles)
- [Configuration des Partitions](#configuration-des-partitions)
- [Flux du Processus OTA](#flux-du-processus-ota)
- [Commandes Disponibles](#commandes-disponibles)
- [Caractéristiques de Sécurité](#caractéristiques-de-sécurité)
- [Gestion des Erreurs et Récupération](#gestion-des-erreurs-et-récupération)
- [Utilisation de Base](#utilisation-de-base)
- [Questions Fréquentes (FAQ)](#questions-fréquentes-faq)

---

## Description Générale

KissOTA est un système avancé de mise à jour Over-The-Air (OTA) conçu spécifiquement pour les dispositifs ESP32-S3 avec PSRAM. Contrairement au système OTA standard d'Espressif, KissOTA fournit plusieurs couches de sécurité, validation et récupération automatique, le tout intégré avec Telegram pour des mises à jour à distance sécurisées.

**Auteur:** Vicente Soriano (victek@gmail.com)
**Version actuelle:** 0.9.0
**Licence:** Incluse dans KissTelegram Suite

---

## Caractéristiques Principales

### 🔒 Sécurité Robuste
- **Authentification PIN/PUK**: Contrôle d'accès par codes PIN et PUK
- **Verrouillage automatique**: Après 3 tentatives échouées de PIN
- **Vérification CRC32**: Validation d'intégrité du firmware avant le flash
- **Validation post-flash**: Nécessite confirmation manuelle de l'utilisateur

### 🛡️ Protection contre les Défaillances
- **Backup automatique**: Copie du firmware actuel avant mise à jour
- **Rollback automatique**: Restauration si le nouveau firmware échoue
- **Détection de boot loops**: Si échec 3 fois en 5 minutes, rollback automatique
- **Partition factory**: Filet de sécurité final si tout le reste échoue
- **Timeout global**: 7 minutes pour compléter tout le processus

### 💾 Utilisation Efficace des Ressources
- **Buffer PSRAM**: Utilise 7-8MB de PSRAM pour télécharger sans toucher le flash
- **Économie d'espace**: Nécessite seulement partitions factory + app (sans OTA_0/OTA_1)
- **Nettoyage automatique**: Supprime les fichiers temporaires et backups anciens

### 📱 Intégration avec Telegram
- **Mise à jour à distance**: Envoyez le .bin directement par Telegram
- **Feedback en temps réel**: Messages de progression et d'état
- **Multi-langue**: Support de 7 langues (ES, EN, FR, IT, DE, PT, CN)

---

## Comparatif avec d'Autres Systèmes OTA

### KissOTA vs Autres Solutions Populaires

| Caractéristique | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **Transport** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Accès distant** | ✅ Global sans config | ❌ LAN uniquement* | ❌ LAN uniquement* | ❌ LAN uniquement* | ❌ LAN uniquement* |
| **Nécessite IP connue** | ❌ Non | ✅ Oui | ✅ Oui | ✅ Oui | ✅ Oui |
| **Port forwarding** | ❌ Non nécessaire | ⚠️ Pour accès distant | ⚠️ Pour accès distant | ⚠️ Pour accès distant | ⚠️ Pour accès distant |
| **Serveur web** | ❌ Non | ✅ AsyncWebServer | ✅ WebServer | ✅ Configurable | ✅ WebServer |
| **Authentification** | 🔒 PIN/PUK (robuste) | ⚠️ Utilisateur/mdp basique | ⚠️ Mot de passe optionnel | ⚠️ Basique | ⚠️ Utilisateur/mdp basique |
| **Backup firmware** | ✅ Dans LittleFS | ❌ Non | ❌ Non | ❌ Non | ❌ Non |
| **Rollback automatique** | ✅✅✅ 3 niveaux | ⚠️ Limité (2 part.) | ❌ Non | ⚠️ Limité (2 part.) | ⚠️ Limité (2 part.) |
| **Validation manuelle** | ✅ 60s avec /otaok | ❌ Non | ❌ Non | ⚠️ Optionnel | ❌ Non |
| **Boot loop detection** | ✅ Automatique | ❌ Non | ❌ Non | ❌ Non | ❌ Non |
| **Buffer de téléchargement** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Progression en temps réel** | ✅ Messages Telegram | ✅ UI Web | ⚠️ Série uniquement | ⚠️ Configurable | ✅ UI Web |
| **Interface utilisateur** | 📱 Chat Telegram | 🖥️ Navigateur web | 🖥️ Navigateur web | ⚡ Programmatique | 🖥️ Navigateur web |
| **Dépendances** | KissTelegram | ESPAsyncWebServer | ESP mDNS | Aucune (natif) | WebServer |
| **Flash requis** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Sécurité sur internet** | ✅ Élevée (API Telegram) | ⚠️ Vulnérable si exposé | ⚠️ Vulnérable si exposé | ⚠️ Vulnérable si exposé | ⚠️ Vulnérable si exposé |
| **Facile à utiliser** | ✅✅ Juste envoyer .bin | ✅ UI web intuitive | ⚠️ Nécessite configuration | ⚠️ Complexité élevée | ✅ UI web intuitive |
| **Multi-langue** | ✅ 7 langues | ❌ Anglais uniquement | ❌ Anglais uniquement | ❌ Anglais uniquement | ❌ Anglais uniquement |
| **Récupération factory** | ✅ Oui | ❌ Non | ❌ Non | ⚠️ Manuelle | ❌ Non |

\* *Avec port forwarding/VPN l'accès distant est possible, mais nécessite configuration réseau avancée*

### Avantages Uniques de KissOTA

#### 🌍 **Accès Véritablement Global**
Les autres systèmes OTA nécessitent:
- Connaître l'IP du dispositif
- Être sur le même réseau LAN, ou
- Configurer le port forwarding (risqué), ou
- Configurer un VPN (complexe)

**KissOTA:** Vous avez seulement besoin de Telegram. Mettez à jour depuis n'importe où dans le monde sans configuration réseau. Comme il est intégré dans KissTelegram, il utilise une connexion SSL.

#### 🔒 **Sécurité sans Compromis**
Exposer un serveur web HTTP à internet est dangereux:
- Vulnérable aux attaques par force brute
- Vecteur potentiel d'exploits web
- Nécessite HTTPS pour la sécurité (certificats, etc.)

**KissOTA:** Utilise l'infrastructure sécurisée de Telegram. Votre ESP32 n'expose jamais de ports vers l'extérieur.

#### 🛡️ **Récupération Multiniveau**
Les autres systèmes avec rollback (ESP-IDF, AsyncElegantOTA) n'ont que 2 partitions:
- Si les deux partitions échouent → dispositif "briqué"
- Sans backup du firmware fonctionnel

**KissOTA:** 3 niveaux de sécurité:
1. **Niveau 1:** Rollback depuis backup dans LittleFS
2. **Niveau 2:** Boot depuis partition factory originale
3. **Niveau 3:** Détection de boot loops automatique

#### 💾 **Économie d'Espace Flash**
Systèmes traditionnels (dual-bank OTA):
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 7 MB minimum requis
OTA_1:    3.5 MB  ┘
Total: 10.5 MB
```

**KissOTA (single-bank + backup):**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 7 MB total
Backup:   ~1.1 MB │  (dans LittleFS, compressible)
Total: ~8.1 MB   ┘
```
**Gain:** ~2.4 MB de flash libre pour votre application ou données.

#### 🚀 **PSRAM comme Buffer**
Les autres systèmes téléchargent directement vers le flash:
- **Usure du flash:** Le flash a des cycles d'écriture limités (~100K)
- **Risque:** Si le téléchargement échoue, le flash a déjà été partiellement écrit

**KissOTA:**
- Téléchargement complet vers PSRAM d'abord
- Vérifie CRC32 en PSRAM
- N'écrit vers le flash que si tout est OK
- **PSRAM sans usure:** Cycles d'écriture infinis

---

## Architecture du Système

### Structure des Partitions

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Firmware original d'usine
│  - Filet de sécurité final         │
│  - Lecture seule en production     │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Firmware en exécution
│  - Firmware actif actuel            │
│  - Mis à jour pendant OTA           │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (restant ~8-9 MB)        │
│  - /ota_backup.bin (backup)         │ ← Backup du firmware précédent
│  - Fichiers de configuration        │
│  - Données persistantes             │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot flags (validation)          │ ← État de validation OTA
│  - Identifiants PIN/PUK             │
│  - Compteur de démarrages           │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Buffer de téléchargement temporaire│ ← Firmware téléchargé avant flash
│  - Non persistant (effacé)          │
└─────────────────────────────────────┘
```

### Flux de Données Pendant OTA

```
API Telegram
    │
    ▼
[Téléchargement vers PSRAM] ← Buffer temporaire 7-8 MB
    │
    ▼
[Vérification CRC32]
    │
    ▼
[Backup firmware actuel → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash nouveau firmware → App Partition]
    │
    ▼
[Redémarrage de l'ESP32]
    │
    ▼
[Validation 60 secondes] → /otaok ou rollback automatique
    │
    ▼
[Suppression firmware précédent] -> Récupère l'espace du firmware précédent

```
---

## Avantages par Rapport à l'OTA Standard d'Espressif

| Caractéristique | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **Espace Flash requis** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (app seule) |
| **Backup du firmware** | ❌ Non | ✅ Oui, dans LittleFS |
| **Rollback automatique** | ⚠️ Limité | ✅ Complet + factory |
| **Validation manuelle** | ❌ Non | ✅ Oui, 60 secondes |
| **Authentification** | ❌ Non | ✅ PIN/PUK |
| **Détection boot loops** | ❌ Non | ✅ Oui, automatique |
| **Intégration Telegram** | ❌ Non | ✅ Native |
| **Buffer de téléchargement** | Flash | PSRAM (pas d'usure du flash) |
| **Récupération d'urgence** | ⚠️ 2 partitions seulement | ✅ 3 niveaux (app/backup/factory) |

---

## Exigences Matérielles

### Minimum Requis
- **MCU**: ESP32-S3 (d'autres variantes ESP32 peuvent fonctionner avec adaptations)
- **PSRAM**: Minimum dépend de la taille du firmware à remplacer 2-8MB PSRAM (pour buffer de téléchargement)
- **Flash**: Minimum dépend de la taille du firmware à remplacer 8-16-32MB
- **Connectivité**: WiFi fonctionnel

### Configuration Recommandée
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (idéal)
- **ESP32-S3-DevKitC-1**: Avec module N16R8 ou supérieur

---

## Configuration des Partitions

Fichier `partitions.csv` recommandé (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Configuration dans Arduino IDE:**
- Tools → Partition Scheme → Custom
- Pointer vers le fichier `partitions.csv`

---

## Flux du Processus OTA

### Diagramme d'États

```
┌─────────────┐
│  OTA_IDLE   │ ← État initial
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Demande PIN (3 tentatives)
└──────┬──────┘
       │ PIN correct
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, prêt à recevoir .bin
└──────┬───────┘
       │ L'utilisateur envoie .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Téléchargement vers PSRAM (progression en temps réel)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Calcule CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Attend /otaconfirm de l'utilisateur
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Sauvegarde firmware actuel dans LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Écrit nouveau firmware depuis PSRAM
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← Redémarre ESP32
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60 secondes pour /otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← Firmware validé ✅
└──────────────┘
       │ Timeout ou /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Restaure backup automatiquement
└──────────────┘
```

### Timeouts Importants

- **Authentification PIN**: Illimité (jusqu'à 3 tentatives), si échec, PIN bloqué et nécessite PUK pour le restaurer
- **Réception du .bin**: Attend jusqu'à 7 minutes, si dépassé annule /ota
- **Confirmation**: Attend jusqu'à 7 minutes, si dépassé annule /ota (attend /otaconfirm)
- **Processus complet**: 7 minutes maximum depuis /otaconfirm
- **Validation post-flash**: 60 secondes pour /otaok

---

## Commandes Disponibles

### `/ota`
Démarre le processus OTA.

**Utilisation:**
```
/ota
```

**Réponse:**
- Si PIN non bloqué: Demande PIN
- Si PIN bloqué: Demande PUK

---

### `/otapin <code>`
Envoie le code PIN (4-8 chiffres).

**Utilisation:**
```
/otapin 0000 (par défaut)
```

**Réponse:**
- ✅ PIN correct: État AUTHENTICATED, prêt à recevoir .bin
- ❌ PIN incorrect: Réduit tentatives restantes
- 🔒 Après 3 échecs: Bloque et demande PUK

---

### `/otapuk <code>`
Déverrouille le système avec le code PUK.

**Utilisation:**
```
/otapuk 12345678
```

**Réponse:**
- ✅ PUK correct: Déverrouille PIN et demande nouveau PIN
- ❌ PUK incorrect: Reste bloqué

---

### `/otaconfirm`
Confirme que vous souhaitez flasher le firmware téléchargé.

**Utilisation:**
```
/otaconfirm
```

**Préconditions:**
- Firmware téléchargé et vérifié
- Checksum CRC32 OK

**Action:**
- Crée backup du firmware actuel
- Flashe nouveau firmware
- Redémarre l'ESP32

---

### `/otaok`
Valide que le nouveau firmware fonctionne correctement.

**Utilisation:**
```
/otaok
```

**Préconditions:**
- Doit être envoyé dans les 60 secondes après le redémarrage
- Disponible uniquement après un flash OTA

**Action:**
- Marque le firmware comme valide
- Supprime le backup précédent
- Le système retourne à l'opération normale

⚠️ **IMPORTANT**: Si `/otaok` n'est pas envoyé dans les 60 secondes, un rollback automatique sera exécuté.

---

### `/otacancel`
Annule le processus OTA ou force le rollback.

**Utilisation:**
```
/otacancel
```

**Comportement:**
- Pendant téléchargement/validation: Annule et nettoie les fichiers temporaires
- Pendant validation post-flash: Exécute rollback immédiat
- Si aucun OTA actif: Informe qu'il n'y a pas de processus actif

---

## Caractéristiques de Sécurité

### 1. Authentification PIN/PUK

#### PIN et PUK (Peuvent être changés à distance avec /changepin <ancien> <nouveau> ou /changepuk <ancien> <nouveau>)

#### PIN (Personal Identification Number)
- **Longueur**: 4-8 chiffres
- **Tentatives**: 3 tentatives avant blocage
- **Configuration**: Défini dans `system_setup.h` Identifiants Fallback
- **Persistance**: Sauvegardé en sécurité dans NVS

#### PUK (PIN Unlock Key)
- **Longueur**: 8 chiffres
- **Fonction**: Débloquer après 3 échecs de PIN
- **Sécurité**: Seul l'administrateur devrait le connaître

**Exemple de flux de blocage:**
```
Tentative 1: PIN incorrect → "❌ PIN incorrect. 2 tentatives restantes"
Tentative 2: PIN incorrect → "❌ PIN incorrect. 1 tentative restante"
Tentative 3: PIN incorrect → "🔒 PIN bloqué. Utilisez /otapuk [code]"
```

### 2. Vérification d'Intégrité

#### CRC32 Checksum
- **Algorithme**: CRC32 IEEE 802.3
- **Calcul**: Sur tout le fichier .bin téléchargé
- **Validation**: Avant de permettre /otaconfirm
- **Rejet**: Si CRC32 ne correspond pas, le téléchargement est supprimé

**Exemple de sortie:**
```
🔍 Vérification du checksum...
🔍 CRC32: 0xF8CAACF6 (1.07 MB vérifiés)
✅ Checksum OK
```

### 3. Validation Post-Flash

#### Fenêtre de Validation (60 secondes)
Après avoir flashé le nouveau firmware:
1. L'ESP32 redémarre
2. Les boot flags marquent `otaInProgress = true`
3. L'utilisateur a 60 secondes pour envoyer `/otaok`
4. S'il ne répond pas → rollback automatique

**Diagramme temporel:**
```
FLASH → REDÉMARRAGE → [60s pour /otaok] → ROLLBACK si timeout
                         ↓
                      /otaok → ✅ Firmware validé
```

### 4. Protection contre Modification Non Autorisée

- **Mode maintenance**: Pendant OTA, les autres commandes sont limitées
- **Chat ID unique**: Seul le chat_id configuré peut exécuter OTA
- **Authentification préalable**: PIN/PUK obligatoires avant toute action

---

## Gestion des Erreurs et Récupération

### Niveaux de Récupération

#### Niveau 1: Nouvelle Tentative Automatique
**Scénarios:**
- Échec de téléchargement depuis Telegram (max 3 nouvelles tentatives)
- Timeout réseau
- Téléchargement interrompu

**Action:**
- Nettoie PSRAM
- Retente téléchargement automatiquement
- Notifie l'utilisateur de la tentative

---

#### Niveau 2: Rollback depuis Backup
**Scénarios:**
- L'utilisateur n'envoie pas `/otaok` dans les 60 secondes
- L'utilisateur envoie `/otacancel` pendant la validation
- Boot loop détecté (3+ démarrages en 5 minutes)

**Processus de Rollback:**
1. Détecte `bootFlags.otaInProgress == true`
2. Lit `bootFlags.backupPath` → `/ota_backup.bin`
3. Restaure backup depuis LittleFS → App Partition
4. Redémarre ESP32
5. Nettoie boot flags
6. Notifie l'utilisateur par Telegram

**Exemple de code:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // Pas de backup
  }

  // Lit /ota_backup.bin depuis LittleFS
  // Écrit dans App Partition
  // Redémarre
}
```

---

#### Niveau 3: Fallback vers Factory
**Scénarios:**
- Rollback depuis backup échoue
- Fichier `/ota_backup.bin` corrompu
- Erreur critique en flash

**Processus:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Redémarre ESP32
3. Démarre depuis firmware original d'usine
4. Notifie erreur critique à l'utilisateur

⚠️ **IMPORTANT**: C'est le dernier recours. Le firmware factory doit être stable et ne jamais être modifié en production.

---

### Détection de Boot Loops

**Algorithme:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minutes
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ démarrages en 5 minutes");
      return true; // Exécuter rollback
    }
  }
  return false;
}
```

**Protection:**
- Incrémente `bootFlags.bootCount` à chaque démarrage
- Si > 3 démarrages en < 5 minutes → Rollback automatique
- À la validation avec `/otaok`, réinitialise le compteur

---

### États d'Erreur

| Erreur | Code | Action Automatique |
|-------|--------|-------------------|
| **Téléchargement échoué** | `DOWNLOAD_FAILED` | Retenter jusqu'à 3 fois |
| **Checksum incorrect** | `CHECKSUM_MISMATCH` | Supprimer téléchargement, annuler OTA |
| **Échec du backup** | `BACKUP_FAILED` | Annuler OTA, ne pas risquer |
| **Échec du flash** | `FLASH_FAILED` | Annuler OTA, maintenir firmware actuel |
| **Timeout validation** | `VALIDATION_TIMEOUT` | Rollback automatique |
| **Boot loop** | `BOOT_LOOP_DETECTED` | Rollback automatique |
| **Rollback échoué** | `ROLLBACK_FAILED` | Fallback vers factory |
| **Sans backup** | `NO_BACKUP` | Maintenir firmware actuel, avertir |

---

## Utilisation de Base

### Mise à Jour Complète Étape par Étape

#### 1. Préparer le Firmware
```bash
# Compiler le projet dans Arduino IDE
# Le .bin est généré dans: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. Démarrer OTA
Depuis Telegram:
```
/ota
```

Réponse du bot:
```
🔐 AUTHENTIFICATION OTA

Entrez le code PIN de 4-8 chiffres:
/otapin [code]

Tentatives restantes: 3
```

#### 3. S'Authentifier avec PIN
```
/otapin 0000
```

Réponse:
```
✅ PIN correct

Système prêt pour OTA.
Envoyez le fichier .bin du nouveau firmware.
```

#### 4. Envoyer le Firmware
- Joignez le fichier `.bin` comme document (pas comme photo)
- Telegram le téléverse et le bot le télécharge automatiquement

Réponse pendant le téléchargement:
```
📥 Téléchargement du firmware vers PSRAM...
⏳ Progression: 45%
```

#### 5. Vérification Automatique
```
✅ Téléchargement complet: 1.07 MB en PSRAM
🔍 Vérification du checksum...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VÉRIFIÉ

Fichier: suite_kiss.ino.bin
Taille: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ CONFIRMATION REQUISE
Pour flasher le firmware:
/otaconfirm

Pour annuler:
/otacancel
```

#### 6. Confirmer Flash
```
/otaconfirm
```

Réponse:
```
💾 Démarrage du backup...
✅ Backup complet: 1123456 octets

⚡ Flash du firmware...
✅ FLASH TERMINÉ

Firmware écrit correctement.
Le dispositif va redémarrer maintenant.

Après redémarrage vous aurez 60 secondes pour valider avec /otaok
Si vous ne validez pas, un rollback automatique sera exécuté.
```

#### 7. Validation Post-Redémarrage
Après le redémarrage (dans les 60 secondes):
```
/otaok
```

Réponse:
```
✅ FIRMWARE VALIDÉ

Le nouveau firmware a été confirmé.
Le système retourne à l'opération normale.

Version: 0.9.0
```

---

### Exemple de Rollback Manuel

Si le nouveau firmware ne fonctionne pas bien:

```
/otacancel
```

Réponse:
```
⚠️ EXÉCUTION DU ROLLBACK

Restauration du firmware précédent depuis le backup...
✅ Firmware précédent restauré
🔄 Redémarrage...

[Après redémarrage]
✅ ROLLBACK TERMINÉ

Système restauré au firmware précédent.
```

---

## Configuration Avancée

### Personnaliser les Timeouts

Dans `KissOTA.h`:

```cpp
// Timeout de validation (par défaut 60 secondes)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Timeout global du processus OTA (par défaut 7 minutes)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### Activer/Désactiver WDT Pendant OTA

Dans `system_setup.h`:

```cpp
// Désactiver WDT pendant opérations critiques
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Met en pause watchdog
  // ... opération OTA ...
  KISS_INIT_WDT();   // Réactive watchdog
#endif
```

### Changer Taille du Buffer PSRAM

Dans `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB par défaut
  // Ajuster selon PSRAM disponible dans votre ESP32
}
```

---
### Changer PIN/PUK par défaut

Dans `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Dépannage

### Erreur: "❌ Pas de PSRAM disponible"
**Cause:** ESP32 sans PSRAM ou PSRAM non activée

**Solution:**
1. Vérifier que l'ESP32-S3 a de la PSRAM physique
2. Dans Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Recompiler le projet

---

### Erreur: "❌ Erreur lors de la création du backup"
**Cause:** LittleFS sans espace ou non monté

**Solution:**
1. Vérifier partition `spiffs` dans `partitions.csv`
2. Formater LittleFS si nécessaire
3. Augmenter taille de partition spiffs

---

### Erreur: "🔥 BOOT LOOP: 3+ démarrages en 5 minutes"
**Cause:** Le nouveau firmware échoue de manière constante

**Solution:**
- Automatique: Rollback s'exécute seul
- Manuelle: Attendre le rollback automatique
- Préventive: Tester le firmware sur un autre ESP32 avant OTA

---

### Le Firmware ne se Valide pas après /otaok
**Cause:** Timeout de 60 secondes expiré

**Solution:**
- Envoyer `/otaok` en laissant plus de temps après le redémarrage pour permettre une connexion stable
- Vérifier connectivité WiFi après redémarrage
- Augmenter `BOOT_VALIDATION_TIMEOUT` si nécessaire

---

## Exemple de Code d'Intégration

### Initialisation dans `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Initialiser identifiants
  credentials = new KissCredentials();
  credentials->begin();

  // Initialiser bot Telegram
  bot = new KissTelegram(BOT_TOKEN);

  // Initialiser OTA
  ota = new KissOTA(bot, credentials);

  // Vérifier si on vient d'un OTA interrompu
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Traiter commandes Telegram
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // Boucle OTA (gère timeouts et états)
  ota->loop();
}
```

---

## Information Technique Supplémentaire

### Format des Boot Flags (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (validation)
  uint32_t bootCount;          // Compteur de démarrages
  uint32_t lastBootTime;       // Timestamp dernier boot
  bool otaInProgress;          // true si en attente de validation
  bool firmwareValid;          // true si firmware validé
  char backupPath[64];         // Chemin du backup (/ota_backup.bin)
};
```

### Tailles Typiques

| Élément | Taille Typique |
|----------|---------------|
| Firmware compilé | 1.0 - 1.5 MB |
| Backup dans LittleFS | ~1.1 MB |
| Buffer PSRAM | 7-8 MB |
| Partition Factory | 3.5 MB |
| Partition App | 3.5 MB |

---

## Contributions et Support

**Auteur:** Vicente Soriano
**Email:** victek@gmail.com
**Projet:** KissTelegram Suite

Pour signaler des bugs ou demander des fonctionnalités, contactez l'auteur.

---

## Questions Fréquentes (FAQ)

### Pourquoi ne pas utiliser AsyncElegantOTA ou ArduinoOTA?

**Réponse Courte:** KissOTA ne nécessite pas de configuration réseau et fonctionne globalement dès le premier instant.

**Réponse Complète:**

AsyncElegantOTA et ArduinoOTA sont excellents pour le développement local, mais ont des limitations en production:

1. **Accès Distant Compliqué:**
   - Vous devez connaître l'IP du dispositif
   - S'il est derrière un routeur/NAT, vous avez besoin de port forwarding
   - Le port forwarding expose votre ESP32 à internet (risque de sécurité)
   - Alternative: VPN (complexe à configurer pour utilisateurs finaux)

2. **Sécurité Limitée:**
   - Utilisateur/mot de passe basique (vulnérable à la force brute)
   - HTTP sans chiffrement (à moins de configurer HTTPS avec certificats)
   - Serveur web exposé = surface d'attaque large

3. **Sans Rollback Réel:**
   - Seulement 2 partitions (OTA_0 et OTA_1)
   - Si les deux échouent, dispositif "briqué"
   - Pas de backup du firmware fonctionnel connu

**KissOTA résout ceci:**
- ✅ Mise à jour globale sans rien configurer (API Telegram)
- ✅ Sécurité robuste (PIN/PUK + infrastructure Telegram)
- ✅ Rollback multiniveau (backup + factory + détection boot loop)
- ✅ N'expose pas de ports vers l'extérieur

**Quand utiliser chacun?**
- **AsyncElegantOTA:** Développement local, prototypage rapide, LAN privé
- **KissOTA:** Production, dispositifs distants globaux, sécurité critique

---

### Fonctionne sans PSRAM?

**Réponse:** La version actuelle de KissOTA **nécessite PSRAM** pour le buffer de téléchargement et la vérification de validité du fichier.

**Raisons techniques:**
- Firmware typique: 1-1.5 MB
- Buffer PSRAM: 7-8 MB disponibles
- RAM normale ESP32-S3: Seulement ~70-105 KB libres

**Alternatives si vous n'avez pas de PSRAM:**
1. **Télécharger vers LittleFS d'abord:**
   - Plus lent (flash est plus lent que PSRAM)
   - Use le flash inutilement
   - Nécessite espace libre dans LittleFS

2. **Utiliser AsyncElegantOTA:**
   - Ne nécessite pas PSRAM
   - Télécharge directement vers partition OTA

3. **Contribuer un PR:**
   - Si vous implémentez le support pour ESP32 sans PSRAM, les PR sont bienvenus

**Matériel recommandé:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 euros
- ESP32-S3-DevKitC-1 avec module N16R8

---

### Puis-je utiliser KissOTA sans Telegram?

**Réponse:** Techniquement oui, mais nécessite un travail d'adaptation.

L'architecture de KissOTA a deux couches:

1. **Core OTA (backend):**
   - Gestion des états
   - Backup/rollback
   - Flash depuis PSRAM
   - Validation CRC32
   - Boot flags
   - **Cette partie est indépendante du transport**

2. **Frontend Telegram:**
   - Authentification PIN/PUK
   - Téléchargement de fichiers depuis l'API Telegram
   - Messages de progression à l'utilisateur
   - **Cette partie dépend de KissTelegram**

**Pour utiliser un autre transport (HTTP, MQTT, Serial, etc.):**

```cpp
// Vous devriez implémenter votre propre frontend
class KissOTACustom : public KissOTACore {
public:
  // Implémenter méthodes abstraites:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**L'effort en vaut-il la peine?**
- Si vous avez déjà KissTelegram: **Non, utilisez KissOTA tel quel**
- Si vous ne voulez pas Telegram: **Probablement mieux d'utiliser AsyncElegantOTA**
- Si vous avez un cas d'usage spécifique (ex: MQTT industriel): **KissTelegram a une version Entreprise, demandez**

---

### Que se passe-t-il si je perds la connexion WiFi pendant le téléchargement?

**Réponse:** Le système gère les déconnexions de manière sécurisée.

**Scénarios:**

**1. Téléchargement vers PSRAM interrompu:**
```
État: DOWNLOADING → Timeout réseau
Action automatique:
  1. Détecte timeout (après 30 secondes sans données)
  2. Nettoie buffer PSRAM
  3. Retente téléchargement (maximum 3 tentatives)
  4. Si 3 échecs: Annule OTA, retour à IDLE
  5. Firmware actuel NON touché (sûr)
```

**2. Déconnexion pendant flash:**
```
État: FLASHING → WiFi tombe
Résultat:
  - Le flash continue (ne dépend pas du WiFi)
  - Les données sont déjà en PSRAM
  - Le flash se termine normalement
  - Au redémarrage, attend /otaok (60s)
  - S'il n'y a pas de WiFi pour envoyer /otaok: Rollback automatique
```

**3. Déconnexion pendant validation:**
```
État: VALIDATING → Sans WiFi
Résultat:
  - Timer de 60 secondes en cours
  - Si WiFi revient avant 60s: Vous pouvez envoyer /otaok
  - Si 60s passent sans /otaok: Rollback automatique
  - Le système retourne au firmware précédent (sûr)
```

**Timeout global:** 7 minutes depuis /otaconfirm jusqu'à compléter tout. Si dépassé, OTA s'annule automatiquement.

---

### Est-il sûr de flasher un firmware plus petit que l'actuel?

**Réponse:** Oui, c'est complètement sûr.

**Explication technique:**

Le processus de flash toujours:
1. **Efface complètement la partition destination** avant d'écrire
2. **Écrit le nouveau firmware** (quelle que soit sa taille)
3. **Marque la taille réelle** dans les métadonnées de la partition

**Exemple:**
```
Firmware actuel:  1.5 MB
Nouveau firmware: 0.9 MB

Processus:
1. Backup de 1.5 MB → LittleFS
2. Effacer partition app (3.5 MB complets)
3. Écrire 0.9 MB nouveaux
4. Métadonnées: size = 0.9 MB
5. Espace restant dans partition: Vide/effacé
```

**Pas de problème de "déchets résiduels"** car:
- L'ESP32 exécute seulement jusqu'à la fin du firmware marqué
- Le reste de la partition est effacé
- CRC32 est calculé seulement sur la taille réelle

**Cas d'usage courants:**
- Désactiver des fonctionnalités pour réduire la taille
- Firmware d'urgence minimaliste (~500 KB)
- Compilation optimisée avec flags différents

---

### Comment réinitialiser le système si tout échoue?

**Réponse:** Vous avez 3 options de récupération.

#### Option 1: Rollback Automatique (Plus Courant)
Si le nouveau firmware échoue, simplement **ne faites rien**:
```
1. Le nouveau firmware démarre mais échoue
2. L'ESP32 redémarre (crash/watchdog)
3. Compteur bootCount++ dans NVS
4. Après 3 redémarrages en 5 minutes → Rollback automatique
5. Le système restaure le firmware précédent depuis /ota_backup.bin
```

#### Option 2: Rollback Manuel
Si vous avez accès à Telegram:
```
/otacancel
```
Force le rollback immédiat depuis le backup.

#### Option 3: Boot en Factory (Urgence)
Si tout le reste échoue:

**Via Série (nécessite accès physique):**
```cpp
// Dans setup(), détecte condition d'urgence
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin à GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Via esptool (dernier recours):**
```bash
# Flasher partition factory avec firmware original
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Option 4: Flash Complet (Effacer et Recommencer)
```bash
# Efface tout le flash
esptool.py --port COM13 erase_flash

# Flashe firmware frais + partitions
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prévention:**
- ✅ Maintenez toujours un firmware factory stable
- ✅ Testez les nouveaux firmwares sur dispositif de développement d'abord
- ✅ Ne modifiez pas la partition factory en production

---

### Combien de temps prend une mise à jour OTA complète?

**Réponse:** Environ 1-3 minutes au total pour un firmware de ~1 MB si le WiFi est parfait.

**Répartition temporelle typique:**

| Phase | Durée Typique | Facteurs |
|------|----------------|----------|
| **Authentification** | 10-30 secondes | Temps de l'utilisateur pour écrire PIN |
| **Téléversement vers Telegram** | 5-15 secondes | Vitesse internet de l'utilisateur |
| **Téléchargement vers PSRAM** | 10-20 secondes | Vitesse WiFi de l'ESP32 + API Telegram |
| **Vérification CRC32** | 2-5 secondes | Taille du firmware |
| **Confirmation utilisateur** | 5-30 secondes | Temps de réaction de l'utilisateur |
| **Backup firmware actuel** | 10-30 secondes | Vitesse d'écriture LittleFS |
| **Flash nouveau firmware** | 5-10 secondes | Vitesse d'écriture flash |
| **Redémarrage** | 5-10 secondes | Temps de boot de l'ESP32 |
| **Validation /otaok** | 5-60 secondes | Temps de réaction de l'utilisateur |
| **TOTAL** | **~2-4 minutes** | Varie selon les conditions |

**Facteurs qui affectent la vitesse:**

**Plus rapide:**
- ✅ WiFi fort (près du routeur)
- ✅ Firmware petit (<1 MB)
- ✅ Utilisateur expérimenté (répond vite)
- ✅ LittleFS non fragmenté. KissTelegram défragmente le FS toutes les 3-5 minutes

**Plus lent:**
- ⚠️ WiFi faible ou congestionné
- ⚠️ Firmware grand (>1.5 MB)
- ⚠️ Utilisateur hésitant à confirmer
- ⚠️ LittleFS très plein

**Timeout de sécurité:** 7 minutes maximum depuis /otaconfirm. Si dépassé, OTA s'annule.

---

### Puis-je mettre à jour plusieurs ESP32 simultanément?

**Réponse:** Oui, mais chaque ESP32 a besoin de son propre bot Telegram (différent BOT_TOKEN).

**Limitation de Telegram:**
- Un bot Telegram ne peut traiter qu'1 conversation simultanée de manière fiable
- L'API Telegram a des limites de taux (~30 messages/seconde par bot)

**Option 1: Un Bot par Dispositif (Recommandé pour Production)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Avantages:**
- ✅ OTAs complètement indépendants
- ✅ Sans conflits de messages
- ✅ Chaque dispositif a son propre chat

**Inconvénients:**
- ⚠️ Plus de bots à gérer
- ⚠️ Plus de tokens à configurer

---

**Option 2: Un Bot, Plusieurs Dispositifs Séquentiellement**
```cpp
// Tous utilisent le même BOT_TOKEN
// Mais mettez-les à jour UN À LA FOIS manuellement
```

**Processus:**
1. Mettre à jour ESP32 #1 → Attendre que ça se termine
2. Mettre à jour ESP32 #2 → Attendre que ça se termine
3. etc.

**Avantages:**
- ✅ Un seul bot à gérer
- ✅ Plus simple pour peu de dispositifs

**Inconvénients:**
- ⚠️ Un à la fois (lent pour beaucoup de dispositifs)
- ⚠️ Facile de se tromper de dispositif

---

**Option 3: Gestion Centralisée (KissTelegram Entreprise)**
```
Système central avec API propre
    ↓
Distribue firmware vers plusieurs ESP32
    ↓
Chaque ESP32 rapporte progression au système central
    ↓
Système central notifie via API Json ou utilisateur via Telegram
```

**Caractéristiques souhaitées (Mais disponibles dans KissTelegram Entreprise):**
- Backend web/cloud
- Base de données de dispositifs
- File d'attente de tâches OTA
- Tableau de bord de surveillance

**Contributions bienvenues** KissOTA a énormément de possibilités, demandez ou faites un PR.

---

## Changelog

### v0.9.0 (Actuelle)
- ✅ Système de version simplifié
- ✅ Suppression de la vérification de rétrogradation
- ✅ Nettoyage du code obsolète
- ✅ Améliorations des messages de log

### v0.1.0
- ✅ Implémentation de backup/rollback automatique
- ✅ Détection de boot loops
- ✅ Intégration complète avec Telegram

### v0.0.2
- ✅ Refactorisation complète vers PSRAM
- ✅ Suppression des partitions OTA_0/OTA_1
- ✅ Système de validation de 60 secondes

### v0.0.1
- ✅ Version initiale avec OTA basique

---

**Document mis à jour:** 12/12/2025
**Version du document:** 0.9.0
**Auteur** Vicente Soriano, victek@gmail.com**
**Licence MIT**
