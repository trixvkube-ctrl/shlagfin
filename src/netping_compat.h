#pragma once

#include <Arduino.h>

#include "barrier_controller.h"
#include "logger.h"

class NetPingCompat {
 public:
  NetPingCompat(BarrierController& barrier, Logger& logger) : barrier_(barrier), logger_(logger) {}

  bool handleIoCgi(const String& query, String& outBody);

 private:
  BarrierController& barrier_;
  Logger& logger_;

  static bool getParam(const String& query, const String& key, String& value, bool& hasValue);
};
