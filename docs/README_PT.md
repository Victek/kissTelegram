# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Português** | [Documentação](docs/GETTING_STARTED_PT.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **PRIMEIRA VEZ USANDO ESP32-S3 COM KISSTELEGRAM?**
> **LEIA ISTO PRIMEIRO:** [**GETTING_STARTED_PT.md**](docs/GETTING_STARTED_PT.md)
> ESP32-S3 requer um **processo de upload em duas etapas** devido a partições personalizadas. Ignorar este guia causará erros de inicialização e partições erradas!

---

## Uma Biblioteca Robusta de Nível Empresarial para Bots Telegram no ESP32-S3

KissTelegram é a **única biblioteca Telegram para ESP32** construída do zero para aplicações críticas. Ao contrário de outras bibliotecas que dependem da classe `String` do Arduino (causando fragmentação de memória e vazamentos), KissTelegram usa arrays puros `char[]` para estabilidade inabalável.

### Por que KissTelegram?

- Cansado de projetos perdidos devido a bibliotecas fracas, vazamentos de memória, soluções de última hora, falta de suporte, palavras vazias, termos que não funcionam, reinícios....

- Esta era minha visão e experiência com outras bibliotecas e este é o resultado com KissTelegram:

- **Zero Perda de Mensagens**: Fila persistente em LittleFS que sobrevive a crashes, reinicializações e falhas de WiFi
- **Sem Vazamentos de Memória**: Implementação pura `char[]`, sem fragmentação de String
- **Segurança SSL/TLS**: Conexões seguras à API do Telegram com validação de certificado (até 2035)
- **Gerenciamento Inteligente de Energia**: 6 modos de energia (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Prioridades de Mensagens**: CRITICAL, HIGH, NORMAL, LOW com gerenciamento inteligente de fila
- **Modo Turbo**: Processamento em lote para grandes filas de mensagens (0,9 msg/s)
- **i18n Multilíngue**: Seleção de idioma em tempo de compilação para envio de mensagens (7 idiomas, zero overhead em tempo de execução)
- **OTA Empresarial**: Atualizações de firmware dual-boot com rollback automático e gerenciamento de segurança
- **100% Utilização de Flash**: Esquema de partição personalizado maximizando os 16MB flash do ESP32-S3
- **Mais Seguro que OTA da Espressif**: Autenticação PIN/PUK, verificação de checksum, janela de validação de 60s
- **Independente de bibliotecas externas**: Tudo construído do zero, parser JSON próprio para bibliotecas KissTelegram.

---

## Requisitos de Hardware

- **ESP32-S3** com **16MB Flash** / **8MB PSRAM**
- Conectividade WiFi
- Arduino IDE ou PlatformIO

---

## Instalação

### Arduino IDE

1. Baixe este repositório como ZIP
2. Abra Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Selecione o arquivo baixado

### PlatformIO

Adicione ao seu `platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Esquema de Partição Personalizado

KissTelegram inclui um `partitions.csv` otimizado que maximiza o uso de flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB de armazenamento SPIFFS** - São 8MB a mais do que os esquemas padrão da Espressif!

Para usar este esquema de partição:
1. Copie `partitions.csv` para seu diretório de projeto
2. No Arduino IDE: Tools ->Partition Scheme ->Custom
3. No PlatformIO: `board_build.partitions = partitions.csv`

---

## Início Rápido

### Exemplo Básico

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

### Exemplo de Atualização OTA

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

**Processo OTA:**
1. Envie `/ota` para seu bot
2. Digite o PIN com `/otapin YOUR_PIN`
3. Faça upload do arquivo firmware `.bin`
4. O bot verifica o checksum automaticamente
5. Confirme com `/otaconfirm`
6. Após reinicializar, valide com `/otaok` dentro de 60 segundos
7. Rollback automático se a validação falhar!

- Leia Readme_KissOTA.md no seu idioma preferido para saber mais sobre a solução.

---

## Recursos Principais Explicados

### 1. Fila de Mensagens Persistente

Mensagens são armazenadas em LittleFS com exclusão automática em lote:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sobrevive a crashes, desconexões WiFi, reinicializações
- Retentativa automática de envios falhados
- Exclusão em lote inteligente (a cada 10 mensagens + quando a fila está vazia)
- Garantia de zero perda de mensagens

### 2. Gerenciamento de Energia

6 modos de energia inteligentes adaptam-se às necessidades da sua aplicação:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase de inicialização inicial (10s)
- **POWER_LOW**: Atividade mínima, polling lento
- **POWER_IDLE**: Sem atividade recente, verificações reduzidas
- **POWER_ACTIVE**: Operação normal
- **POWER_TURBO**: Processamento em lote de alta velocidade (intervalos de 50ms)
- **POWER_MAINTENANCE**: Override manual para atualizações
- **Timing de decaimento para transições suaves**

### 3. Prioridades de Mensagens

Quatro níveis de prioridade asseguram que mensagens críticas sejam enviadas primeiro, pulando as de prioridade inferior:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

A fila processa: **CRITICAL /HIGH /NORMAL /LOW**
Processos internos: **OTAMODE /MAINTENANCEMODE**

### 4. Segurança SSL/TLS

Conexões seguras com validação de certificado (até 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback automático entre seguro e inseguro
- Verificações de ping periódicas para manter a conexão
- Código de conexão reutilizável economiza overhead de conexão para máximo desempenho

### 5. Modo Turbo

Ativado automaticamente ao enviar lotes usando o comando /llenar:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Processa 10 mensagens por ciclo
- Intervalos de 50ms entre lotes
- Atinge throughput de 0,9 msg/s
- Desativa automaticamente quando a fila é enviada

### 6. Modos de Operação

Perfis pré-configurados para diferentes cenários:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Padrão (polling: 10s, retentativa: 3)
- **MODE_PERFORMANCE**: Rápido (polling: 5s, retentativa: 2)
- **MODE_POWERSAVE**: Lento (polling: 30s, retentativa: 2)
- **MODE_RELIABILITY**: Robusto (polling: 15s, retentativa: 5)

### 7. Diagnósticos

Monitoramento e depuração completos:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Mostra:
- Memória livre (heap/PSRAM)
- Estatísticas da fila de mensagens
- Qualidade de conexão
- Histórico de modo de energia
- Uso de armazenamento
- Tempo de atividade

---

## 8. Gerenciamento WiFi
- Gerenciador WiFi integrado ativa outras tarefas apenas quando WiFi está estável
- Previne condições de corrida
- Recupera mensagens em andamento para armazenamento FS até que a conexão seja restaurada, pode conter até 3500 msg (padrão mas facilmente expansível, depende de quanto espaço você quer usar)
- Monitoramento de qualidade de conexão (EXCELLENT, GOOD, FAIR, POOR, DEAD) e nível de saída RSSI
- Você só precisa se preocupar com seu sketch, adicione seu código e KissTelegram cuida das tarefas críticas, WiFi, SSL, Mensagens, OTA, Gerenciamento de Energia, Prioridades.. todo o trabalho que você economiza....

---

## 9. Recurso Chave: Comando `/estado`

**A ferramenta de depuração mais poderosa que você incluirá no seu sketch**

Envie `/estado` para seu bot e obtenha um **relatório completo de saúde do seu sketch** naquele momento, (disponível em 7 idiomas):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ CONFIABILIDADE DO SISTEMA
✓ Sistema: CONFIÁVEL
✓ Mensagens enviadas: 940
📨 Mensagens pendentes: 70
✓ Mensagens perdidas: 0
🗑️ Descartes (fila cheia): 0

⚠️ ADVERSIDADES EXTERNAS
⚠️ Erros totais: 0
🔄 Recuperados (fallback): 0
📡 Quedas WiFi: 0

📊 INFORMAÇÕES TÉCNICAS
⏱️ Tempo de atividade: 0h 0m
🧠 RAM livre: 223960 bytes
💾 PSRAM livre: 1027820 bytes
💽 FS livre: 13549568 bytes
📦 Máx. em FS: 3500 Mensagens
⚡ Modo Energia: 3 
📶 Sinal WiFi: -64 dBm (Regular)
🔒 SSL: SEGURO
🚀 Turbo: INATIVO
🤖 Auto-mensagens: SIM

```

**Por que `/estado` é essencial:**
- Verificação instantânea de saúde do sistema
- Monitoramento de qualidade WiFi (diagnostica problemas de conectividade)
- Detecção de vazamentos de memória (observe o heap livre)
- Status da fila de mensagens (veja mensagens pendentes/falhadas)
- Rastreamento de tempo de atividade (monitoramento de estabilidade)
- Sua primeira ferramenta de diagnóstico

**Dica profissional:** Faça de `/estado` sua primeira mensagem após cada atualização de firmware para verificar que tudo funciona!

---

## 10. NTP
- Código próprio para sincronizar/ressincronizar para SSL. GNSS, LTE e Scheduler (Edição Enterprise)
---

## 11. Documentação (7 Idiomas)

- **[GETTING_STARTED_PT.md](docs/GETTING_STARTED_PT.md)** - **COMECE AQUI!** Guia completo desde receber o ESP32-S3 até a primeira mensagem enviada para Telegram
- **[README_PT.md](docs/README_PT.md)** (este arquivo) - Visão geral de recursos, início rápido, referência de API
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Comparação técnica com 6 bibliotecas Telegram (apenas inglês, mas auto-explicativo)
- **[README_KissOTA_XX.md](docs/README_KissOTA_PT.md)** - Grande valor pois detalha os passos do sistema de atualização OTA (7 idiomas: EN, ES, FR, IT, DE, PT, CN)

**Escolha seu idioma:** Todas as mensagens do construtor enviadas para Telegram são exibidas em 7 idiomas via seleção de idioma durante a compilação (lang.h).


## Vantagens de Segurança OTA

KissTelegram OTA é **muito mais seguro que a arquitetura da Espressif e economiza espaço no seu ESP32S3**:

| Recurso | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Autenticação | PIN + PUK | Nenhuma |
| Confirmação de Checksum | CRC32 automático | Manual |
| Backup e Rollback | Automático | Manual |
| Janela de Validação | 60s com `/otaok` | Nenhuma |
| Detecção de Loop de Boot | Sim | Não |
| Integração Telegram | Nativa | Requer código personalizado |
| Otimização de Flash | 13MB SPIFFS | 5MB SPIFFS |

---

## Referência da API

### Inicialização

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Mensagens

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configuração

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Monitoramento

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### Armazenamento

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Exemplos

Consulte o .ino incluído na biblioteca para explorar alguns cenários e recursos e meu estilo de codificação no KissTelegram. Melhor ainda, descomente seu idioma em [lang.h] para receber mensagens dos construtores principais (.cpp) no seu idioma local, se todos os idiomas estiverem comentados as mensagens são em espanhol, o idioma padrão:

As convenções de código estão em inglês, mas os pensamentos e comentários estão em espanhol, meu idioma nativo, use seu tradutor online, o código é fácil, dentro do código está minha visão e o conceito de KissTelegram...

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
## Configuração Básica
- Renomeie system_setup_template.h para system_setup.h na sua pasta KissTelegram para começar a compilação.
- Substitua as seguintes linhas pelas suas credenciais.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

## Arquitetura, Visão, Conceito, Soluções e Design (e responsável por qualquer mau funcionamento, sou eu...)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Contribuidores**
- Muitos assistentes de IA em Traduções, Código, Solução de problemas e muitas horas tentando impedi-los de reinventar a roda.....

---


## Contribuindo

Contribuições são bem-vindas! Por favor, sinta-se livre para enviar um Pull Request ou me enviar um email, mas prefiro um PR para que outros possam encontrar sua pergunta.

---

## Suporte

Se você achar esta biblioteca útil, por favor considere:
- Dar uma estrela a este repositório
- Reportar bugs através do GitHub Issues
- Compartilhar seus projetos usando KissTelegram
- Comentar com seus conhecidos sobre os recursos e soluções desta biblioteca
- Propor casos de uso e experiências com KissTelegram

---
