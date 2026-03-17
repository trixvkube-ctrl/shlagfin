#include "storage.h"

#include <ArduinoJson.h>
#include <Hash.h>
#include <LittleFS.h>

bool Storage::begin() {
  if (!LittleFS.begin()) {
    logger_.log("ERROR", "storage", "LittleFS mount failed");
    return false;
  }
  return true;
}

String Storage::hashPassword(const String& plain) { return sha1(plain); }

void Storage::resetToDefaults(AppConfig& appConfig) {
  appConfig = AppConfig::defaults();
  appConfig.password_hash = hashPassword("admin");
}

bool Storage::load(AppConfig& c) {
  if (!LittleFS.exists(kConfigPath_)) {
    logger_.log("INFO", "storage", "config missing, loading defaults");
    resetToDefaults(c);
    return save(c);
  }

  File f = LittleFS.open(kConfigPath_, "r");
  if (!f) {
    logger_.log("ERROR", "storage", "config open failed");
    resetToDefaults(c);
    return save(c);
  }

  DynamicJsonDocument doc(3072);
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    logger_.log("ERROR", "storage", "config JSON invalid, defaults restored");
    resetToDefaults(c);
    return save(c);
  }

  AppConfig d = AppConfig::defaults();
  c.open_mode = static_cast<BarrierOpenMode>(doc["open_mode"] | static_cast<uint8_t>(d.open_mode));
  c.repeat_policy = static_cast<RepeatCommandPolicy>(doc["repeat_policy"] | static_cast<uint8_t>(d.repeat_policy));
  c.open_pulse_ms = doc["open_pulse_ms"] | d.open_pulse_ms;
  c.open_timeout_ms = doc["open_timeout_ms"] | d.open_timeout_ms;
  c.open_hold_ms = doc["open_hold_ms"] | d.open_hold_ms;
  c.close_timeout_ms = doc["close_timeout_ms"] | d.close_timeout_ms;
  c.close_extra_ms = doc["close_extra_ms"] | d.close_extra_ms;
  c.close_retries = doc["close_retries"] | d.close_retries;
  c.debounce_ms = doc["debounce_ms"] | d.debounce_ms;
  c.relay_invert = doc["relay_invert"] | d.relay_invert;
  c.open_limit_invert = doc["open_limit_invert"] | d.open_limit_invert;
  c.close_limit_invert = doc["close_limit_invert"] | d.close_limit_invert;
  c.allow_commands_in_error = doc["allow_commands_in_error"] | d.allow_commands_in_error;
  c.auto_recover_from_error = doc["auto_recover_from_error"] | d.auto_recover_from_error;
  c.dhcp_enable = doc["dhcp_enable"] | d.dhcp_enable;

  const String defaultStaticIp = d.static_ip.toString();
  const char* staticIpValue = doc["static_ip"] | defaultStaticIp.c_str();
  c.static_ip.fromString(staticIpValue);
  const String defaultSubnet = d.subnet.toString();
  const char* subnetValue = doc["subnet"] | defaultSubnet.c_str();
  c.subnet.fromString(subnetValue);
  const String defaultGateway = d.gateway.toString();
  const char* gatewayValue = doc["gateway"] | defaultGateway.c_str();
  c.gateway.fromString(gatewayValue);
  const String defaultDns = d.dns.toString();
  const char* dnsValue = doc["dns"] | defaultDns.c_str();
  c.dns.fromString(dnsValue);
  c.http_port = doc["http_port"] | d.http_port;
  c.trusted_ip_enabled = doc["trusted_ip_enabled"] | d.trusted_ip_enabled;
  const String defaultTrustedIp = d.trusted_ip.toString();
  const char* trustedIpValue = doc["trusted_ip"] | defaultTrustedIp.c_str();
  c.trusted_ip.fromString(trustedIpValue);
  const char* sharedTokenValue = doc["shared_token"] | d.shared_token.c_str();
  c.shared_token = String(sharedTokenValue);
  const char* loginValue = doc["login"] | d.login.c_str();
  c.login = String(loginValue);
  const char* passwordHashValue = doc["password_hash"] | "";
  c.password_hash = String(passwordHashValue);
  if (c.password_hash.isEmpty()) c.password_hash = hashPassword("admin");

  logger_.log("INFO", "storage", "config loaded");
  return true;
}

bool Storage::save(const AppConfig& c) {
  DynamicJsonDocument doc(3072);
  doc["open_mode"] = static_cast<uint8_t>(c.open_mode);
  doc["repeat_policy"] = static_cast<uint8_t>(c.repeat_policy);
  doc["open_pulse_ms"] = c.open_pulse_ms;
  doc["open_timeout_ms"] = c.open_timeout_ms;
  doc["open_hold_ms"] = c.open_hold_ms;
  doc["close_timeout_ms"] = c.close_timeout_ms;
  doc["close_extra_ms"] = c.close_extra_ms;
  doc["close_retries"] = c.close_retries;
  doc["debounce_ms"] = c.debounce_ms;
  doc["relay_invert"] = c.relay_invert;
  doc["open_limit_invert"] = c.open_limit_invert;
  doc["close_limit_invert"] = c.close_limit_invert;
  doc["allow_commands_in_error"] = c.allow_commands_in_error;
  doc["auto_recover_from_error"] = c.auto_recover_from_error;
  doc["dhcp_enable"] = c.dhcp_enable;
  doc["static_ip"] = c.static_ip.toString();
  doc["subnet"] = c.subnet.toString();
  doc["gateway"] = c.gateway.toString();
  doc["dns"] = c.dns.toString();
  doc["http_port"] = c.http_port;
  doc["trusted_ip_enabled"] = c.trusted_ip_enabled;
  doc["trusted_ip"] = c.trusted_ip.toString();
  doc["shared_token"] = c.shared_token;
  doc["login"] = c.login;
  doc["password_hash"] = c.password_hash;

  File f = LittleFS.open(kConfigPath_, "w");
  if (!f) {
    logger_.log("ERROR", "storage", "config write open failed");
    return false;
  }

  if (serializeJson(doc, f) == 0) {
    f.close();
    logger_.log("ERROR", "storage", "config write failed");
    return false;
  }
  f.close();
  logger_.log("INFO", "storage", "config saved");
  return true;
}
