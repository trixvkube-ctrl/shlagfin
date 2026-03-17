#include "net_server.h"

#include <ArduinoJson.h>

#include "pins.h"

NetServer::NetServer(AppConfig& appConfig, BarrierController& barrier, Storage& storage, Logger& logger,
                     NetPingCompat& netpingCompat, WebUi& webUi)
    : cfg_(appConfig), barrier_(barrier), storage_(storage), logger_(logger),
      netping_(netpingCompat), webUi_(webUi) {}

void NetServer::begin() {
  startEthernet();
  server_ = new EthernetServer(cfg_.http_port);
  server_->begin();
  logger_.log("INFO", "net", "HTTP server started");
}

void NetServer::startEthernet() {
  Ethernet.init(Pins::ENC_CS);
  int rc = 0;
  if (cfg_.dhcp_enable) {
    rc = Ethernet.begin(mac_);
  } else {
    Ethernet.begin(mac_, cfg_.static_ip, cfg_.dns, cfg_.gateway, cfg_.subnet);
    rc = 1;
  }
  if (rc == 0) {
    logger_.log("WARN", "net", "Ethernet link/config failed");
  } else {
    String msg = "network up " + Ethernet.localIP().toString();
    logger_.log("INFO", "net", msg.c_str());
  }
}

IPAddress NetServer::currentIp() const { return Ethernet.localIP(); }

String NetServer::urlDecode(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    if (in[i] == '+') {
      out += ' ';
    } else if (in[i] == '%' && i + 2 < in.length()) {
      const char h1 = in[i + 1];
      const char h2 = in[i + 2];
      char hex[3] = {h1, h2, 0};
      out += static_cast<char>(strtol(hex, nullptr, 16));
      i += 2;
    } else {
      out += in[i];
    }
  }
  return out;
}

bool NetServer::getKV(const String& body, const String& key, String& value) {
  int start = body.indexOf(key + "=");
  if (start < 0) return false;
  start += key.length() + 1;
  int end = body.indexOf('&', start);
  if (end < 0) end = body.length();
  value = urlDecode(body.substring(start, end));
  return true;
}

bool NetServer::isAuthorized(const String& path, const String& cookie) const {
  if (path == "/login" || path.startsWith("/io.cgi") || path.startsWith("/event")) return true;
  return loggedIn_ && cookie.indexOf("SID=1") >= 0;
}

bool NetServer::tokenAllowed(const String& query, IPAddress remoteIp) const {
  if (cfg_.trusted_ip_enabled && remoteIp != cfg_.trusted_ip) return false;
  const int t = query.indexOf("token=");
  if (t < 0) return false;
  int end = query.indexOf('&', t);
  if (end < 0) end = query.length();
  const String val = urlDecode(query.substring(t + 6, end));
  return val == cfg_.shared_token;
}

void NetServer::sendResponse(EthernetClient& client, int code, const char* contentType, const String& body,
                             const char* extraHeaders) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.println(code == 200 ? " OK" : (code == 302 ? " Found" : " Error"));
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  if (extraHeaders) client.println(extraHeaders);
  client.print("Content-Length: ");
  client.println(body.length());
  client.println();
  client.print(body);
}

void NetServer::handleClient(EthernetClient& client) {
  String reqLine;
  String headerLine;
  String cookie;
  int contentLength = 0;
  uint32_t started = millis();
  while (client.connected() && millis() - started < 1000) {
    if (!client.available()) continue;
    reqLine = client.readStringUntil('\n');
    reqLine.trim();
    break;
  }
  if (reqLine.length() == 0) return;

  while (client.connected()) {
    headerLine = client.readStringUntil('\n');
    headerLine.trim();
    if (headerLine.length() == 0) break;
    if (headerLine.startsWith("Content-Length:")) contentLength = headerLine.substring(15).toInt();
    if (headerLine.startsWith("Cookie:")) cookie = headerLine.substring(7);
  }

  String method = reqLine.substring(0, reqLine.indexOf(' '));
  int pathStart = reqLine.indexOf(' ') + 1;
  int pathEnd = reqLine.indexOf(' ', pathStart);
  String target = reqLine.substring(pathStart, pathEnd);
  String path = target;
  String query;
  const int qPos = target.indexOf('?');
  if (qPos >= 0) {
    path = target.substring(0, qPos);
    query = target.substring(qPos + 1);
  }

  String body;
  while (contentLength > 0 && client.available()) {
    body += static_cast<char>(client.read());
    contentLength--;
  }

  if (!isAuthorized(path, cookie)) {
    sendResponse(client, 302, "text/plain", "redirect", "Location: /login\r\n");
    return;
  }

  if (path == "/io.cgi") {
    String out;
    bool ok = netping_.handleIoCgi(query, out);
    sendResponse(client, ok ? 200 : 400, "text/plain", out);
    return;
  }

  if (path == "/event") {
    if (!tokenAllowed(query, client.remoteIP())) {
      sendResponse(client, 403, "text/plain", "forbidden");
      return;
    }
    if (query.indexOf("action=open") >= 0) {
      barrier_.requestOpen("event");
      sendResponse(client, 200, "text/plain", "ok");
    } else {
      sendResponse(client, 400, "text/plain", "bad action");
    }
    return;
  }

  if (path == "/login" && method == "GET") {
    sendResponse(client, 200, "text/html", webUi_.loginPage("Use configured credentials"));
    return;
  }
  if (path == "/login" && method == "POST") {
    String login;
    String pass;
    getKV(body, "login", login);
    getKV(body, "password", pass);
    if (login == cfg_.login && Storage::hashPassword(pass) == cfg_.password_hash) {
      loggedIn_ = true;
      sendResponse(client, 302, "text/plain", "ok", "Set-Cookie: SID=1\r\nLocation: /\r\n");
    } else {
      sendResponse(client, 401, "text/html", webUi_.loginPage("Invalid credentials"));
    }
    return;
  }

  if (path == "/") { sendResponse(client, 200, "text/html", webUi_.statusPage(currentIp())); return; }
  if (path == "/control") { sendResponse(client, 200, "text/html", webUi_.controlPage()); return; }
  if (path == "/logic") { sendResponse(client, 200, "text/html", webUi_.logicPage()); return; }
  if (path == "/network") { sendResponse(client, 200, "text/html", webUi_.networkPage()); return; }
  if (path == "/security") { sendResponse(client, 200, "text/html", webUi_.securityPage()); return; }
  if (path == "/logs") { sendResponse(client, 200, "text/html", webUi_.logsPage()); return; }

  if (path == "/api/status") {
    StaticJsonDocument<512> doc;
    doc["state"] = barrierStateToStr(barrier_.state());
    doc["relay"] = barrier_.relayIsActive();
    doc["open_limit"] = barrier_.openLimitActive();
    doc["close_limit"] = barrier_.closeLimitActive();
    doc["retry_counter"] = barrier_.retryCounter();
    doc["last_error"] = barrier_.lastError();
    doc["ip"] = currentIp().toString();
    doc["uptime_s"] = millis() / 1000UL;
    String out;
    serializeJson(doc, out);
    sendResponse(client, 200, "application/json", out);
    return;
  }

  if (path == "/api/open" && method == "POST") { barrier_.requestOpen("http"); sendResponse(client, 200, "text/plain", "ok"); return; }
  if (path == "/api/close" && method == "POST") { barrier_.requestClose("http"); sendResponse(client, 200, "text/plain", "ok"); return; }
  if (path == "/api/reset_error" && method == "POST") { barrier_.resetError("http"); sendResponse(client, 200, "text/plain", "ok"); return; }
  if (path == "/api/test_relay" && method == "POST") { barrier_.forceRelay(true, cfg_.open_pulse_ms, "http"); sendResponse(client, 200, "text/plain", "ok"); return; }
  if (path == "/api/reboot" && method == "POST") { sendResponse(client, 200, "text/plain", "rebooting"); ESP.restart(); return; }

  if (path == "/api/config" && method == "GET") {
    StaticJsonDocument<768> doc;
    doc["open_mode"] = static_cast<uint8_t>(cfg_.open_mode);
    doc["repeat_policy"] = static_cast<uint8_t>(cfg_.repeat_policy);
    doc["open_pulse_ms"] = cfg_.open_pulse_ms;
    doc["open_timeout_ms"] = cfg_.open_timeout_ms;
    doc["open_hold_ms"] = cfg_.open_hold_ms;
    doc["close_timeout_ms"] = cfg_.close_timeout_ms;
    doc["close_extra_ms"] = cfg_.close_extra_ms;
    doc["close_retries"] = cfg_.close_retries;
    doc["dhcp_enable"] = cfg_.dhcp_enable;
    doc["static_ip"] = cfg_.static_ip.toString();
    doc["http_port"] = cfg_.http_port;
    doc["trusted_ip"] = cfg_.trusted_ip.toString();
    doc["shared_token"] = cfg_.shared_token;
    String out;
    serializeJson(doc, out);
    sendResponse(client, 200, "application/json", out);
    return;
  }

  if (path == "/api/config" && method == "POST") {
    String val;
    if (getKV(body, "open_mode", val)) cfg_.open_mode = static_cast<BarrierOpenMode>(val.toInt());
    if (getKV(body, "repeat_policy", val)) cfg_.repeat_policy = static_cast<RepeatCommandPolicy>(val.toInt());
    if (getKV(body, "open_pulse_ms", val)) cfg_.open_pulse_ms = val.toInt();
    if (getKV(body, "open_timeout_ms", val)) cfg_.open_timeout_ms = val.toInt();
    if (getKV(body, "open_hold_ms", val)) cfg_.open_hold_ms = val.toInt();
    if (getKV(body, "close_timeout_ms", val)) cfg_.close_timeout_ms = val.toInt();
    if (getKV(body, "close_extra_ms", val)) cfg_.close_extra_ms = val.toInt();
    if (getKV(body, "close_retries", val)) cfg_.close_retries = val.toInt();
    if (getKV(body, "relay_active_high", val)) cfg_.relay_invert = (val.toInt() == 0);
    if (getKV(body, "open_limit_invert", val)) cfg_.open_limit_invert = (val.toInt() != 0);
    if (getKV(body, "close_limit_invert", val)) cfg_.close_limit_invert = (val.toInt() != 0);
    if (getKV(body, "debounce_ms", val)) cfg_.debounce_ms = val.toInt();
    if (getKV(body, "allow_commands_in_error", val)) cfg_.allow_commands_in_error = (val.toInt() != 0);
    if (getKV(body, "auto_recover_from_error", val)) cfg_.auto_recover_from_error = (val.toInt() != 0);
    if (getKV(body, "dhcp_enable", val)) cfg_.dhcp_enable = (val.toInt() != 0);
    if (getKV(body, "static_ip", val)) cfg_.static_ip.fromString(val);
    if (getKV(body, "subnet", val)) cfg_.subnet.fromString(val);
    if (getKV(body, "gateway", val)) cfg_.gateway.fromString(val);
    if (getKV(body, "dns", val)) cfg_.dns.fromString(val);
    if (getKV(body, "http_port", val)) cfg_.http_port = val.toInt();
    if (getKV(body, "trusted_ip_enabled", val)) cfg_.trusted_ip_enabled = (val.toInt() != 0);
    if (getKV(body, "trusted_ip", val)) cfg_.trusted_ip.fromString(val);
    if (getKV(body, "shared_token", val)) cfg_.shared_token = val;
    if (getKV(body, "login", val)) cfg_.login = val;
    String currentPass;
    String newPass;
    getKV(body, "current_password", currentPass);
    if (getKV(body, "new_password", newPass) && newPass.length() > 0) {
      if (Storage::hashPassword(currentPass) == cfg_.password_hash) {
        cfg_.password_hash = Storage::hashPassword(newPass);
        logger_.log("INFO", "security", "password changed");
      } else {
        logger_.log("WARN", "security", "password change rejected");
      }
    }
    storage_.save(cfg_);
    sendResponse(client, 302, "text/plain", "saved", "Location: /\r\n");
    return;
  }

  sendResponse(client, 404, "text/plain", "not found");
}

void NetServer::update() {
  if (cfg_.dhcp_enable && millis() - lastDhcpCheckMs_ > 10000UL) {
    lastDhcpCheckMs_ = millis();
    Ethernet.maintain();
  }

  if (!server_) return;
  EthernetClient client = server_->available();
  if (client) {
    handleClient(client);
    client.stop();
  }
}
