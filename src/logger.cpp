#include "logger.h"

void Logger::begin() {
  Serial.begin(115200);
  Serial.println();
}

void Logger::log(const char* eventType, const char* source, const char* message) {
  LogEntry& e = entries_[head_];
  e.uptime_ms = millis();
  snprintf(e.event_type, sizeof(e.event_type), "%s", eventType);
  snprintf(e.source, sizeof(e.source), "%s", source);
  snprintf(e.message, sizeof(e.message), "%s", message);

  head_ = (head_ + 1) % CAPACITY;
  if (size_ < CAPACITY) size_++;

  Serial.printf("[%lu] [%s] [%s] %s\n", static_cast<unsigned long>(e.uptime_ms), e.event_type,
                e.source, e.message);
}

size_t Logger::count() const { return size_; }

const LogEntry& Logger::getFromOldest(size_t idx) const {
  static LogEntry empty{};
  if (idx >= size_) return empty;
  const size_t oldest = (head_ + CAPACITY - size_) % CAPACITY;
  return entries_[(oldest + idx) % CAPACITY];
}
