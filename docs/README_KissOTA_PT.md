# KissOTA - Sistema de Atualização OTA para ESP32

## Índice
- [Descrição Geral](#descrição-geral)
- [Características Principais](#características-principais)
- [Comparativo com Outros Sistemas OTA](#comparativo-com-outros-sistemas-ota)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Vantagens sobre o OTA Padrão da Espressif](#vantagens-sobre-o-ota-padrão-da-espressif)
- [Requisitos de Hardware](#requisitos-de-hardware)
- [Configuração de Partições](#configuração-de-partições)
- [Fluxo do Processo OTA](#fluxo-do-processo-ota)
- [Comandos Disponíveis](#comandos-disponíveis)
- [Características de Segurança](#características-de-segurança)
- [Gestão de Erros e Recuperação](#gestão-de-erros-e-recuperação)
- [Uso Básico](#uso-básico)
- [Perguntas Frequentes (FAQ)](#perguntas-frequentes-faq)

---

## Descrição Geral

KissOTA é um sistema avançado de atualização Over-The-Air (OTA) projetado especificamente para dispositivos ESP32-S3 com PSRAM. Ao contrário do sistema OTA padrão da Espressif, KissOTA fornece múltiplas camadas de segurança, validação e recuperação automática, tudo integrado com Telegram para atualizações remotas seguras.

**Autor:** Vicente Soriano (victek@gmail.com)
**Versão atual:** 0.9.0
**Licença:** Incluída no KissTelegram Suite

---

## Características Principais

### 🔒 Segurança Robusta
- **Autenticação PIN/PUK**: Controle de acesso mediante códigos PIN e PUK
- **Bloqueio automático**: Após 3 tentativas falhas de PIN
- **Verificação CRC32**: Validação de integridade do firmware antes do flash
- **Validação pós-flash**: Requer confirmação manual do usuário

### 🛡️ Proteção contra Falhas
- **Backup automático**: Cópia do firmware atual antes de atualizar
- **Rollback automático**: Restauração se o novo firmware falhar
- **Detecção de boot loops**: Se falhar 3 vezes em 5 minutos, rollback automático
- **Partição factory**: Rede de segurança final se tudo mais falhar
- **Timeout global**: 7 minutos para completar todo o processo

### 💾 Uso Eficiente de Recursos
- **Buffer PSRAM**: Usa 7-8MB de PSRAM para baixar sem tocar no flash
- **Economia de espaço**: Só precisa de partições factory + app (sem OTA_0/OTA_1)
- **Limpeza automática**: Remove arquivos temporários e backups antigos

### 📱 Integração com Telegram
- **Atualização remota**: Envia o .bin diretamente pelo Telegram
- **Feedback em tempo real**: Mensagens de progresso e status
- **Multi-idioma**: Suporte para 7 idiomas (ES, EN, FR, IT, DE, PT, CN)

---

## Comparativo com Outros Sistemas OTA

### KissOTA vs Outras Soluções Populares

| Característica | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **Transporte** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Acesso remoto** | ✅ Global sem config | ❌ Só LAN* | ❌ Só LAN* | ❌ Só LAN* | ❌ Só LAN* |
| **Requer IP conhecido** | ❌ Não | ✅ Sim | ✅ Sim | ✅ Sim | ✅ Sim |
| **Port forwarding** | ❌ Não necessário | ⚠️ Para acesso remoto | ⚠️ Para acesso remoto | ⚠️ Para acesso remoto | ⚠️ Para acesso remoto |
| **Servidor web** | ❌ Não | ✅ AsyncWebServer | ✅ WebServer | ✅ Configurável | ✅ WebServer |
| **Autenticação** | 🔒 PIN/PUK (robusto) | ⚠️ Usuário/senha básico | ⚠️ Senha opcional | ⚠️ Básica | ⚠️ Usuário/senha básico |
| **Backup firmware** | ✅ Em LittleFS | ❌ Não | ❌ Não | ❌ Não | ❌ Não |
| **Rollback automático** | ✅✅✅ 3 níveis | ⚠️ Limitado (2 part.) | ❌ Não | ⚠️ Limitado (2 part.) | ⚠️ Limitado (2 part.) |
| **Validação manual** | ✅ 60s com /otaok | ❌ Não | ❌ Não | ⚠️ Opcional | ❌ Não |
| **Boot loop detection** | ✅ Automático | ❌ Não | ❌ Não | ❌ Não | ❌ Não |
| **Buffer de download** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Progresso em tempo real** | ✅ Telegram messages | ✅ Web UI | ⚠️ Serial only | ⚠️ Configurável | ✅ Web UI |
| **Interface de usuário** | 📱 Telegram chat | 🖥️ Navegador web | 🖥️ Navegador web | ⚡ Programática | 🖥️ Navegador web |
| **Dependências** | KissTelegram | ESPAsyncWebServer | ESP mDNS | Nenhuma (nativo) | WebServer |
| **Flash requerido** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Segurança na internet** | ✅ Alta (Telegram API) | ⚠️ Vulnerável se exposto | ⚠️ Vulnerável se exposto | ⚠️ Vulnerável se exposto | ⚠️ Vulnerável se exposto |
| **Fácil de usar** | ✅✅ Só enviar .bin | ✅ UI web intuitiva | ⚠️ Requer configuração | ⚠️ Complexidade alta | ✅ UI web intuitiva |
| **Multi-idioma** | ✅ 7 idiomas | ❌ Só inglês | ❌ Só inglês | ❌ Só inglês | ❌ Só inglês |
| **Recuperação factory** | ✅ Sim | ❌ Não | ❌ Não | ⚠️ Manual | ❌ Não |

\* *Com port forwarding/VPN é possível acesso remoto, mas requer configuração avançada de rede*

### Vantagens Únicas do KissOTA

#### 🌍 **Acesso Verdadeiramente Global**
Outros sistemas OTA requerem:
- Conhecer o IP do dispositivo
- Estar na mesma rede LAN, ou
- Configurar port forwarding (arriscado), ou
- Configurar VPN (complexo)

**KissOTA:** Só precisa do Telegram. Atualize de qualquer parte do mundo sem configuração de rede. Como está integrado ao KissTelegram usa conexão SSL.

#### 🔒 **Segurança sem Compromissos**
Expor um servidor web HTTP à internet é perigoso:
- Vulnerável a ataques de força bruta
- Possível vetor de exploits web
- Requer HTTPS para segurança (certificados, etc.)

**KissOTA:** Usa a infraestrutura segura do Telegram. Seu ESP32 nunca expõe portas ao exterior.

#### 🛡️ **Recuperação Multinível**
Outros sistemas com rollback (ESP-IDF, AsyncElegantOTA) só têm 2 partições:
- Se ambas as partições falharem → dispositivo "bricked"
- Sem backup do firmware funcional

**KissOTA:** 3 níveis de segurança:
1. **Nível 1:** Rollback desde backup no LittleFS
2. **Nível 2:** Boot desde partição factory original
3. **Nível 3:** Detecção de boot loops automática

#### 💾 **Economia de Espaço Flash**
Sistemas tradicionais (dual-bank OTA):
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
Backup:   ~1.1 MB │  (no LittleFS, comprimível)
Total: ~8.1 MB   ┘
```
**Ganho:** ~2.4 MB de flash livre para sua aplicação ou dados.

#### 🚀 **PSRAM como Buffer**
Outros sistemas baixam diretamente para flash:
- **Desgaste do flash:** Flash tem ciclos limitados de escrita (~100K)
- **Risco:** Se o download falhar, flash já foi escrito parcialmente

**KissOTA:**
- Download completo para PSRAM primeiro
- Verifica CRC32 na PSRAM
- Só escreve no flash se tudo estiver OK
- **PSRAM não tem desgaste:** Infinitos ciclos de escrita

---

## Arquitetura do Sistema

### Estrutura de Partições

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Firmware original de fábrica
│  - Rede de segurança final         │
│  - Só leitura em produção          │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Firmware em execução
│  - Firmware ativo atual             │
│  - Se atualiza durante OTA          │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (restante ~8-9 MB)        │
│  - /ota_backup.bin (backup)         │ ← Backup do firmware anterior
│  - Arquivos de configuração         │
│  - Dados persistentes               │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot flags (validação)           │ ← Estado de validação OTA
│  - Credenciais PIN/PUK              │
│  - Contador de inicializações       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Buffer de download temporal      │ ← Firmware baixado antes do flash
│  - Não persistente (se apaga)       │
└─────────────────────────────────────┘
```

### Fluxo de Dados Durante OTA

```
Telegram API
    │
    ▼
[Download para PSRAM] ← 7-8 MB buffer temporal
    │
    ▼
[Verificação CRC32]
    │
    ▼
[Backup firmware atual → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash novo firmware → App Partition]
    │
    ▼
[Reinício do ESP32]
    │
    ▼
[Validação 60 segundos] → /otaok ou rollback automático
    │
    ▼
[Apaga firmware anterior] -> Recupera o espaço de firmware anterior

```
---

## Vantagens sobre o OTA Padrão da Espressif

| Característica | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **Espaço Flash requerido** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (só app) |
| **Backup do firmware** | ❌ Não | ✅ Sim, no LittleFS |
| **Rollback automático** | ⚠️ Limitado | ✅ Completo + factory |
| **Validação manual** | ❌ Não | ✅ Sim, 60 segundos |
| **Autenticação** | ❌ Não | ✅ PIN/PUK |
| **Detecção boot loops** | ❌ Não | ✅ Sim, automática |
| **Integração Telegram** | ❌ Não | ✅ Nativa |
| **Buffer de download** | Flash | PSRAM (não desgasta flash) |
| **Recuperação de emergência** | ⚠️ Só 2 partições | ✅ 3 níveis (app/backup/factory) |

---

## Requisitos de Hardware

### Mínimo Requerido
- **MCU**: ESP32-S3 (outras variantes ESP32 podem funcionar com adaptações)
- **PSRAM**: Mínimo depende do tamanho do firmware a substituir 2-8MB PSRAM (para buffer de download)
- **Flash**: Mínimo depende do tamanho do firmware a substituir 8-16-32MB
- **Conectividade**: WiFi funcional

### Configuração Recomendada
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (ideal)
- **ESP32-S3-DevKitC-1**: Com módulo N16R8 ou superior

---

## Configuração de Partições

Arquivo `partitions.csv` recomendado (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Configuração no Arduino IDE:**
- Tools → Partition Scheme → Custom
- Apontar para o arquivo `partitions.csv`

---

## Fluxo do Processo OTA

### Diagrama de Estados

```
┌─────────────┐
│  OTA_IDLE   │ ← Estado inicial
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Solicita PIN (3 tentativas)
└──────┬──────┘
       │ PIN correto
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, pronto para receber .bin
└──────┬───────┘
       │ Usuário envia .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Download para PSRAM (progresso em tempo real)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Calcula CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Espera /otaconfirm do usuário
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Guarda firmware atual no LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Escreve novo firmware desde PSRAM
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
       │ Timeout ou /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Restaura backup automaticamente
└──────────────┘
```

### Timeouts Importantes

- **Autenticação PIN**: Ilimitado (até 3 tentativas), se falhar, PIN bloqueado e precisa PUK para restaurá-lo
- **Recepção do .bin**: Espera até 7 minutos, se for ultrapassado cancela /ota
- **Confirmação**: Espera até 7 minutos, se for ultrapassado cancela /ota (espera /otaconfirm)
- **Processo completo**: 7 minutos máximo desde /otaconfirm
- **Validação pós-flash**: 60 segundos para /otaok

---

## Comandos Disponíveis

### `/ota`
Inicia o processo OTA.

**Uso:**
```
/ota
```

**Resposta:**
- Se PIN não bloqueado: Solicita PIN
- Se PIN bloqueado: Solicita PUK

---

### `/otapin <código>`
Envia o código PIN (4-8 dígitos).

**Uso:**
```
/otapin 0000 (por padrão)
```

**Resposta:**
- ✅ PIN correto: Estado AUTHENTICATED, pronto para receber .bin
- ❌ PIN incorreto: Reduz tentativas restantes
- 🔒 Após 3 falhas: Bloqueia e solicita PUK

---

### `/otapuk <código>`
Desbloqueia o sistema com o código PUK.

**Uso:**
```
/otapuk 12345678
```

**Resposta:**
- ✅ PUK correto: Desbloqueia PIN e solicita novo PIN
- ❌ PUK incorreto: Permanece bloqueado

---

### `/otaconfirm`
Confirma que deseja gravar o firmware baixado.

**Uso:**
```
/otaconfirm
```

**Pré-condições:**
- Firmware baixado e verificado
- Checksum CRC32 OK

**Ação:**
- Cria backup do firmware atual
- Grava novo firmware
- Reinicia o ESP32

---

### `/otaok`
Valida que o novo firmware funciona corretamente.

**Uso:**
```
/otaok
```

**Pré-condições:**
- Deve ser enviado dentro de 60 segundos após o reinício
- Só disponível após um flash OTA

**Ação:**
- Marca o firmware como válido
- Remove o backup anterior
- Sistema volta à operação normal

⚠️ **IMPORTANTE**: Se não for enviado `/otaok` em 60 segundos, será executado rollback automático.

---

### `/otacancel`
Cancela o processo OTA ou força rollback.

**Uso:**
```
/otacancel
```

**Comportamento:**
- Durante download/validação: Cancela e limpa arquivos temporários
- Durante validação pós-flash: Executa rollback imediato
- Se não há OTA ativo: Informa que não há processo ativo

---

## Características de Segurança

### 1. Autenticação PIN/PUK

#### PIN e PUK (Podem ser alterados remotamente com /changepin <velho> <novo> ou /changepuk <velho> <novo>)

#### PIN (Personal Identification Number)
- **Comprimento**: 4-8 dígitos
- **Tentativas**: 3 tentativas antes do bloqueio
- **Configuração**: Definido em `system_setup.h` Credenciais Fallback
- **Persistência**: É guardado seguro no NVS

#### PUK (PIN Unlock Key)
- **Comprimento**: 8 dígitos
- **Função**: Desbloquear após 3 falhas de PIN
- **Segurança**: Só o administrador deveria conhecê-lo

**Exemplo de fluxo de bloqueio:**
```
Tentativa 1: PIN incorreto → "❌ PIN incorreto. 2 tentativas restantes"
Tentativa 2: PIN incorreto → "❌ PIN incorreto. 1 tentativa restante"
Tentativa 3: PIN incorreto → "🔒 PIN bloqueado. Use /otapuk [código]"
```

### 2. Verificação de Integridade

#### CRC32 Checksum
- **Algoritmo**: CRC32 IEEE 802.3
- **Cálculo**: Sobre todo o arquivo .bin baixado
- **Validação**: Antes de permitir /otaconfirm
- **Rejeição**: Se CRC32 não coincidir, download é removido

**Exemplo de saída:**
```
🔍 Verificando checksum...
🔍 CRC32: 0xF8CAACF6 (1.07 MB verificados)
✅ Checksum OK
```

### 3. Validação Pós-Flash

#### Janela de Validação (60 segundos)
Após gravar o novo firmware:
1. ESP32 reinicia
2. Boot flags marcam `otaInProgress = true`
3. Usuário tem 60 segundos para enviar `/otaok`
4. Se não responder → rollback automático

**Diagrama temporal:**
```
FLASH → REINÍCIO → [60s para /otaok] → ROLLBACK se timeout
                         ↓
                      /otaok → ✅ Firmware validado
```

### 4. Proteção contra Modificação Não Autorizada

- **Modo manutenção**: Durante OTA, outros comandos estão limitados
- **Chat ID único**: Só o chat_id configurado pode executar OTA
- **Autenticação prévia**: PIN/PUK obrigatórios antes de qualquer ação

---

## Gestão de Erros e Recuperação

### Níveis de Recuperação

#### Nível 1: Tentativa Automática
**Cenários:**
- Falha no download do Telegram (max 3 tentativas)
- Timeout de rede
- Download interrompido

**Ação:**
- Limpa PSRAM
- Tenta download automaticamente
- Notifica o usuário da tentativa

---

#### Nível 2: Rollback desde Backup
**Cenários:**
- Usuário não envia `/otaok` em 60 segundos
- Usuário envia `/otacancel` durante validação
- Boot loop detectado (3+ inicializações em 5 minutos)

**Processo de Rollback:**
1. Detecta `bootFlags.otaInProgress == true`
2. Lê `bootFlags.backupPath` → `/ota_backup.bin`
3. Restaura backup desde LittleFS → App Partition
4. Reinicia ESP32
5. Limpa boot flags
6. Notifica o usuário pelo Telegram

**Código de exemplo:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // Não há backup
  }

  // Lê /ota_backup.bin desde LittleFS
  // Escreve em App Partition
  // Reinicia
}
```

---

#### Nível 3: Fallback para Factory
**Cenários:**
- Rollback desde backup falha
- Arquivo `/ota_backup.bin` corrompido
- Erro crítico no flash

**Processo:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Reinicia ESP32
3. Inicializa desde firmware original de fábrica
4. Notifica erro crítico ao usuário

⚠️ **IMPORTANTE**: Este é o último recurso. O firmware factory deve ser estável e nunca modificado em produção.

---

### Detecção de Boot Loops

**Algoritmo:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minutos
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ inicializações em 5 minutos");
      return true; // Executar rollback
    }
  }
  return false;
}
```

**Proteção:**
- Incrementa `bootFlags.bootCount` em cada inicialização
- Se > 3 inicializações em < 5 minutos → Rollback automático
- Ao validar com `/otaok`, reseta contador

---

### Estados de Erro

| Erro | Código | Ação Automática |
|-------|--------|-------------------|
| **Download falhou** | `DOWNLOAD_FAILED` | Tentar até 3 vezes |
| **Checksum incorreto** | `CHECKSUM_MISMATCH` | Remover download, cancelar OTA |
| **Falha no backup** | `BACKUP_FAILED` | Cancelar OTA, não arriscar |
| **Falha no flash** | `FLASH_FAILED` | Cancelar OTA, manter firmware atual |
| **Timeout validação** | `VALIDATION_TIMEOUT` | Rollback automático |
| **Boot loop** | `BOOT_LOOP_DETECTED` | Rollback automático |
| **Rollback falhou** | `ROLLBACK_FAILED` | Fallback para factory |
| **Sem backup** | `NO_BACKUP` | Manter firmware atual, avisar |

---

## Uso Básico

### Atualização Completa Passo a Passo

#### 1. Preparar o Firmware
```bash
# Compilar o projeto no Arduino IDE
# O .bin é gerado em: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. Iniciar OTA
Do Telegram:
```
/ota
```

Resposta do bot:
```
🔐 AUTENTICAÇÃO OTA

Insira o código PIN de 4-8 dígitos:
/otapin [código]

Tentativas restantes: 3
```

#### 3. Autenticar com PIN
```
/otapin 0000
```

Resposta:
```
✅ PIN correto

Sistema pronto para OTA.
Envie o arquivo .bin do novo firmware.
```

#### 4. Enviar o Firmware
- Anexe o arquivo `.bin` como documento (não como foto)
- Telegram faz o upload e o bot baixa automaticamente

Resposta durante download:
```
📥 Baixando firmware para PSRAM...
⏳ Progresso: 45%
```

#### 5. Verificação Automática
```
✅ Download completo: 1.07 MB na PSRAM
🔍 Verificando checksum...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VERIFICADO

Arquivo: suite_kiss.ino.bin
Tamanho: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ CONFIRMAÇÃO REQUERIDA
Para gravar o firmware:
/otaconfirm

Para cancelar:
/otacancel
```

#### 6. Confirmar Flash
```
/otaconfirm
```

Resposta:
```
💾 Iniciando backup...
✅ Backup completo: 1123456 bytes

⚡ Gravando firmware...
✅ FLASH COMPLETADO

Firmware escrito corretamente.
O dispositivo será reiniciado agora.

Após reiniciar terá 60 segundos para validar com /otaok
Se não validar será executado rollback automático.
```

#### 7. Validação Pós-Reinício
Após o reinício (em 60 segundos):
```
/otaok
```

Resposta:
```
✅ FIRMWARE VALIDADO

O novo firmware foi confirmado.
Sistema volta à operação normal.

Versão: 0.9.0
```

---

### Exemplo de Rollback Manual

Se o firmware novo não funcionar bem:

```
/otacancel
```

Resposta:
```
⚠️ EXECUTANDO ROLLBACK

Restaurando firmware anterior desde backup...
✅ Firmware anterior restaurado
🔄 Reiniciando...

[Após reinício]
✅ ROLLBACK COMPLETADO

Sistema restaurado ao firmware anterior.
```

---

## Configuração Avançada

### Personalizar Timeouts

Em `KissOTA.h`:

```cpp
// Timeout de validação (por padrão 60 segundos)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Timeout global do processo OTA (por padrão 7 minutos)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### Ativar/Desativar WDT Durante OTA

Em `system_setup.h`:

```cpp
// Desativar WDT durante operações críticas
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Pausa watchdog
  // ... operação OTA ...
  KISS_INIT_WDT();   // Reativa watchdog
#endif
```

### Mudar Tamanho do Buffer PSRAM

Em `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB por padrão
  // Ajustar segundo PSRAM disponível no seu ESP32
}
```

---
### Mudar PIN/PUK por padrão

Em `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Solução de Problemas

### Erro: "❌ Não há PSRAM disponível"
**Causa:** ESP32 sem PSRAM ou PSRAM não habilitada

**Solução:**
1. Verificar que o ESP32-S3 tenha PSRAM física
2. No Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Recompilar o projeto

---

### Erro: "❌ Erro criando backup"
**Causa:** LittleFS sem espaço ou não montado

**Solução:**
1. Verificar partição `spiffs` em `partitions.csv`
2. Formatar LittleFS se necessário
3. Aumentar tamanho da partição spiffs

---

### Erro: "🔥 BOOT LOOP: 3+ inicializações em 5 minutos"
**Causa:** Firmware novo falha consistentemente

**Solução:**
- Automática: Rollback é executado sozinho
- Manual: Esperar o rollback automático
- Preventiva: Testar firmware em outro ESP32 antes do OTA

---

### Firmware não se valida após /otaok
**Causa:** Timeout de 60 segundos expirado

**Solução:**
- Enviar `/otaok` deixando mais tempo após reinício para permitir conexão estável
- Verificar conectividade WiFi após reinício
- Aumentar `BOOT_VALIDATION_TIMEOUT` se necessário

---

## Código de Exemplo de Integração

### Inicialização em `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Inicializar credenciais
  credentials = new KissCredentials();
  credentials->begin();

  // Inicializar bot Telegram
  bot = new KissTelegram(BOT_TOKEN);

  // Inicializar OTA
  ota = new KissOTA(bot, credentials);

  // Verificar se vem de um OTA interrompido
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Processar comandos Telegram
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // Loop de OTA (gerencia timeouts e estados)
  ota->loop();
}
```

---

## Informação Técnica Adicional

### Formato de Boot Flags (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (validação)
  uint32_t bootCount;          // Contador de inicializações
  uint32_t lastBootTime;       // Timestamp última inicialização
  bool otaInProgress;          // true se esperando validação
  bool firmwareValid;          // true se firmware validado
  char backupPath[64];         // Caminho do backup (/ota_backup.bin)
};
```

### Tamanhos Típicos

| Elemento | Tamanho Típico |
|----------|---------------|
| Firmware compilado | 1.0 - 1.5 MB |
| Backup no LittleFS | ~1.1 MB |
| Buffer PSRAM | 7-8 MB |
| Partição Factory | 3.5 MB |
| Partição App | 3.5 MB |

---

## Contribuições e Suporte

**Autor:** Vicente Soriano
**Email:** victek@gmail.com
**Projeto:** KissTelegram Suite

Para reportar bugs ou solicitar características, contatar o autor.

---

## Perguntas Frequentes (FAQ)

### Por que não usar AsyncElegantOTA ou ArduinoOTA?

**Resposta Curta:** KissOTA não requer configuração de rede e funciona globalmente desde o primeiro momento.

**Resposta Completa:**

AsyncElegantOTA e ArduinoOTA são excelentes para desenvolvimento local, mas têm limitações em produção:

1. **Acesso Remoto Complicado:**
   - Precisa conhecer o IP do dispositivo
   - Se está atrás de roteador/NAT, precisa de port forwarding
   - Port forwarding expõe seu ESP32 à internet (risco de segurança)
   - Alternativa: VPN (complexo de configurar para usuários finais)

2. **Segurança Limitada:**
   - Usuário/senha básico (vulnerável a força bruta)
   - HTTP sem criptografia (a menos que configure HTTPS com certificados)
   - Servidor web exposto = superfície de ataque ampla

3. **Sem Rollback Real:**
   - Só têm 2 partições (OTA_0 e OTA_1)
   - Se ambas falharem, dispositivo "bricked"
   - Não há backup do firmware funcional conhecido

**KissOTA resolve isto:**
- ✅ Atualização global sem configurar nada (Telegram API)
- ✅ Segurança robusta (PIN/PUK + infraestrutura Telegram)
- ✅ Rollback multinível (backup + factory + boot loop detection)
- ✅ Não expõe portas ao exterior

**Quando usar cada um?**
- **AsyncElegantOTA:** Desenvolvimento local, prototipagem rápida, LAN privada
- **KissOTA:** Produção, dispositivos remotos globais, segurança crítica

---

### Funciona sem PSRAM?

**Resposta:** A versão atual do KissOTA **requer PSRAM** para o buffer de download e verificação de validade do arquivo.

**Razões técnicas:**
- Firmware típico: 1-1.5 MB
- Buffer PSRAM: 7-8 MB disponíveis
- RAM normal ESP32-S3: Só ~70-105 KB livres

**Alternativas se não tiver PSRAM:**
1. **Baixar para LittleFS primeiro:**
   - Mais lento (flash é mais lento que PSRAM)
   - Desgasta flash desnecessariamente
   - Requer espaço livre no LittleFS

2. **Usar AsyncElegantOTA:**
   - Não requer PSRAM
   - Download diretamente para partição OTA

3. **Contribuir um PR:**
   - Se implementar suporte para ESP32 sem PSRAM, PR bem-vindo

**Hardware recomendado:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 euros
- ESP32-S3-DevKitC-1 com módulo N16R8

---

### Posso usar KissOTA sem Telegram?

**Resposta:** Tecnicamente sim, mas requer trabalho de adaptação.

A arquitetura do KissOTA tem duas camadas:

1. **Core OTA (backend):**
   - Gestão de estados
   - Backup/rollback
   - Flash desde PSRAM
   - Validação CRC32
   - Boot flags
   - **Esta parte é independente do transporte**

2. **Frontend Telegram:**
   - Autenticação PIN/PUK
   - Download de arquivos desde Telegram API
   - Mensagens de progresso ao usuário
   - **Esta parte depende de KissTelegram**

**Para usar outro transporte (HTTP, MQTT, Serial, etc.):**

```cpp
// Precisaria implementar seu próprio frontend
class KissOTACustom : public KissOTACore {
public:
  // Implementar métodos abstratos:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**Vale a pena o esforço?**
- Se já tem KissTelegram: **Não, use KissOTA como está**
- Se não quer Telegram: **Provavelmente melhor usar AsyncElegantOTA**
- Se tem um caso de uso específico (ex: MQTT industrial): **KissTelegram tem uma versão para Empresas, pergunte**

---

### O que acontece se perder conexão WiFi durante o download?

**Resposta:** O sistema gerencia desconexões de forma segura.

**Cenários:**

**1. Download para PSRAM interrompido:**
```
Estado: DOWNLOADING → Timeout de rede
Ação automática:
  1. Detecta timeout (após 30 segundos sem dados)
  2. Limpa buffer PSRAM
  3. Tenta download (máximo 3 tentativas)
  4. Se 3 falhas: Cancela OTA, volta para IDLE
  5. Firmware atual NÃO tocado (seguro)
```

**2. Desconexão durante flash:**
```
Estado: FLASHING → WiFi cai
Resultado:
  - Flash continua (não depende de WiFi)
  - Dados já estão na PSRAM
  - Flash completa normalmente
  - Ao reiniciar, esperará /otaok (60s)
  - Se não há WiFi para enviar /otaok: Rollback automático
```

**3. Desconexão durante validação:**
```
Estado: VALIDATING → Sem WiFi
Resultado:
  - Timer de 60 segundos rodando
  - Se WiFi voltar antes de 60s: Pode enviar /otaok
  - Se passar 60s sem /otaok: Rollback automático
  - Sistema volta ao firmware anterior (seguro)
```

**Timeout global:** 7 minutos desde /otaconfirm até completar tudo. Se for excedido, OTA é cancelado automaticamente.

---

### É seguro gravar firmware menor que o atual?

**Resposta:** Sim, é completamente seguro.

**Explicação técnica:**

O processo de flash sempre:
1. **Apaga completamente a partição destino** antes de escrever
2. **Escreve o novo firmware** (seja do tamanho que for)
3. **Marca o tamanho real** nos metadados da partição

**Exemplo:**
```
Firmware atual:  1.5 MB
Firmware novo:   0.9 MB

Processo:
1. Backup de 1.5 MB → LittleFS
2. Apagar partição app (3.5 MB completos)
3. Escrever 0.9 MB novos
4. Metadados: size = 0.9 MB
5. Espaço restante na partição: Vazio/apagado
```

**Não há problema de "lixo residual"** porque:
- ESP32 só executa até o final do firmware marcado
- O resto da partição está apagado
- CRC32 é calculado só sobre o tamanho real

**Casos de uso comuns:**
- Desativar features para reduzir tamanho
- Firmware de emergência minimalista (~500 KB)
- Compilação otimizada com flags diferentes

---

### Como reseto o sistema se tudo falhar?

**Resposta:** Você tem 3 opções de recuperação.

#### Opção 1: Rollback Automático (Mais Comum)
Se o firmware novo falhar, simplesmente **não faça nada**:
```
1. Firmware novo inicializa mas falha
2. ESP32 reinicia (crash/watchdog)
3. Contador bootCount++ no NVS
4. Após 3 reinícios em 5 minutos → Rollback automático
5. Sistema restaura firmware anterior desde /ota_backup.bin
```

#### Opção 2: Rollback Manual
Se tiver acesso ao Telegram:
```
/otacancel
```
Força rollback imediato desde backup.

#### Opção 3: Boot em Factory (Emergência)
Se tudo mais falhar:

**Via Serial (requer acesso físico):**
```cpp
// No setup(), detecta condição de emergência
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin a GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Via esptool (último recurso):**
```bash
# Gravar factory partition com firmware original
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Opção 4: Flash Completo (Começar do Zero)
```bash
# Apaga todo o flash
esptool.py --port COM13 erase_flash

# Grava firmware fresco + partições
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prevenção:**
- ✅ Mantenha sempre um firmware factory estável
- ✅ Teste firmware novos em dispositivo de desenvolvimento primeiro
- ✅ Não modifique a partição factory em produção

---

### Quanto tempo demora uma atualização OTA completa?

**Resposta:** Aproximadamente 1-3 minutos total para firmware de ~1 MB se o WiFi for perfeito.

**Desglose temporal típico:**

| Fase | Duração Típica | Fatores |
|------|----------------|----------|
| **Autenticação** | 10-30 segundos | Tempo de usuário escrevendo PIN |
| **Upload para Telegram** | 5-15 segundos | Velocidade de internet do usuário |
| **Download para PSRAM** | 10-20 segundos | Velocidade WiFi do ESP32 + Telegram API |
| **Verificação CRC32** | 2-5 segundos | Tamanho do firmware |
| **Confirmação usuário** | 5-30 segundos | Tempo de reação do usuário |
| **Backup firmware atual** | 10-30 segundos | Velocidade de escrita LittleFS |
| **Flash novo firmware** | 5-10 segundos | Velocidade de escrita flash |
| **Reinício** | 5-10 segundos | Tempo de boot do ESP32 |
| **Validação /otaok** | 5-60 segundos | Tempo de reação do usuário |
| **TOTAL** | **~2-4 minutos** | Varia segundo condições |

**Fatores que afetam a velocidade:**

**Mais rápido:**
- ✅ WiFi forte (perto do roteador)
- ✅ Firmware pequeno (<1 MB)
- ✅ Usuário experiente (responde rápido)
- ✅ LittleFS não fragmentado. KissTelegram desfragmenta o FS a cada 3-5 minutos

**Mais lento:**
- ⚠️ WiFi fraco ou congestionado
- ⚠️ Firmware grande (>1.5 MB)
- ⚠️ Usuário hesitando em confirmar
- ⚠️ LittleFS muito cheio

**Timeout de segurança:** 7 minutos máximo desde /otaconfirm. Se for excedido, OTA é cancelado.

---

### Posso atualizar múltiplos ESP32 simultaneamente?

**Resposta:** Sim, mas cada ESP32 precisa de seu próprio bot do Telegram (diferente BOT_TOKEN).

**Limitação do Telegram:**
- Um bot Telegram só pode processar 1 conversação simultânea de forma confiável
- Telegram API tem rate limits (~30 mensagens/segundo por bot)

**Opção 1: Um Bot por Dispositivo (Recomendado para Produção)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Vantagens:**
- ✅ OTAs completamente independentes
- ✅ Sem conflitos de mensagens
- ✅ Cada dispositivo tem seu próprio chat

**Desvantagens:**
- ⚠️ Mais bots para gerenciar
- ⚠️ Mais tokens para configurar

---

**Opção 2: Um Bot, Múltiplos Dispositivos Sequencialmente**
```cpp
// Todos usam o mesmo BOT_TOKEN
// Mas atualize-os UM DE CADA VEZ manualmente
```

**Processo:**
1. Atualiza ESP32 #1 → Espera completar
2. Atualiza ESP32 #2 → Espera completar
3. etc.

**Vantagens:**
- ✅ Só um bot para gerenciar
- ✅ Mais simples para poucos dispositivos

**Desvantagens:**
- ⚠️ Um por vez (lento para muitos dispositivos)
- ⚠️ Fácil confundir dispositivo

---

**Opção 3: Gestão Centralizada (KissTelegram para Empresas)**
```
Sistema central com API própria
    ↓
Distribui firmware para múltiplos ESP32
    ↓
Cada ESP32 reporta progresso ao sistema central
    ↓
Sistema central notifica ao via Json API ou usuário via Telegram
```

**Características liked (Mas disponíveis no Kisstelegram para empresas):**
- Backend web/cloud
- Banco de dados de dispositivos
- Fila de trabalhos OTA
- Dashboard de monitoramento

**Contribuições bem-vindas** KissOTA tem muitas possibilidades, pergunte ou faça um PR.

---

## Changelog

### v0.9.0 (Atual)
- ✅ Sistema de versão simplificado
- ✅ Eliminada verificação de downgrade
- ✅ Limpeza de código obsoleto
- ✅ Melhorias em mensagens de log

### v0.1.0
- ✅ Implementação de backup/rollback automático
- ✅ Detecção de boot loops
- ✅ Integração completa com Telegram

### v0.0.2
- ✅ Refatoração completa para PSRAM
- ✅ Eliminação de partições OTA_0/OTA_1
- ✅ Sistema de validação de 60 segundos

### v0.0.1
- ✅ Versão inicial com OTA básico

---

**Documento atualizado:** 12/12/2025
**Versão do documento:** 0.9.0
**Autor** Vicente Soriano, victek@gmail.com**
**Licença MIT**
