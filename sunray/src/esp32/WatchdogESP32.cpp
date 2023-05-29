
// https://github.com/adafruit/Adafruit_SleepyDog/blob/master/utility/WatchdogSAMD.cpp

// Be careful to use a platform-specific conditional include to only make the
// code visible for the appropriate platform.  Arduino will try to compile and
// link all .cpp files regardless of platform.
#if defined(ESP32)
#include <esp_task_wdt.h>
#include "WatchdogESP32.h"
// #include "../../config.h"


int WatchdogESP32::enable(int maxPeriodMS, bool isForSleep) { 
  
  // Enable the watchdog with a period up to the specified max period in
  // milliseconds.

 

  if (!_initialized)
    _initialize_wdt();    

  esp_task_wdt_init(maxPeriodMS, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  
  return (1); // WDT cycles -> ms
}

void WatchdogESP32::reset() {
       esp_task_wdt_reset();
}

uint8_t WatchdogESP32::resetCause() {
  return 0;
}

void WatchdogESP32::disable() {
  esp_task_wdt_delete(NULL); //add current thread to WDT watch
}


int WatchdogESP32::sleep(int maxPeriodMS) {

  return 0;
}

void WatchdogESP32::_initialize_wdt() {
  // One-time initialization of watchdog timer.
  //esp_int_wdt_init();  This is called in the init when active in configuration. 
  // Anyway, it isn't in the headerfile
  _initialized = true;
}


WatchdogESP32 watchdog;


void watchdogReset(){
  watchdog.reset();
}

void watchdogEnable(uint32_t timeout){
  watchdog.enable(timeout);
}

#endif // defined(esp32)

