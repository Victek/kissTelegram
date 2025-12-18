# Primeiros Passos com KissTelegram no ESP32-S3

**Guia completo para configurar seu ESP32-S3 do zero até a primeira mensagem do Telegram**

> ⚠️ **CRÍTICO**: Leia este guia completamente antes de carregar qualquer firmware. O ESP32-S3 N16R8 requer um **processo de upload em duas etapas** devido às partições personalizadas. Pular etapas causará erros!

---

## Índice

1. [Antes de Começar](#antes-de-começar)
2. [Criar seu Bot do Telegram](#criar-seu-bot-do-telegram)
3. [Configuração de Hardware](#configuração-de-hardware)
4. [Configuração do Arduino IDE](#configuração-do-arduino-ide)
5. [Primeiro Upload (Criar partições com esptool)](#primeiro-upload)
6. [Arquivos de Configuração](#arquivos-de-configuração)
7. [Sucesso! E Agora?](#sucesso-e-agora)

---

## Antes de Começar

### O Que Você Precisa

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Dois cabos USB-C** (para alternar entre portas bootloader e OTG)
- **Arduino IDE 2.x** ou superior
- **PC Windows** (este guia é focado em Windows, adapte os caminhos para Linux/Mac)
- **Conta do Telegram** no seu telefone

### O Que Torna Isto Diferente

Seu novo ESP32-S3 N16R8 vem com um aplicativo demo de LED RGB integrado. O KissTelegram **substitui completamente a tabela de partições** para maximizar seus 16MB de flash:

| Partição | Padrão Espressif | KissTelegram Personalizado |
|----------|------------------|----------------------------|
| Espaço App | 1.5 MB | 4.5 MB (3x maior!) |
| Sistema de Arquivos | 5 MB | 13 MB (2.6x maior!) |
| Total Usado | 6.5 MB | 17.5 MB |

É por isso que o processo de dois uploads é necessário: **a tabela de partições muda entre uploads**.

---

## Criar seu Bot do Telegram

### Passo 1: Falar com o BotFather

1. Abra o Telegram no seu telefone
2. Pesquise por `@BotFather` (bot oficial, tem marca de verificação azul)
3. Inicie conversa com `/start`
4. Crie seu bot com `/newbot`
5. Escolha um nome (exemplo: "Meu Assistente Doméstico")
6. Escolha um nome de usuário (deve terminar com `bot`, exemplo: "meuassistente_domestico_bot")
7. **Salve seu Bot Token** - parece com: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Passo 2: Obter seu Chat ID

**Método 1: Usando um Bot (Mais Fácil)**

1. Pesquise por `@ChatIDHelperBot` no Telegram
2. Inicie conversa com `/start`
3. Ele responderá com seu **Chat ID** (um número como `123456789`)
4. **Salve este número** - você precisará dele na configuração

**Método 2: Usando Navegador Web**

1. Envie qualquer mensagem ao seu bot recém-criado
2. Abra o navegador e visite:
   ```
   https://api.telegram.org/bot<SEU_BOT_TOKEN>/getUpdates
   ```
   (Substitua `<SEU_BOT_TOKEN>` pelo seu token real)
3. Procure por `"chat":{"id":123456789` na resposta JSON
4. Esse número é seu **Chat ID**

**✅ Você agora tem:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

Guarde-os com segurança! Você precisará deles em breve.

---

## Configuração de Hardware

### Entendendo as Duas Portas USB-C

Seu ESP32-S3 N16R8 tem **duas portas USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED Power    │
│  └─┘                 │
│  [USB-C]  ← PORTA DIREITA (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← PORTA ESQUERDA (OTG/Operação Normal)
│                      │
└─────────────────────┘
```

**PORTA DIREITA (perto do LED Power):**
- Usada para **upload inicial do firmware**
- Usada para **modo bootloader**
- Use esta quando Arduino IDE disser "Connecting..."

**PORTA ESQUERDA (OTG):**
- Usada para **operação normal** após o primeiro upload
- Usada para **segundo upload** (correção de partição)
- Use esta para Monitor Serial em operação normal

---

## Configuração do Arduino IDE

### Passo 1: Mostrar Arquivos Ocultos (Windows)

1. Abra o **Explorador de Arquivos**
2. Clique na aba **Exibir** → **Mostrar** → Marque:
   - ✅ Extensões de nomes de arquivos
   - ✅ Itens ocultos
3. Na aba **Filtro**: **Todos os tipos de arquivos**

### Passo 2: Modificar boards.txt

1. Navegue até:
   ```
   C:\Users\<SEU_NOME_USUARIO>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Substitua `3.3.4` pela sua versão do core ESP32 se diferente)

2. Encontre e abra `boards.txt` (use Notepad++ ou qualquer editor de texto)

3. Pressione `Ctrl+F` e pesquise por:
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

6. Se Arduino IDE estava aberto, **feche e reinicie**

### Passo 3: Configurar Arduino IDE

1. **Abra** sua pasta de sketch KissTelegram (com `.ino`, `.h`, `.cpp`, e `partitions.csv`)

2. No Arduino IDE, vá para **Ferramentas** → **Placa** → **4D Systems gen4-ESP32-S3R8n16**

3. **Ferramentas** → **Recarregar Dados da Placa** (você verá confirmação na parte inferior)

4. **Configure todas as opções do menu Ferramentas:**

   | Configuração | Valor |
   |--------------|-------|
   | **Placa** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Configurações críticas** - verifique duas vezes!

5. **Ferramentas** → **Monitor Serial** → Configure a velocidade para **115200**

---

## Primeiro Upload (Problemas Comuns)

### Por Que Dois Uploads São Necessários

**O Problema:**
- Primeiro upload: Arduino usa a **tabela de partições antiga** para escrever o firmware
- ESP32 inicializa: Encontra a **nova tabela de partições** (de `partitions.csv`)
- **Incompatibilidade** entre onde o firmware foi escrito vs onde o ESP32 procura por ele
- Resultado: Erros de boot, erros de partição, crashes

**A Solução:**
Dois uploads garantem que o firmware seja escrito na **localização correta** definida pela nova tabela de partições.

---

### Upload #1: Flash Inicial (Gravar Bootloader)

1. **Conecte a porta USB-C DIREITA** (perto do LED Power) ao seu PC

2. **Selecione a porta**: Ferramentas → Porta → Selecione a porta COM que aparece

3. **Verifique as configurações**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**
   

4. **Ferramentas, Gravar Bootloader** (Clique nesta opção)
   - ✅ Ferramentas ➡️, no final do menu suspenso encontre 'Gravar Bootloader'
   - ✅ Clique aqui e ele gravará a nova partição, usando esptool
   - Leva 53.6 segundos e você tem a nova partição para KissTelegram 

Continue com Upload #2.

---

### Upload #2: Upload do Sketch

1. **Desconecte a porta USB-C DIREITA**

2. **Conecte a porta USB-C ESQUERDA** (porta OTG) ao seu PC

3. **Selecione a nova porta**: Ferramentas → Porta → Selecione a nova porta COM
   - **Importante**: O número da porta mudará! Procure dados no Monitor Serial para confirmar a porta correta, por exemplo, pressione reset do ESP32s3 até ver dados como resposta

4. **Verifique as configurações novamente**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Pressione Upload novamente** (`Ctrl+U`)

6. **Aguarde ~2-3 minutos** (apagando + carregando)

7. **Abra o Monitor Serial** - você deve ver agora (se você configurou corretamente as credenciais 
em system_setup.h (o system_setup_template renomeado)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi conectado
   ✅ Bot do Telegram habilitado
   ✅ Sistema pronto
   ```

8. **Verifique o Telegram** - você receberá a mensagem de boas-vindas:
   ```
   📦 Olá! KissTelegram está pronto.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Sinal WiFi: -59 dBm (Bom)
   ✅ 0 mensagens na fila
   ```

**Sucesso!** Seu ESP32-S3 agora executa KissTelegram com as partições corretas.

---

### Uploads Futuros

**Boas notícias:** Após os dois uploads iniciais, todos os uploads futuros funcionam normalmente:

- Use a **porta USB-C ESQUERDA** (OTG)
- **Não precisa** de "Erase All Flash" mais (a menos que você tenha feito alterações nos dados NVRAM)
- Carregue uma vez e funciona imediatamente

---

## Arquivos de Configuração

### system_setup.h (Requerido Antes do Primeiro Upload!)

**Antes de compilar:**

1. Navegue até sua pasta KissTelegram
2. Encontre `system_setup_template.h`
3. **Renomeie** para `system_setup.h`
4. **Abra** `system_setup.h` e preencha:

```cpp
// Seu Bot do Telegram (do BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Seu Chat ID (do @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Suas credenciais WiFi
#define KISS_FALLBACK_WIFI_SSID "NomeDoSeuWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "SenhaDoSeuWiFi"

// Segurança OTA (mude o PIN/PUK padrão!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 dígitos
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 dígitos
```

5. **Salve** o arquivo

**⚠️ Aviso de Segurança:** Mude o PIN padrão (`0000`) e o PUK (`00000000`) para seus próprios segredos!

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

## Sucesso! E Agora?

### Verificar Que Tudo Funciona

1. **Envie `/status` ao seu bot** no Telegram - você receberá um relatório de status detalhado:
   ```
   📦 KissTelegram v1.x.x
   🎯 CONFIABILIDADE DO SISTEMA
   ✅ Sistema: CONFIÁVEL
   ✅ Mensagens enviadas: 2
   💾 Mensagens pendentes: 0
   📡 Sinal WiFi: -59 dBm (Bom)
   🔋 Tempo ativo: 123 segundos
   💾 Memória livre: 223 KB
   ```

2. **Verifique o Monitor Serial** - não deve mostrar erros

3. **Teste os comandos**:
   - `/start` - Mensagem de boas-vindas
   - `/help` - Comandos disponíveis
   - `/status` - Status do sistema (verificação de saúde)

---

### Entendendo Atualizações OTA

Uma vez que KissTelegram esteja rodando, você pode atualizar o firmware **via Telegram** (sem cabo USB!):

1. Envie `/ota` ao seu bot
2. Digite o PIN: `/otapin 0000` (ou seu PIN personalizado)
3. **Envie seu arquivo de firmware `.bin`** (arraste e solte no Telegram)
4. O bot verifica o checksum automaticamente
5. Confirme: `/otaconfirm`
6. O ESP32 reinicia com o novo firmware
7. **Dentro de 60 segundos**, envie `/otaok` para confirmar que funciona
8. Se você não confirmar, o ESP32 **reverte automaticamente** para o firmware anterior!

📖 **Leia mais:** Veja `README_KissOTA_PT.md` para documentação OTA completa.

---

### Explorar o Código de Exemplo

O exemplo `suite_kiss.ino` demonstra:

- ✅ Gerenciamento WiFi com monitoramento de qualidade
- ✅ Fila de mensagens com prioridades
- ✅ Modos de gerenciamento de energia
- ✅ Manipulação de comandos (`/start`, `/help`, `/status`, etc.)
- ✅ Atualizações OTA via Telegram
- ✅ Recuperação de crashes e persistência
- ✅ Conexões seguras SSL/TLS

**Dica profissional:** Use o comando `/status` como sua **ferramenta de monitoramento de saúde** - é sua janela para os internos do KissTelegram!

---

### Solução de Problemas Comuns

**Problema: "Porta não encontrada" ou "Acesso negado"**
- Windows bloqueou a porta. Desconecte USB, aguarde 5s, reconecte.
- Tente um cabo USB diferente (alguns são apenas para carga, não para dados)

**Problema: "Timeout aguardando dispositivo" durante o upload**
- Porta USB errada! Lembre-se: porta DIREITA para primeiro upload, porta ESQUERDA para segundo
- Mantenha pressionado o botão BOOT no ESP32 enquanto clica em Upload, solte após aparecer "Connecting..."

**Problema: Monitor Serial mostra caracteres lixo**
- Taxa de baud incorreta. Configure para **115200** no menu suspenso do Monitor Serial

**Problema: O bot não responde no Telegram**
- Verifique se `system_setup.h` tem o Bot Token e Chat ID corretos
- Verifique se as credenciais WiFi estão corretas
- Abra o Monitor Serial e procure por mensagens de conexão WiFi

**Problema: Erro de compilação "Tabela de partições não cabe"**
- Não adicionou a partição personalizada ao `boards.txt` corretamente
- Ou não selecionou "Custom (4MB APP/12MB LtlFS)" em Ferramentas → Partition Scheme

---

### Obter Mais Ajuda

- 📧 **Email**: victek@gmail.com
- 📖 **Documentação**: Veja todos os arquivos `README_*.md` na sua pasta KissTelegram
- 🐛 **Relatórios de Bugs**: GitHub issues (link no README.md principal)
- 💡 **Solicitações de Recursos**: Também bem-vindas por email ou GitHub!

---

## Resumo: O Processo Completo

```
1. Obter Bot Token + Chat ID do Telegram ✅
2. Modificar boards.txt (adicionar partição personalizada) ✅
3. Configurar Arduino IDE (Partição Custom, Erase habilitado) ✅
4. Editar system_setup.h (credenciais) ✅
5. Conectar porta USB DIREITA ✅
6. Upload #1 (Gravar Bootloader) ✅
7. Desconectar DIREITA, conectar porta USB ESQUERDA ✅
8. Upload #2 (Carregar Sketch KissTelegram) ✅
9. Receber mensagem de boas-vindas no Telegram ✅
10. Enviar /status para verificar que tudo funciona ✅
```

**Você está pronto para construir projetos incríveis com KissTelegram!** 🎉
