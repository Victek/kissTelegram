# Iniciando com KissTelegram no ESP32-S3

**Um guia abrangente para configurar seu ESP32-S3 do zero até a primeira mensagem no Telegram**

> ⚠️ **CRÍTICO**: Leia este guia completamente antes de fazer upload de qualquer firmware. O ESP32-S3 N16R8 requer um **processo de upload em duas etapas** devido a partições personalizadas. Pular etapas causará erros!

---

## Sumário

1. [Antes de Começar](#antes-de-começar)
2. [Criando Seu Bot Telegram](#criando-seu-bot-telegram)
3. [Configuração de Hardware](#configuração-de-hardware)
4. [Configuração do Arduino IDE](#configuração-do-arduino-ide)
5. [Primeiro Upload (Problemas Comuns)](#primeiro-upload-problemas-comuns)
6. [Arquivos de Configuração](#arquivos-de-configuração)
7. [Sucesso! O que vem depois?](#sucesso-o-que-vem-depois)

---

## Antes de Começar

### O Que Você Precisa

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Dois cabos USB-C** (para alternar entre as portas bootloader e OTG)
- **Arduino IDE 2.x** ou mais recente
- **PC Windows** (este guia é focado em Windows, adapte os caminhos para Linux/Mac)
- **Conta Telegram** no seu telefone

### O Que Torna Isso Diferente

Seu novo ESP32-S3 N16R8 vem com um programa de LED RGB de demonstração. KissTelegram **substituirá completamente a tabela de partições** para maximizar seu flash de 16MB:

| Partição | Padrão Espressif | KissTelegram Personalizado |
|-----------|-------------------|---------------------|
| Espaço da App | 1,5 MB | 4,5 MB (3x maior!) |
| Sistema de Arquivos | 5 MB | 13 MB (2,6x maior!) |
| Total Usado | 6,5 MB | 17,5 MB |

É por isso que o processo de dois uploads é necessário: **a tabela de partições muda entre os uploads**.

---

## Criando Seu Bot Telegram

### Passo 1: Converse com BotFather

1. Abra o Telegram no seu telefone
2. Procure por `@BotFather` (bot oficial, tem marca de verificação azul)
3. Inicie uma conversa com `/start`
4. Crie seu bot com `/newbot`
5. Escolha um nome (exemplo: "Meu Assistente Doméstico")
6. Escolha um nome de usuário (deve terminar em `bot`, exemplo: "meu_assistente_bot")
7. **Salve seu Token do Bot** - parece: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Passo 2: Obtenha Seu ID de Chat

**Método 1: Usando um Bot (Mais Fácil)**

1. Procure por `@userinfobot` no Telegram
2. Inicie conversa com `/start`
3. Ele responderá com seu **ID de Chat** (um número como `123456789`)
4. **Salve este número** - você precisará dele na configuração

**Método 2: Usando Navegador Web**

1. Envie qualquer mensagem para seu bot recém-criado
2. Abra um navegador e visite:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   (Substitua `<YOUR_BOT_TOKEN>` pelo seu token real)
3. Procure por `"chat":{"id":123456789` na resposta JSON
4. Esse número é seu **ID de Chat**

**✅ Agora você tem:**
- Token do Bot: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- ID de Chat: `123456789`

Guarde-os com segurança! Você precisará deles em breve.

---

## Configuração de Hardware

### Entendendo as Duas Portas USB-C

Seu ESP32-S3 N16R8 tem **duas portas USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED de Energia    │
│  └─┘                 │
│  [USB-C]  ← PORTA DIREITA (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← PORTA ESQUERDA (OTG/Operação Normal)
│                      │
└─────────────────────┘
```

**PORTA DIREITA (perto do LED de Energia):**
- Usada para **upload inicial do firmware**
- Usada para **modo bootloader**
- Use quando Arduino IDE disser "Conectando..."

**PORTA ESQUERDA (OTG):**
- Usada para **operação normal** após o primeiro upload
- Usada para **segundo upload** (correção de partição)
- Use para Serial Monitor em operação normal

---

## Configuração do Arduino IDE

### Passo 1: Mostrar Arquivos Ocultos (Windows)

1. Abra **Explorador de Arquivos**
2. Clique na aba **Exibir** → **Mostrar** → Marque:
   - ✅ Extensões de nome de arquivo
   - ✅ Itens ocultos
3. Na aba **Filtro**: **Todos os tipos de arquivo**

### Passo 2: Modificar boards.txt

1. Navegue para:
   ```
   C:\Users\<SEU_NOME_DE_USUÁRIO>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Substitua `3.3.4` pela sua versão do núcleo ESP32 se diferente)

2. Encontre e abra `boards.txt` (use Notepad++ ou qualquer editor de texto)

3. Pressione `Ctrl+F` e procure por:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Imediatamente abaixo dessa linha**, cole estas três linhas:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Salve** e feche `boards.txt`

6. Se Arduino IDE estava aberto, **feche e reinicie-o**

### Passo 3: Configurar Arduino IDE

1. **Abra** sua pasta do sketch KissTelegram (com `.ino`, `.h`, `.cpp`, e `partitions.csv`)

2. No Arduino IDE, vá para **Ferramentas** → **Placa** → **4D Systems gen4-ESP32-S3R8n16**

3. **Ferramentas** → **Recarregar Dados da Placa** (você verá uma confirmação na parte inferior da tela)

4. **Configure todas as opções do menu Ferramentas:**

   | Configuração | Valor |
   |---------|-------|
   | **Placa** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Ativado |
   | **Tamanho do Flash** | 16MB (128Mb) |
   | **Esquema de Partição** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Velocidade de Upload** | 921600 |
   | **Apagar Todo o Flash Antes do Upload do Sketch** | **Ativado** ⚠️ |

   ⚠️ **Configurações críticas** - verifique estas duas vezes!

5. **Ferramentas** → **Serial Monitor** → Defina taxa de baud para **115200**

---

## Primeiro Upload (Problemas Comuns)

### Por Que Dois Uploads São Necessários

**O Problema:**
- Primeiro upload: Arduino usa **tabela de partições antiga** para escrever o firmware
- ESP32 inicia: Encontra **tabela de partições nova** (do `partitions.csv`)
- **Incompatibilidade** entre onde o firmware foi escrito vs onde ESP32 procura por ele
- Resultado: Erros de boot, erros de partição, travamentos

**A Solução:**
Dois uploads garantem que o firmware seja escrito no **local correto** definido pela nova tabela de partições.

---

### Upload #1: Flash Inicial (Espere Erros!)

1. **Conecte a porta USB-C DIREITA** (perto do LED de Energia) ao seu PC

2. **Selecione a porta**: Ferramentas → Porta → Selecione a porta COM que aparece

3. **Verifique as configurações**:
   - ✅ Apagar Todo o Flash Antes do Upload do Sketch: **Ativado**
   - ✅ Esquema de Partição: **Custom (4MB APP/12MB LtlFS)**

4. **Pressione Upload** (`Ctrl+U` ou botão ➡️)

5. **Aguarde ~2-3 minutos** (apagando + fazendo upload)

6. **Resultado esperado**:
   ```
   ✅ Upload bem-sucedido
   ```

7. **Abra Serial Monitor** - você verá erros como:
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **Se você configurou `system_setup.h` corretamente**: Você pode receber a **primeira mensagem no Telegram** (mas Serial mostrará erros)

**Não se desespere!** Esses erros são **esperados** e **normais**. Continue para o Upload #2.

---

### Upload #2: Correção de Partição (Erros Desaparecem)

1. **Desconecte a porta USB-C DIREITA**

2. **Conecte a porta USB-C ESQUERDA** (porta OTG) ao seu PC

3. **Selecione a nova porta**: Ferramentas → Porta → Selecione a nova porta COM
   - **Importante**: O número da porta mudará! Procure no Serial Monitor por dados para confirmar a porta correta.

4. **Verifique as configurações novamente**:
   - ✅ Apagar Todo o Flash Antes do Upload do Sketch: **Ativado**
   - ✅ Esquema de Partição: **Custom (4MB APP/12MB LtlFS)**

5. **Pressione Upload novamente** (`Ctrl+U`)

6. **Aguarde ~2-3 minutos** (apagando + fazendo upload)

7. **Abra Serial Monitor** - você deve ver agora:
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi conectado
   ✅ Bot Telegram ativado
   ✅ Sistema pronto
   ```

8. **Verifique Telegram** - você receberá a mensagem de boas-vindas:
   ```
   📦 Olá! KissTelegram está pronto.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Sinal WiFi: -59 dBm (Bom)
   ✅ 0 mensagens na fila
   ```

**Sucesso!** Seu ESP32-S3 agora está executando KissTelegram com partições corretas.

---

### Uploads Futuros

**Boas notícias:** Após os dois uploads iniciais, todos os uploads futuros funcionam normalmente:

- Use **porta USB-C ESQUERDA** (OTG)
- **Não precisa** "Apagar Todo o Flash" mais (a menos que queira começar do zero)
- Faça upload uma vez e funcionará imediatamente

---

## Arquivos de Configuração

### system_setup.h (Obrigatório Antes do Primeiro Upload!)

**Antes de compilar:**

1. Navegue até sua pasta KissTelegram
2. Encontre `system_setup_template.h`
3. **Renomeie** para `system_setup.h`
4. **Abra** `system_setup.h` e preencha:

```cpp
// Seu Bot Telegram (do BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Seu ID de Chat (do @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Suas credenciais WiFi
#define KISS_FALLBACK_WIFI_SSID "SeuNomeWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "SuaSenhaWiFi"

// Segurança OTA (mude o PIN/PUK padrão!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 dígitos
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 dígitos
```

5. **Salve** o arquivo

**⚠️ Aviso de Segurança:** Mude o PIN padrão (`0000`) e PUK (`00000000`) para seus próprios segredos!

---

### lang.h (Opcional: Escolha Seu Idioma)

KissTelegram suporta 7 idiomas para mensagens do sistema:

```cpp
// Em lang.h, descomente UM idioma:

// #define LANG_CN  // 中文 (Chinês)
// #define LANG_DE  // Deutsch (Alemão)
// #define LANG_EN  // English (Inglês)
// #define LANG_FR  // Français (Francês)
// #define LANG_IT  // Italiano (Italiano)
// #define LANG_PT  // Português (Português)
// #define LANG_ES  // Español (Espanhol) - PADRÃO se todos comentados
```

Escolha seu idioma **antes de compilar** para mensagens localizadas.

---

## Sucesso! O que vem depois?

### Verifique se Tudo Funciona

1. **Envie `/estado` para seu bot** no Telegram - você receberá um relatório de status detalhado:
   ```
   📦 KissTelegram v1.x.x
   🎯 CONFIABILIDADE DO SISTEMA
   ✅ Sistema: CONFIÁVEL
   ✅ Mensagens enviadas: 2
   💾 Mensagens pendentes: 0
   📡 Sinal WiFi: -59 dBm (Bom)
   🔋 Tempo de atividade: 123 segundos
   💾 Memória livre: 223 KB
   ```

2. **Verifique Serial Monitor** - não deve mostrar erros

3. **Teste comandos**:
   - `/start` - Mensagem de boas-vindas
   - `/help` - Comandos disponíveis
   - `/estado` - Status do sistema (verificação de saúde)

---

### Entendendo Atualizações OTA

Uma vez que KissTelegram está executando, você pode atualizar o firmware **via Telegram** (sem cabo USB!):

1. Envie `/ota` para seu bot
2. Digite o PIN: `/otapin 0000` (ou seu PIN personalizado)
3. **Envie seu arquivo de firmware `.bin`** (arraste e solte no Telegram)
4. Bot verifica checksum automaticamente
5. Confirme: `/otaconfirm`
6. ESP32 reinicia com novo firmware
7. **Dentro de 60 segundos**, envie `/otaok` para confirmar que funciona
8. Se você não confirmar, ESP32 **automaticamente volta** para o firmware anterior!

📖 **Leia mais:** Veja `README_KissOTA_PT.md` (ou seu idioma) para documentação OTA completa.

---

### Explore o Código de Exemplo

O exemplo `suite_kiss.ino` demonstra:

- ✅ Gerenciamento WiFi com monitoramento de qualidade
- ✅ Fila de mensagens com prioridades
- ✅ Modos de gerenciamento de energia
- ✅ Tratamento de comandos (`/start`, `/help`, `/estado`, etc.)
- ✅ Atualizações OTA via Telegram
- ✅ Recuperação de crash e persistência
- ✅ Conexões SSL/TLS seguras

**Dica profissional:** Use o comando `/estado` como sua **ferramenta de monitoramento de saúde** - é sua janela para os internals do KissTelegram!

---

### Troubleshooting Comum

**Problema: "Port not found" ou "Access denied"**
- Windows bloqueou a porta. Desconecte USB, aguarde 5s, reconecte.
- Tente um cabo USB diferente (alguns são apenas para carga, não para dados)

**Problema: "Timeout waiting for device" durante upload**
- Porta USB errada! Lembre-se: porta DIREITA para primeiro upload, porta ESQUERDA para segundo
- Segure o botão BOOT no ESP32 enquanto clica Upload, solte após "Conectando..." aparecer

**Problema: Serial Monitor mostra caracteres aleatórios**
- Taxa de baud errada. Defina para **115200** no dropdown do Serial Monitor

**Problema: Bot não responde no Telegram**
- Verifique se `system_setup.h` tem o Token do Bot e ID de Chat corretos
- Verifique se as credenciais WiFi estão corretas
- Abra Serial Monitor e procure por mensagens de conexão WiFi

**Problema: Erro de compilação "Partition table does not fit"**
- Você não adicionou a partição personalizada ao `boards.txt` corretamente
- Ou não selecionou "Custom (4MB APP/12MB LtlFS)" em Ferramentas → Esquema de Partição

---

### Obtenha Mais Ajuda

- 📧 **Email**: victek@gmail.com
- 📖 **Documentação**: Veja todos os arquivos `README_*.md` em sua pasta KissTelegram
- 🐛 **Relatórios de Bug**: Issues no GitHub (link no README.md principal)
- 💡 **Solicitações de Funcionalidades**: Também bem-vindas via email ou GitHub!

---

## Resumo: O Processo Completo

```
1. Obtenha Token do Bot + ID de Chat do Telegram ✅
2. Modifique boards.txt (adicione partição personalizada) ✅
3. Configure Arduino IDE (partição personalizada, Erase ativado) ✅
4. Edite system_setup.h (credenciais) ✅
5. Conecte porta USB DIREITA ✅
6. Upload #1 (espere erros) ✅
7. Desconecte DIREITA, conecte porta USB ESQUERDA ✅
8. Upload #2 (erros desaparecidos!) ✅
9. Receba mensagem de boas-vindas no Telegram ✅
10. Envie /estado para verificar se tudo funciona ✅
```

**Você está pronto para construir projetos incríveis com KissTelegram!** 🎉

---

**Feliz codificação!**

*Vicente Soriano - victek@gmail.com*
