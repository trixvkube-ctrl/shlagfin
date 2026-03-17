#pragma once

#include "config.h"
#include "logger.h"

class Storage {
 public:
  explicit Storage(Logger& logger) : logger_(logger) {}

  bool begin();
  bool load(AppConfig& appConfig);
  bool save(const AppConfig& appConfig);
  void resetToDefaults(AppConfig& appConfig);
  static String hashPassword(const String& plain);

 private:
  Logger& logger_;
  const char* kConfigPath_ = "/config.json";
};
