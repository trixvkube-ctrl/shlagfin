#pragma once

#include <Arduino.h>

struct LogEntry {
  uint32_t uptime_ms;
  char event_type[12];
  char source[12];
  char message[96];
};

class Logger {
 public:
  static const size_t CAPACITY = 80;

  void begin();
  void log(const char* eventType, const char* source, const char* message);
  size_t count() const;
  const LogEntry& getFromOldest(size_t idx) const;

 private:
  LogEntry entries_[CAPACITY];
  size_t head_ = 0;
  size_t size_ = 0;
};
