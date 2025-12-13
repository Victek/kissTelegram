#ifndef KISS_SSL_H
#define KISS_SSL_H

#include <Arduino.h>
#include <WiFiClientSecure.h> //Wrapper template de defines

class KissSSL {
public:
  KissSSL();
  ~KissSSL();

  bool connectToTelegram();
  bool connect(const char* host, uint16_t port);
  bool connected();
  bool isConnected();
  void disconnect();

  void setCACert(const char* rootCA);
  void setInsecure();
  bool verify(const char* fingerprint, const char* url);
  void setSecureMode(bool secure);
  bool isSecureMode();
  void toggleSecureMode();


  size_t print(const char* str);
  size_t print(const String& str);
  size_t println(const char* str);
  size_t println(const String& str);

  int available();
  int read();
  int read(uint8_t* buffer, size_t size);
  void stop();
  void printInfo();

private:
  WiFiClientSecure* client;
  bool manuallyConnected;
  bool secureMode;
};

#endif