#include <Arduino.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"
#include "net_server.h"
#include "netping_compat.h"
#include "storage.h"
#include "web_ui.h"

Logger gLogger;
AppConfig gConfig;
Storage gStorage(gLogger);
BarrierController gBarrier(gLogger, gConfig);
NetPingCompat gNetPing(gBarrier, gLogger);
WebUi gWebUi(gBarrier, gConfig, gLogger);
NetServer gNetServer(gConfig, gBarrier, gStorage, gLogger, gNetPing, gWebUi);

void setup() {
  gLogger.begin();
  gLogger.log("INFO", "boot", "boot start");

  gStorage.begin();
  gStorage.load(gConfig);

  gBarrier.begin();
  gNetServer.begin();

  gLogger.log("INFO", "boot", "boot complete");
}

void loop() {
  gBarrier.update();
  gNetServer.update();
}
