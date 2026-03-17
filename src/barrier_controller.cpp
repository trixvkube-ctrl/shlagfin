#include "barrier_controller.h"

#include "pins.h"

BarrierController::BarrierController(Logger& logger, AppConfig& appConfig)
    : logger_(logger), cfg_(appConfig) {}

void BarrierController::begin() {
  pinMode(Pins::RELAY, OUTPUT);
  pinMode(Pins::OPEN_LIMIT, INPUT_PULLUP);
  pinMode(Pins::CLOSE_LIMIT, INPUT_PULLUP);

  setRelay(false);
  updateDebouncedInputs();

  if (closeLimitStable_) {
    enterState(BarrierState::IDLE_CLOSED);
  } else {
    enterState(BarrierState::IDLE_UNKNOWN);
  }
}

bool BarrierController::readOpenLimitRaw() const {
  bool active = digitalRead(Pins::OPEN_LIMIT) == LOW;
  return cfg_.open_limit_invert ? !active : active;
}

bool BarrierController::readCloseLimitRaw() const {
  bool active = digitalRead(Pins::CLOSE_LIMIT) == LOW;
  return cfg_.close_limit_invert ? !active : active;
}

void BarrierController::updateDebouncedInputs() {
  const uint32_t now = millis();
  const bool rawOpen = readOpenLimitRaw();
  const bool rawClose = readCloseLimitRaw();

  if (rawOpen != openLimitRaw_) {
    openLimitRaw_ = rawOpen;
    openLimitChangedMs_ = now;
  }
  if (rawClose != closeLimitRaw_) {
    closeLimitRaw_ = rawClose;
    closeLimitChangedMs_ = now;
  }

  if (now - openLimitChangedMs_ >= cfg_.debounce_ms) openLimitStable_ = openLimitRaw_;
  if (now - closeLimitChangedMs_ >= cfg_.debounce_ms) closeLimitStable_ = closeLimitRaw_;

  if (openLimitStable_ && closeLimitStable_) {
    if (bothLimitsActiveSince_ == 0) bothLimitsActiveSince_ = now;
    if (now - bothLimitsActiveSince_ > cfg_.debounce_ms * 3U) {
      enterError("both limits active");
    }
  } else {
    bothLimitsActiveSince_ = 0;
  }
}

void BarrierController::setRelay(bool active) {
  relayActive_ = active;
  const bool pinActive = cfg_.relay_invert ? !active : active;
  digitalWrite(Pins::RELAY, pinActive ? HIGH : LOW);
}

void BarrierController::enterState(BarrierState next) {
  state_ = next;
  char msg[48];
  snprintf(msg, sizeof(msg), "state -> %s", barrierStateToStr(next));
  logger_.log("STATE", "barrier", msg);
}

void BarrierController::enterError(const char* msg) {
  if (state_ == BarrierState::ERROR) return;
  lastError_ = msg;
  setRelay(false);
  forcedRelayUntilMs_ = 0;
  enterState(BarrierState::ERROR);
  logger_.log("ERROR", "barrier", msg);
}

void BarrierController::startOpenCycle() {
  closeRetryCounter_ = 0;
  actionStartedMs_ = millis();
  holdStartedMs_ = 0;
  if (cfg_.open_mode == BarrierOpenMode::PULSE) {
    setRelay(true);
    relayOnStartedMs_ = millis();
    logger_.log("INFO", "barrier", "open cycle started (PULSE)");
  } else {
    setRelay(true);
    logger_.log("INFO", "barrier", "open cycle started (HOLD)");
  }
  enterState(BarrierState::OPENING);
}

void BarrierController::startCloseCycle() {
  actionStartedMs_ = millis();
  closeWaitStartedMs_ = 0;
  setRelay(true);
  relayOnStartedMs_ = millis();
  enterState(BarrierState::CLOSING);
  logger_.log("INFO", "barrier", "closing started");
}

void BarrierController::handleRepeatCommand() {
  if (cfg_.repeat_policy == RepeatCommandPolicy::IGNORE) {
    logger_.log("INFO", "barrier", "repeat command ignored");
  } else if (cfg_.repeat_policy == RepeatCommandPolicy::REFRESH_HOLD_TIMER) {
    holdStartedMs_ = millis();
    logger_.log("INFO", "barrier", "hold timer refreshed");
  } else {
    pendingReopen_ = true;
    logger_.log("INFO", "barrier", "pending reopen queued");
  }
}

bool BarrierController::requestOpen(const char* source) {
  char m[48];
  snprintf(m, sizeof(m), "open requested by %s", source);
  logger_.log("CMD", "barrier", m);

  if (state_ == BarrierState::ERROR && !cfg_.allow_commands_in_error) return false;

  if (state_ == BarrierState::OPENING || state_ == BarrierState::OPENED_HOLD ||
      state_ == BarrierState::CLOSING) {
    handleRepeatCommand();
    return true;
  }

  startOpenCycle();
  return true;
}

bool BarrierController::requestClose(const char* source) {
  char m[48];
  snprintf(m, sizeof(m), "close requested by %s", source);
  logger_.log("CMD", "barrier", m);

  if (state_ == BarrierState::ERROR && !cfg_.allow_commands_in_error) return false;
  if (state_ == BarrierState::CLOSING) return true;
  startCloseCycle();
  return true;
}

bool BarrierController::resetError(const char* source) {
  if (state_ != BarrierState::ERROR) return true;
  char m[48];
  snprintf(m, sizeof(m), "error reset by %s", source);
  logger_.log("CMD", "barrier", m);
  lastError_ = "";
  pendingReopen_ = false;
  closeRetryCounter_ = 0;
  if (closeLimitStable_) {
    enterState(BarrierState::IDLE_CLOSED);
  } else {
    enterState(BarrierState::IDLE_UNKNOWN);
  }
  return true;
}

void BarrierController::forceRelay(bool active, uint32_t durationMs, const char* source) {
  char msg[56];
  snprintf(msg, sizeof(msg), "manual relay=%d %lums by %s", active ? 1 : 0,
           static_cast<unsigned long>(durationMs), source);
  logger_.log("CMD", "relay", msg);
  setRelay(active);
  if (active && durationMs > 0) {
    forcedRelayUntilMs_ = millis() + durationMs;
  } else {
    forcedRelayUntilMs_ = 0;
  }
}

bool BarrierController::relayIsActive() const { return relayActive_; }

void BarrierController::update() {
  updateDebouncedInputs();
  const uint32_t now = millis();

  if (forcedRelayUntilMs_ != 0 && static_cast<int32_t>(now - forcedRelayUntilMs_) >= 0) {
    setRelay(false);
    forcedRelayUntilMs_ = 0;
  }

  if (state_ == BarrierState::OPENING) {
    if (cfg_.open_mode == BarrierOpenMode::PULSE && relayActive_ && now - relayOnStartedMs_ >= cfg_.open_pulse_ms) {
      setRelay(false);
    }

    if (openLimitStable_) {
      setRelay(false);
      holdStartedMs_ = now;
      enterState(BarrierState::OPENED_HOLD);
      logger_.log("INFO", "barrier", "open confirmed by limit");
      return;
    }

    if (now - actionStartedMs_ > cfg_.open_timeout_ms) {
      setRelay(false);
      enterError("open timeout");
      return;
    }
  }

  if (state_ == BarrierState::OPENED_HOLD) {
    if (now - holdStartedMs_ >= cfg_.open_hold_ms) {
      startCloseCycle();
      return;
    }
  }

  if (state_ == BarrierState::CLOSING) {
    if (relayActive_ && now - relayOnStartedMs_ >= cfg_.open_pulse_ms) {
      setRelay(false);
      closeWaitStartedMs_ = now;
    }

    if (closeLimitStable_) {
      pendingReopen_ = false;
      enterState(BarrierState::IDLE_CLOSED);
      logger_.log("INFO", "barrier", "close confirmed by limit");
      return;
    }

    if (now - actionStartedMs_ > cfg_.close_timeout_ms) {
      if (closeRetryCounter_ < cfg_.close_retries) {
        if (closeWaitStartedMs_ == 0 || (now - closeWaitStartedMs_ >= cfg_.close_extra_ms)) {
          closeRetryCounter_++;
          char m[44];
          snprintf(m, sizeof(m), "closing retry %u of %u", closeRetryCounter_, cfg_.close_retries);
          logger_.log("WARN", "barrier", m);
          actionStartedMs_ = now;
          setRelay(true);
          relayOnStartedMs_ = now;
          closeWaitStartedMs_ = 0;
        }
      } else {
        enterError("closing failed: timeout");
        return;
      }
    }
  }

  if (state_ == BarrierState::IDLE_CLOSED && pendingReopen_) {
    pendingReopen_ = false;
    startOpenCycle();
  }

  if (state_ == BarrierState::ERROR && cfg_.auto_recover_from_error && !openLimitStable_ && !closeLimitStable_) {
    lastError_ = "";
    enterState(BarrierState::IDLE_UNKNOWN);
  }
}
