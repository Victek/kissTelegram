# Contributing to KissTelegram

Thank you for your interest in contributing to KissTelegram! This document provides guidelines for contributing to the project.

---

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [How Can I Contribute?](#how-can-i-contribute)
3. [Development Setup](#development-setup)
4. [Coding Guidelines](#coding-guidelines)
5. [Commit Guidelines](#commit-guidelines)
6. [Pull Request Process](#pull-request-process)
7. [Reporting Bugs](#reporting-bugs)
8. [Suggesting Features](#suggesting-features)
9. [Translation Contributions](#translation-contributions)

---

## Code of Conduct

- **Be respectful** and constructive in all interactions
- **Focus on technical merit** and objective discussion
- **Help newcomers** - we were all beginners once
- **Provide evidence** for claims (benchmarks, logs, code)

---

## How Can I Contribute?

### 1. Report Bugs
See [Reporting Bugs](#reporting-bugs) section below.

### 2. Suggest Features
See [Suggesting Features](#suggesting-features) section below.

### 3. Improve Documentation
- Fix typos or unclear explanations
- Add examples or use cases
- Translate documentation to new languages
- Improve GETTING_STARTED.md with better troubleshooting

### 4. Submit Code
- Fix bugs
- Add new features
- Optimize performance
- Improve memory usage

### 5. Test and Review
- Test pre-release versions
- Review pull requests
- Provide feedback on proposed changes

---

## Development Setup

### Prerequisites
- **ESP32-S3** with 16MB Flash / 8MB PSRAM (recommended: N16R8)
- **Arduino IDE 2.x** or **PlatformIO**
- **ESP32 Arduino Core 3.3.4** or newer
- **Telegram Bot Token** and **Chat ID** for testing

### Setup Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/Victek/KissTelegram.git
   cd KissTelegram
   ```

2. **Create your credentials file**
   ```bash
   cp system_setup_template.h system_setup.h
   ```
   Then edit `system_setup.h` with your credentials.

3. **Install ESP32 board support** in Arduino IDE:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Tools → Board → Boards Manager → Install "esp32"

4. **Configure custom partition** (see GETTING_STARTED.md for details)

5. **Compile and test** on real hardware

---

## Coding Guidelines

### General Principles
- **Use `char[]` arrays** instead of Arduino `String` class
- **Avoid dynamic memory allocation** in loops or frequent operations
- **Check return values** and handle errors gracefully
- **Document complex logic** with comments
- **Keep functions focused** on a single responsibility

### Code Style
```cpp
// Use clear variable names
const char* botToken = credentials.getBotToken();  // Good
const char* bt = creds.getTkn();                   // Bad

// Use consistent indentation (2 spaces)
if (condition) {
  doSomething();
  doSomethingElse();
}

// Add comments for non-obvious code
// Calculate RSSI quality based on signal strength
// -50 dBm or better = Excellent
// -60 to -50 dBm = Good
if (rssi >= -50) {
  quality = WIFI_EXCELLENT;
}

// Use const for read-only data
const int MAX_RETRIES = 5;

// Prefer stack allocation over heap
char buffer[256];  // Good
char* buffer = new char[256];  // Avoid if possible
```

### Memory Safety
- **Always check buffer sizes** before writing
- **Use `snprintf()` instead of `sprintf()`**
- **Validate input parameters** in public functions
- **Free allocated memory** in destructors

---

## Commit Guidelines

### Commit Message Format
```
<type>(<scope>): <subject>

<body (optional)>

<footer (optional)>
```

### Types
- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Code style changes (formatting, no logic change)
- **refactor**: Code refactoring (no functional change)
- **perf**: Performance improvements
- **test**: Adding or updating tests
- **chore**: Build process, dependencies, or tooling changes

### Examples
```
feat(ota): Add automatic rollback on boot failure

Implements boot loop detection and automatic rollback to
previous firmware if new firmware fails to validate within
60 seconds.

Closes #42
```

```
fix(wifi): Handle reconnection during message send

Added check for WiFi status before sending messages.
Queues messages to LittleFS if WiFi is disconnected.

Fixes #38
```

```
docs(getting-started): Add troubleshooting for USB port issues

Added section explaining the two-upload process and USB port
switching for ESP32-S3 N16R8.
```

---

## Pull Request Process

1. **Fork the repository** and create a new branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following the coding guidelines

3. **Test thoroughly** on real ESP32-S3 hardware:
   - Compile successfully
   - Upload and run without errors
   - Test affected functionality
   - Check `/estado` output for memory leaks
   - Test WiFi reconnection scenarios

4. **Update documentation** if needed:
   - Update README.md if adding features
   - Update GETTING_STARTED.md if changing setup process
   - Update comments in code

5. **Commit your changes** with clear commit messages

6. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Create a Pull Request** with:
   - Clear description of changes
   - Reference to related issues
   - Test results and serial output
   - Screenshots if applicable

8. **Respond to feedback** from reviewers

---

## Reporting Bugs

**Before reporting:**
- Check [existing issues](https://github.com/Victek/KissTelegram/issues)
- Read [GETTING_STARTED.md](GETTING_STARTED.md) for common issues
- Verify it's not a configuration problem

**When reporting, include:**
- Clear description of the bug
- Steps to reproduce
- Expected vs actual behavior
- Serial monitor output
- `/estado` command output
- Environment details (board, IDE version, ESP32 core version)
- Minimal code example that reproduces the issue

Use the [Bug Report template](.github/ISSUE_TEMPLATE/bug_report.md).

---

## Suggesting Features

**Before suggesting:**
- Check if it already exists in open/closed issues
- Consider if it fits the project's scope (ESP32-S3 Telegram bot library)
- Think about backward compatibility

**When suggesting, include:**
- Clear description of the feature
- Use case and motivation
- Proposed implementation (optional)
- Alternatives you've considered

Use the [Feature Request template](.github/ISSUE_TEMPLATE/feature_request.md).

---

## Translation Contributions

We welcome translations to new languages!

### Adding a New Language

1. **Create language header file**:
   ```cpp
   // lang_xx.h (replace xx with language code)
   #ifndef LANG_XX_H
   #define LANG_XX_H

   #define HELLO_MESSAGE "Your translated message"
   // ... translate all macros

   #endif
   ```

2. **Translate documentation**:
   - README_XX.md
   - GETTING_STARTED_XX.md
   - BENCHMARK_XX.md
   - README_KissOTA_XX.md

3. **Update lang.h** to include new language option

4. **Test compilation** with new language selected

5. **Submit PR** with all translated files

### Translation Guidelines
- Keep technical terms in English (ESP32-S3, WiFi, OTA, etc.)
- Maintain same tone as original (professional, technical)
- Test that messages fit in display/Telegram message limits
- Preserve formatting (newlines, emojis, spacing)

---

## Questions?

If you have questions about contributing:
- Open a [Discussion](https://github.com/Victek/KissTelegram/discussions)
- Email: victek@gmail.com
- Check existing documentation first

---

**Thank you for contributing to KissTelegram!** 🚀
