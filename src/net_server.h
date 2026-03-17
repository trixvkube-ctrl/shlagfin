#pragma once

#include <Arduino.h>
#include <EthernetENC.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"
#include "netping_compat.h"
#include "storage.h"
#include "web_ui.h"

class NetServer {
 public:
  NetServer(AppConfig& appConfig, BarrierController& barrier, Storage& storage, Logger& logger,
            NetPingCompat& netpingCompat, WebUi& webUi);

  void begin();
  void update();
  IPAddress currentIp() const;

 private:
  AppConfig& cfg_;
  BarrierController& barrier_;
  Storage& storage_;
  Logger& logger_;
  NetPingCompat& netping_;
  WebUi& webUi_;

  EthernetServer* server_ = nullptr;
  byte mac_[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x82, 0x60};
  uint32_t lastDhcpCheckMs_ = 0;
  bool loggedIn_ = false;

  void startEthernet();
  void handleClient(EthernetClient& client);

  static String urlDecode(const String& in);
  static bool getKV(const String& body, const String& key, String& value);
  bool isAuthorized(const String& path, const String& cookie) const;
  bool tokenAllowed(const String& query, IPAddress remoteIp) const;
  void sendResponse(EthernetClient& client, int code, const char* contentType, const String& body, const char* extraHeaders = nullptr);
};
