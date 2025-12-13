# KissTelegram

![Licença](https://img.shields.io/badge/license-MIT-blue.svg)
![Plataforma](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Versão](https://img.shields.io/badge/version-1.x-green.svg)
![Idiomas](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Português** | [Documentação](GETTING_STARTED_PT.md) | [Benchmarks](BENCHMARK.md)

---

> 🚨 **PRIMEIRA VEZ USANDO ESP32-S3 COM KISSTELEGRAM?**
> **LEIA ISTO PRIMEIRO:** [**GETTING_STARTED_PT.md**](GETTING_STARTED_PT.md) ⚠️
> O ESP32-S3 requer um **processo de upload em duas etapas** devido às partições customizadas. Pular este guia causará erros de inicialização!

---

## Uma Biblioteca Robusta e de Nível Empresarial para Bots Telegram em ESP32-S3

KissTelegram é a **única biblioteca Telegram para ESP32** construída do zero para aplicações críticas. Diferentemente de outras bibliotecas que dependem da classe `String` do Arduino (causando fragmentação de memória e vazamentos), KissTelegram utiliza arrays puros `char[]` para estabilidade inabalável.

### Por que KissTelegram?

- Cansado de perder projetos por bibliotecas fracas, vazamentos de memória, soluções de última hora, falta de suporte, buzzwords, reinicializações....

- Minha visão, agora os fatos:

- **Sem Perda de Mensagens**: Fila persistente em LittleFS sobrevive a travamentos, reinicializações e falhas de WiFi
- **Sem Vazamentos de Memória**: Implementação pura em `char[]`, sem fragmentação de String
- **Segurança SSL/TLS**: Conexões seguras com a API Telegram com validação de certificado
- **Gerenciamento Inteligente de Energia**: 6 modos de potência (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Prioridades de Mensagem**: CRITICAL, HIGH, NORMAL, LOW com gerenciamento inteligente de fila
- **Modo Turbo**: Processamento em lote para filas de mensagens grandes (0,9 msg/s)
- **i18n Multilíngue**: Seleção de idioma em tempo de compilação (7 idiomas, zero overhead em tempo de execução)
- **OTA Empresarial**: Atualizações de firmware com dual-boot, rollback automático e gateway de segurança
- **Utilização de 100% Flash**: Esquema de partição customizado maximiza o flash de 16MB do ESP32-S3
- **Mais Seguro que OTA Espressif**: Autenticação PIN/PUK, verificação de checksum, janela de validação de 60s
- **Independente de bibliotecas externas**: Tudo feito do zero, parser JSON próprio.

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

## Esquema de Partição Customizado

KissTelegram inclui um `partitions.csv` otimizado que maximiza o uso de flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB de armazenamento SPIFFS** - são 8MB a mais que os esquemas padrão da Espressif!

Para usar este esquema de partição:
1. Copie `partitions.csv` para o diretório do seu projeto
2. Na Arduino IDE: Tools → Partition Scheme → Custom
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

### Exemplo de Atualizações OTA

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

**Processo de OTA:**
1. Envie `/ota` para seu bot
2. Digite o PIN com `/otapin YOUR_PIN`
3. Faça upload do arquivo `.bin` de firmware
4. Bot verifica checksum automaticamente
5. Confirme com `/otaconfirm`
6. Após reiniciar, valide com `/otaok` em até 60 segundos
7. Rollback automático se a validação falhar!

- Leia Readme_KissOTA.md em seu idioma preferido para saber mais sobre a solução.

---

## Recursos-Chave Explicados

### 1. Fila de Mensagens Persistente

Mensagens são armazenadas em LittleFS com exclusão automática em lote:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sobrevive a travamentos, desconexões de WiFi, reinicializações
- Automaticamente tenta novamente envios falhados
- Exclusão inteligente em lote (a cada 10 mensagens + quando fila vazia)
- Garantia de zero perda de mensagens

### 2. Gerenciamento de Energia

6 modos inteligentes de potência se adaptam às necessidades da sua aplicação:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase inicial de inicialização (10s)
- **POWER_LOW**: Atividade mínima, polling lento
- **POWER_IDLE**: Sem atividade recente, verificações reduzidas
- **POWER_ACTIVE**: Operação normal
- **POWER_TURBO**: Processamento em lote de alta velocidade (intervalos de 50ms)
- **POWER_MAINTENANCE**: Sobreposição manual para atualizações
- **Timing de decaimento para mudança suave**

### 3. Prioridades de Mensagem

Quatro níveis de prioridade garantem que mensagens críticas sejam enviadas primeiro:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

Fila processa: **CRITICAL → HIGH → NORMAL → LOW**
Processos internos: **OTAMODE → MAINTENANCEMODE**

### 4. Segurança SSL/TLS

Conexões seguras com validação de certificado:

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback automático seguro/inseguro
- Verificações de ping periódicas para manter a conexão
- Código de conexão reutilizável economiza cabeçalho de conexão para máxima taxa de transferência

### 5. Modo Turbo

Ativa automaticamente quando a fila > 100 mensagens:

```cpp
bot.enableTurboMode();  // Manual activation
```

- Processa 10 mensagens por ciclo
- Intervalos de 50ms entre lotes
- Atinge taxa de transferência de 15-20 msg/s
- Desativa automaticamente quando a fila é enviada

### 6. Modos de Operação

Perfis pré-configurados para diferentes cenários:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Padrão (polling: 10s, retry: 3)
- **MODE_PERFORMANCE**: Rápido (polling: 5s, retry: 2)
- **MODE_POWERSAVE**: Lento (polling: 30s, retry: 2)
- **MODE_RELIABILITY**: Robusto (polling: 15s, retry: 5)

### 7. Diagnósticos

Monitoramento e debug abrangentes:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Exibe:
- Memória livre (heap/PSRAM)
- Estatísticas de fila de mensagens
- Qualidade de conexão
- Histórico de modo de potência
- Uso de armazenamento
- Tempo de atividade

---

## 8. Gerenciamento de WiFi
- WiFi Manager integrado dispara início de outras tarefas até que WiFi seja estável
- Previne condições de corrida
- Reclama mensagens contínuas para ir para armazenamento FS até que a conexão seja restabelecida, pode manter até 3500 msg (padrão mas facilmente expansível)
- Monitoramento de qualidade de conexão (EXCELLENT, GOOD, FAIR, POOR, DEAD) e nível de saída RSSI
- Você apenas precisa se preocupar com seu sketch, use KissTelegram para o resto

---

## 9. Recurso Matador: Comando `/estado`

**A ferramenta de debug mais poderosa que você já usará:**

Envie `/estado` para seu bot e obtenha um **relatório de saúde completo** em segundos:

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

**Por que `/estado` é essencial:**
- ✅ Verificação instantânea de saúde do sistema
- ✅ Monitoramento de qualidade de WiFi (diagnostique problemas de conectividade)
- ✅ Detecção de vazamento de memória (observe heap livre)
- ✅ Status de fila de mensagens (veja mensagens pendentes/falhadas)
- ✅ Rastreamento de tempo de atividade (monitoramento de estabilidade)
- ✅ Disponível em 7 idiomas
- ✅ Sua primeira ferramenta ao depurar problemas

**Dica profissional:** Faça `/estado` sua primeira mensagem após cada atualização de firmware para verificar se tudo funciona!

---

## 10. NTP
- Código próprio para sincronizar/ressincronizar para SSL e Scheduler (Enterprise Edition)
---

## 11. Documentação (7 Idiomas)

- **[GETTING_STARTED_PT.md](GETTING_STARTED_PT.md)** - **COMECE AQUI!** Guia completo desde desempacotar o ESP32-S3 até a primeira mensagem no Telegram
- **[README_PT.md](README_PT.md)** (este arquivo) - Visão geral de recursos, início rápido, referência de API
- **[BENCHMARK.md](BENCHMARK.md)** - Comparação técnica vs 6 outras bibliotecas Telegram (apenas em inglês)
- **[README_KissOTA_XX.md](README_KissOTA_PT.md)** - Documentação do sistema de atualização OTA (7 idiomas: EN, ES, FR, IT, DE, PT, CN)

**Escolha seu idioma:** Todas as mensagens do sistema KissTelegram suportam 7 idiomas via seleção em tempo de compilação.


## Vantagens de Segurança do OTA

KissTelegram OTA é **mais seguro que a implementação Espressif**:

| Recurso | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Autenticação | PIN + PUK | Nenhuma |
| Verificação de Checksum | CRC32 automático | Manual |
| Backup & Rollback | Automático | Manual |
| Janela de Validação | 60s com `/otaok` | Nenhuma |
| Detecção de Loop de Boot | Sim | Não |
| Integração Telegram | Nativa | Requer código customizado |
| Otimização de Flash | 13MB SPIFFS | 5MB SPIFFS |

---

## Referência de API

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

Veja os arquivos .ino inclusos na biblioteca para explorar alguns cenários e recursos e meu estilo de código do KissTelegram. Melhor ainda, descomente seu idioma em [lang.h] para receber mensagens dos construtores principais (.cpp) em seu idioma local, se todos os idiomas estiverem comentados você obtém mensagens em espanhol, o idioma padrão:

As convenções de código estão em inglês, mas os pensamentos e comentários estão em meu idioma nativo, use seu tradutor online, código é fácil, por trás do código há muito mais complicado ...

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
## Configurações básicas
- Renomeie system_setup_template.h para system_setup.h na sua pasta KissTelegram para iniciar a compilação.
- Substitua as seguintes linhas por suas configurações.

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

## Arquitetura, Visão, Conceito, Soluções && Design (e o responsável por qualquer mau funcionamento)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Contribuidores**
- Muitos assistentes IA em Traduções, Código, Troubleshooting e piadas.

---


## Contribuindo

Contribuições são bem-vindas! Por favor, sinta-se livre para enviar um Pull Request.

---

## Suporte

Se você achar esta biblioteca útil, por favor considere:
- Dar uma estrela neste repositório
- Reportar bugs via GitHub Issues
- Compartilhar seus projetos usando KissTelegram

---
