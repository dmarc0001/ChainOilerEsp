# ChainOilerEsp
Chain oiler with esp controller and WLAN config (OTA Updates)


Fully on
Behavior:
Everything on
Consumption:
about 70 mA
When to use:
Usually never
How to use with Arduino:
extern "C" {
  #include "user_interface.h"
}
void setup() {
  wifi_set_sleep_type(NONE_SLEEP_T);
}

Fully on, no Wifi configured
Fully on, Wifi configured and reachable
Auto Modem Sleep
Behavior:
After an idle timeout of 10 seconds the WiFi hardware will be shut down. It wakes up every <100 ms * Access Point DTIM interval> to check for incoming data. Everything else powered up.
Note: If the WiFi connection is not configured or not reachable, the chip will not sleep at all.

Note: Auto Modem Sleep will not happen if there are no delay() or only delay(0) calls in the code.

Consumption:
If Wifi is idle, about 70 mA until timeout, then about 18 mA.
When to use:
If you need Wifi connectivity and Auto Light Sleep didn’t work for you.
How to use with Arduino:
Do nothing, this is the default.

Auto Light Sleep
Behavior:
After an idle timeout of 10 seconds the WiFi hardware will be shut down. It wakes up every <100 ms * Access Point DTIM interval> to check for incoming data. Furthermore while the program is running delay() the system clock will be shut down too.
Note: If the WiFi connection is not configured or not reachable, the chip will not sleep at all.

Note: Auto Light Sleep will not happen if there are no delay() or only delay(0) calls in the code.

Consumption:
If Wifi is idle, about 70 mA until timeout, then about 2 mA.
When to use:
Recommended power state if you need Wifi connectivity.
Because of the halted system clock some peripherals might not work in this mode, use Auto Modem Sleep in those cases.

How to use with Arduino:
extern "C" {
  #include "user_interface.h"
}
void setup() {
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

Forced Modem Sleep
Behavior:
WiFi hardware is off, everything else powered up.
Consumption:
about 16 mA
When to use:
When you don’t need WiFi
How to use with Arduino:
WiFi.forceSleepBegin(); // Wifi off
WiFi.forceSleepWake(); // Wifi on

If you also use ESP.deepSleep(), you can add RF_DISABLED as the second argument, this prevents the Wifi hardware from booting up after deep sleep. (See below.) Note that there is no way to enable it again without deep sleeping again.


Forced Light Sleep
Behavior:
Everything is halted until woken up by a GPIO interrupt.
Consumption:
about 1 mA
When to use:
When you want to sleep until an external event happens and you want to keep the current execution state.
