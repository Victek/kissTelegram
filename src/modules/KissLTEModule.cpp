// KissLTEModule.cpp
// Vicente Soriano - victek@gmail.com
// Implementación de helpers comunes para todos los módulos LTE

#include "KissLTEModule.h"
#include "../Kiss_setup.h"

// ========== HELPERS COMUNES A TODOS LOS MÓDULOS ==========

bool KissLTEModule::sendATCommand(const char* cmd, const char* expected, unsigned long timeout) {
    if (!_serial || !cmd) return false;

    // Limpiar buffer
    clearSerialBuffer();

    // Wake up si tiene DTR
    if (_dtrPin >= 0) {
        digitalWrite(_dtrPin, LOW);
        delay(30);
    }

    if (_debug) {
        KISS_LOGF("→ %s", cmd);
    }

    // Enviar comando
    _serial->println(cmd);
    delay(100);

    // Leer respuesta
    unsigned long startTime = millis();
    String response = "";

    while (millis() - startTime < timeout) {
        if (_serial->available()) {
            char c = _serial->read();
            if (c != '\r') {
                response += c;
            }

            // Buscar respuesta esperada
            if (response.indexOf(expected) >= 0) {
                if (_debug) {
                    KISS_LOGF("✓ %s", response.c_str());
                }
                return true;
            }

            // Detectar error
            if (response.indexOf("ERROR") >= 0) {
                if (_debug) {
                    KISS_LOGF("✗ ERROR: %s", response.c_str());
                }
                return false;
            }
        }
        yield();
    }

    if (_debug) {
        KISS_LOG("✗ Timeout");
    }
    return false;
}

bool KissLTEModule::sendATCommand(const char* cmd, char* response, size_t responseSize, unsigned long timeout) {
    if (!_serial || !cmd || !response || responseSize == 0) return false;

    // Limpiar buffer
    clearSerialBuffer();

    // Wake up si tiene DTR
    if (_dtrPin >= 0) {
        digitalWrite(_dtrPin, LOW);
        delay(30);
    }

    if (_debug) {
        KISS_LOGF("→ %s", cmd);
    }

    // Enviar comando
    _serial->println(cmd);
    delay(100);

    // Leer respuesta completa
    memset(response, 0, responseSize);
    unsigned long startTime = millis();
    size_t idx = 0;
    bool foundOK = false;
    bool foundERROR = false;
    unsigned long lastCharTime = millis();

    while (millis() - startTime < timeout && idx < responseSize - 1) {
        if (_serial->available()) {
            char c = _serial->read();
            lastCharTime = millis();

            if (c != '\r') {
                response[idx++] = c;
            }

            // Detectar fin de respuesta
            if (strstr(response, "OK")) foundOK = true;
            if (strstr(response, "ERROR")) foundERROR = true;

            // Salir si terminó la respuesta
            if ((foundOK || foundERROR) && (millis() - lastCharTime > 100)) {
                break;
            }
        }
        yield();
    }

    response[idx] = '\0';

    if (_debug && idx > 0) {
        KISS_LOGF("← %s", response);
    }

    return foundOK;
}

String KissLTEModule::readResponse(unsigned long timeout) {
    if (!_serial) return "";

    String response = "";
    unsigned long startTime = millis();
    unsigned long lastCharTime = millis();

    while (millis() - startTime < timeout) {
        if (_serial->available()) {
            char c = _serial->read();
            lastCharTime = millis();

            if (c != '\r') {
                response += c;
            }

            // Si llevamos 200ms sin recibir nada, asumir fin
            if (millis() - lastCharTime > 200) {
                break;
            }
        }
        yield();
    }

    return response;
}

void KissLTEModule::clearSerialBuffer() {
    if (!_serial) return;

    while (_serial->available()) {
        _serial->read();
        yield();
    }
}

bool KissLTEModule::waitForResponse(const char* expected, unsigned long timeout) {
    if (!_serial || !expected) return false;

    unsigned long startTime = millis();
    String response = "";

    while (millis() - startTime < timeout) {
        if (_serial->available()) {
            char c = _serial->read();
            if (c != '\r') {
                response += c;
            }

            if (response.indexOf(expected) >= 0) {
                return true;
            }

            if (response.indexOf("ERROR") >= 0) {
                return false;
            }
        }
        yield();
    }

    return false;
}
