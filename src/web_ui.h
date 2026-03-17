#pragma once

#include <Arduino.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"

class WebUi {
 public:
  WebUi(BarrierController& barrier, AppConfig& appConfig, Logger& logger)
      : barrier_(barrier), cfg_(appConfig), logger_(logger) {}

  String statusPage(const IPAddress& ip) const;
  String controlPage() const;
  String logicPage() const;
  String networkPage() const;
  String securityPage() const;
  String logsPage() const;
  String loginPage(const String& msg) const;

 private:
  BarrierController& barrier_;
  AppConfig& cfg_;
  Logger& logger_;

  static String htmlHead(const char* title);
};
