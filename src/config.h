#pragma once

#include <Arduino.h>
#include <IPAddress.h>

enum class BarrierOpenMode : uint8_t {
  PULSE = 0,
  HOLD_UNTIL_LIMIT = 1
};

enum class RepeatCommandPolicy : uint8_t {
  IGNORE = 0,
  REFRESH_HOLD_TIMER = 1,
  QUEUE_ONE_PENDING_REOPEN = 2
};

enum class BarrierState : uint8_t {
  BOOT = 0,
  IDLE_UNKNOWN,
  IDLE_CLOSED,
  OPENING,
  OPENED_HOLD,
  CLOSING,
  ERROR
};

struct AppConfig {
  BarrierOpenMode open_mode;
  RepeatCommandPolicy repeat_policy;

  uint16_t open_pulse_ms;
  uint16_t open_timeout_ms;
  uint16_t open_hold_ms;
  uint16_t close_timeout_ms;
  uint16_t close_extra_ms;
  uint8_t close_retries;
  uint16_t debounce_ms;

  bool relay_invert;
  bool open_limit_invert;
  bool close_limit_invert;
  bool allow_commands_in_error;
  bool auto_recover_from_error;

  bool dhcp_enable;
  IPAddress static_ip;
  IPAddress subnet;
  IPAddress gateway;
  IPAddress dns;
  uint16_t http_port;
  IPAddress trusted_ip;
  bool trusted_ip_enabled;
  String shared_token;

  String login;
  String password_hash;

  static AppConfig defaults() {
    AppConfig c;
    c.open_mode = BarrierOpenMode::PULSE;
    c.repeat_policy = RepeatCommandPolicy::IGNORE;
    c.open_pulse_ms = 500;
    c.open_timeout_ms = 8000;
    c.open_hold_ms = 5000;
    c.close_timeout_ms = 9000;
    c.close_extra_ms = 1200;
    c.close_retries = 3;
    c.debounce_ms = 80;
    c.relay_invert = true;
    c.open_limit_invert = false;
    c.close_limit_invert = false;
    c.allow_commands_in_error = false;
    c.auto_recover_from_error = false;

    c.dhcp_enable = true;
    c.static_ip = IPAddress(192, 168, 1, 200);
    c.subnet = IPAddress(255, 255, 255, 0);
    c.gateway = IPAddress(192, 168, 1, 1);
    c.dns = IPAddress(8, 8, 8, 8);
    c.http_port = 80;
    c.trusted_ip = IPAddress(0, 0, 0, 0);
    c.trusted_ip_enabled = false;
    c.shared_token = "changeme-token";

    c.login = "admin";
    c.password_hash = "";  // set on first boot to hash(admin)
    return c;
  }
};

inline const char* barrierStateToStr(BarrierState s) {
  switch (s) {
    case BarrierState::BOOT: return "BOOT";
    case BarrierState::IDLE_UNKNOWN: return "IDLE_UNKNOWN";
    case BarrierState::IDLE_CLOSED: return "IDLE_CLOSED";
    case BarrierState::OPENING: return "OPENING";
    case BarrierState::OPENED_HOLD: return "OPENED_HOLD";
    case BarrierState::CLOSING: return "CLOSING";
    case BarrierState::ERROR: return "ERROR";
  }
  return "UNKNOWN";
}
