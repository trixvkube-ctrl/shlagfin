#include "netping_compat.h"

bool NetPingCompat::getParam(const String& query, const String& key, String& value, bool& hasValue) {
  const int start = query.indexOf(key);
  if (start < 0) return false;
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();
  const int eq = query.indexOf('=', start);
  if (eq > 0 && eq < end) {
    value = query.substring(eq + 1, end);
    hasValue = true;
  } else {
    value = "";
    hasValue = false;
  }
  return true;
}

bool NetPingCompat::handleIoCgi(const String& query, String& outBody) {
  String ioVal;
  bool hasValue = false;
  if (!getParam(query, "io1", ioVal, hasValue)) {
    outBody = "io_result('error')";
    return false;
  }

  String modeVal;
  bool hasModeVal = false;
  getParam(query, "mode", modeVal, hasModeVal);

  if (!hasValue) {
    const int state = barrier_.relayIsActive() ? 1 : 0;
    outBody = "io_result('ok', -1, " + String(state) + ", 0)";
    logger_.log("INFO", "netping", "io1 state read");
    return true;
  }

  ioVal.trim();
  if (ioVal == "1") {
    barrier_.forceRelay(true, 0, "netping");
    outBody = "io_result('ok')";
  } else if (ioVal == "0") {
    barrier_.forceRelay(false, 0, "netping");
    outBody = "io_result('ok')";
  } else if (ioVal == "f") {
    barrier_.forceRelay(!barrier_.relayIsActive(), 0, "netping");
    outBody = "io_result('ok')";
  } else if (ioVal.startsWith("f,")) {
    const int sec = ioVal.substring(2).toInt();
    const bool newState = !barrier_.relayIsActive();
    barrier_.forceRelay(newState, sec > 0 ? static_cast<uint32_t>(sec) * 1000UL : 0, "netping");
    outBody = "io_result('ok')";
  } else {
    outBody = "io_result('error')";
    return false;
  }

  char msg[48];
  snprintf(msg, sizeof(msg), "received io1=%s mode=%s", ioVal.c_str(), hasModeVal ? modeVal.c_str() : "-");
  logger_.log("INFO", "netping", msg);
  return true;
}
