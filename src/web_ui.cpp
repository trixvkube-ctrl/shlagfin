#include "web_ui.h"

String WebUi::htmlHead(const char* title) {
  String s;
  s.reserve(300);
  s += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  s += "<title>";
  s += title;
  s += "</title><style>body{font-family:Arial;margin:12px}table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:6px}input{width:220px}small{color:#555}.nav a{margin-right:8px}</style></head><body>";
  s += "<div class='nav'><a href='/'>STATUS</a><a href='/control'>CONTROL</a><a href='/logic'>LOGIC SETTINGS</a><a href='/network'>NETWORK SETTINGS</a><a href='/security'>SECURITY</a><a href='/logs'>LOGS</a></div><hr>";
  return s;
}

String WebUi::statusPage(const IPAddress& ip) const {
  String s = htmlHead("Status");
  s += "<h3>STATUS</h3><table>";
  s += "<tr><td>Current state</td><td>" + String(barrierStateToStr(barrier_.state())) + "</td></tr>";
  s += "<tr><td>Barrier position</td><td>" + String(barrier_.openLimitActive() ? "OPEN" : (barrier_.closeLimitActive() ? "CLOSED" : "UNKNOWN")) + "</td></tr>";
  s += "<tr><td>Relay state</td><td>" + String(barrier_.relayIsActive() ? "ACTIVE" : "INACTIVE") + "</td></tr>";
  s += "<tr><td>Open limit status</td><td>" + String(barrier_.openLimitActive() ? "ACTIVE" : "INACTIVE") + "</td></tr>";
  s += "<tr><td>Close limit status</td><td>" + String(barrier_.closeLimitActive() ? "ACTIVE" : "INACTIVE") + "</td></tr>";
  s += "<tr><td>Barrier open mode</td><td>" + String(cfg_.open_mode == BarrierOpenMode::PULSE ? "PULSE" : "HOLD_UNTIL_LIMIT") + "</td></tr>";
  s += "<tr><td>Retry counter</td><td>" + String(barrier_.retryCounter()) + "</td></tr>";
  s += "<tr><td>Last error</td><td>" + barrier_.lastError() + "</td></tr>";
  s += "<tr><td>IP address</td><td>" + ip.toString() + "</td></tr>";
  s += "<tr><td>Uptime (s)</td><td>" + String(millis() / 1000UL) + "</td></tr></table>";
  s += "<h4>Last events</h4><ul>";
  const size_t total = logger_.count();
  const size_t start = total > 10 ? total - 10 : 0;
  for (size_t i = start; i < total; ++i) {
    const LogEntry& e = logger_.getFromOldest(i);
    s += "<li>[" + String(e.uptime_ms / 1000UL) + "s] " + String(e.event_type) + " / " + String(e.message) + "</li>";
  }
  s += "</ul></body></html>";
  return s;
}

String WebUi::controlPage() const {
  String s = htmlHead("Control");
  s += "<h3>CONTROL</h3>";
  s += "<form method='POST' action='/api/open'><button>Open barrier</button></form><br>";
  s += "<form method='POST' action='/api/close'><button>Close barrier</button></form><br>";
  s += "<form method='POST' action='/api/reset_error'><button>Reset error</button></form><br>";
  s += "<form method='POST' action='/api/test_relay'><button>Test relay</button></form><br>";
  s += "<form method='POST' action='/api/reboot'><button>Reboot device</button></form>";
  s += "</body></html>";
  return s;
}

String WebUi::logicPage() const {
  String s = htmlHead("Logic settings");
  s += "<h3>LOGIC SETTINGS</h3><form method='POST' action='/api/config'>";
  s += "Open mode <small>(PULSE=0, HOLD_UNTIL_LIMIT=1)</small><br><input name='open_mode' value='" + String((uint8_t)cfg_.open_mode) + "'><br>";
  s += "Open pulse duration ms<br><input name='open_pulse_ms' value='" + String(cfg_.open_pulse_ms) + "'><br>";
  s += "Open timeout ms<br><input name='open_timeout_ms' value='" + String(cfg_.open_timeout_ms) + "'><br>";
  s += "Open hold time ms<br><input name='open_hold_ms' value='" + String(cfg_.open_hold_ms) + "'><br>";
  s += "Close timeout ms<br><input name='close_timeout_ms' value='" + String(cfg_.close_timeout_ms) + "'><br>";
  s += "Extra wait before retry ms<br><input name='close_extra_ms' value='" + String(cfg_.close_extra_ms) + "'><br>";
  s += "Close retries<br><input name='close_retries' value='" + String(cfg_.close_retries) + "'><br>";
  s += "Relay logic active HIGH? (1/0)<br><input name='relay_active_high' value='" + String(cfg_.relay_invert ? 0 : 1) + "'><br>";
  s += "Open limit invert (1/0)<br><input name='open_limit_invert' value='" + String(cfg_.open_limit_invert ? 1 : 0) + "'><br>";
  s += "Close limit invert (1/0)<br><input name='close_limit_invert' value='" + String(cfg_.close_limit_invert ? 1 : 0) + "'><br>";
  s += "Debounce ms<br><input name='debounce_ms' value='" + String(cfg_.debounce_ms) + "'><br>";
  s += "Allow commands in ERROR (1/0)<br><input name='allow_commands_in_error' value='" + String(cfg_.allow_commands_in_error ? 1 : 0) + "'><br>";
  s += "Auto recover from error (1/0)<br><input name='auto_recover_from_error' value='" + String(cfg_.auto_recover_from_error ? 1 : 0) + "'><br>";
  s += "Repeat command policy <small>(0=ignore,1=refresh,2=queue)</small><br><input name='repeat_policy' value='" + String((uint8_t)cfg_.repeat_policy) + "'><br><br>";
  s += "<button>Save logic settings</button></form></body></html>";
  return s;
}

String WebUi::networkPage() const {
  String s = htmlHead("Network settings");
  s += "<h3>NETWORK SETTINGS</h3><form method='POST' action='/api/config'>";
  s += "DHCP enable (1/0)<br><input name='dhcp_enable' value='" + String(cfg_.dhcp_enable ? 1 : 0) + "'><br>";
  s += "Static IP<br><input name='static_ip' value='" + cfg_.static_ip.toString() + "'><br>";
  s += "Subnet mask<br><input name='subnet' value='" + cfg_.subnet.toString() + "'><br>";
  s += "Gateway<br><input name='gateway' value='" + cfg_.gateway.toString() + "'><br>";
  s += "DNS<br><input name='dns' value='" + cfg_.dns.toString() + "'><br>";
  s += "HTTP port<br><input name='http_port' value='" + String(cfg_.http_port) + "'><br>";
  s += "Trusted IP enable (1/0)<br><input name='trusted_ip_enabled' value='" + String(cfg_.trusted_ip_enabled ? 1 : 0) + "'><br>";
  s += "Trusted IP<br><input name='trusted_ip' value='" + cfg_.trusted_ip.toString() + "'><br>";
  s += "Shared token<br><input name='shared_token' value='" + cfg_.shared_token + "'><br><br>";
  s += "<button>Save network settings</button></form></body></html>";
  return s;
}

String WebUi::securityPage() const {
  String s = htmlHead("Security");
  s += "<h3>SECURITY</h3><form method='POST' action='/api/config'>";
  s += "Login<br><input name='login' value='" + cfg_.login + "'><br>";
  s += "Current password<br><input type='password' name='current_password'><br>";
  s += "Change password<br><input type='password' name='new_password'><br><small>Password is stored as SHA1 hash.</small><br><br>";
  s += "<button>Save security settings</button></form></body></html>";
  return s;
}

String WebUi::logsPage() const {
  String s = htmlHead("Logs");
  s += "<h3>LOGS</h3><table><tr><th>Uptime s</th><th>Type</th><th>Source</th><th>Message</th></tr>";
  for (size_t i = 0; i < logger_.count(); ++i) {
    const LogEntry& e = logger_.getFromOldest(i);
    s += "<tr><td>" + String(e.uptime_ms / 1000UL) + "</td><td>" + String(e.event_type) + "</td><td>" +
         String(e.source) + "</td><td>" + String(e.message) + "</td></tr>";
  }
  s += "</table></body></html>";
  return s;
}

String WebUi::loginPage(const String& msg) const {
  String s;
  s += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Login</title></head><body>";
  s += "<h3>Login</h3><p>" + msg + "</p><form method='POST' action='/login'>";
  s += "Login<br><input name='login'><br>Password<br><input type='password' name='password'><br><button>Sign in</button></form>";
  s += "</body></html>";
  return s;
}
