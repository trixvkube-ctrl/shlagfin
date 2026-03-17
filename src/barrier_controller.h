#pragma once

#include "config.h"
#include "logger.h"

class BarrierController {
 public:
  BarrierController(Logger& logger, AppConfig& appConfig);

  void begin();
  void update();

  bool requestOpen(const char* source);
  bool requestClose(const char* source);
  bool resetError(const char* source);

  void forceRelay(bool active, uint32_t durationMs, const char* source);
  bool relayIsActive() const;

  BarrierState state() const { return state_; }
  bool openLimitActive() const { return openLimitStable_; }
  bool closeLimitActive() const { return closeLimitStable_; }
  bool pendingReopen() const { return pendingReopen_; }
  uint8_t retryCounter() const { return closeRetryCounter_; }
  const String& lastError() const { return lastError_; }

 private:
  Logger& logger_;
  AppConfig& cfg_;

  BarrierState state_ = BarrierState::BOOT;
  bool relayActive_ = false;
  uint32_t relayOnStartedMs_ = 0;
  uint32_t actionStartedMs_ = 0;
  uint32_t holdStartedMs_ = 0;
  uint32_t closeWaitStartedMs_ = 0;
  uint8_t closeRetryCounter_ = 0;
  bool pendingReopen_ = false;
  String lastError_;

  bool openLimitRaw_ = false;
  bool closeLimitRaw_ = false;
  bool openLimitStable_ = false;
  bool closeLimitStable_ = false;
  uint32_t openLimitChangedMs_ = 0;
  uint32_t closeLimitChangedMs_ = 0;
  uint32_t bothLimitsActiveSince_ = 0;

  uint32_t forcedRelayUntilMs_ = 0;

  void setRelay(bool active);
  bool readOpenLimitRaw() const;
  bool readCloseLimitRaw() const;
  void updateDebouncedInputs();

  void enterState(BarrierState next);
  void enterError(const char* msg);
  void startOpenCycle();
  void startCloseCycle();
  void handleRepeatCommand();
};
