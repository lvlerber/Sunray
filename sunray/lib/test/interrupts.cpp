#include <Arduino.h>
#include "../../config.h"

void  echoLeft() {
    	int btn_state = gpio_get_level((gpio_num_t) pinSonarLeftEcho);
	uint32_t teller=esp_log_timestamp();

	ets_printf(" %d button %d is %d \n",teller, btn_state, pinSonarLeftEcho);
    Serial.println("echoleft");
}
void IRAM_ATTR echoCenter() {
    	int btn_state = gpio_get_level((gpio_num_t) pinSonarCenterEcho);
	uint32_t teller=esp_log_timestamp();

	ets_printf(" %d button %d is %d \n",teller, btn_state, pinSonarCenterEcho);
}
void IRAM_ATTR echoRight() {
    	int btn_state = gpio_get_level((gpio_num_t) pinSonarRightEcho);
	uint32_t teller=esp_log_timestamp();

	ets_printf(" %d button %d is %d \n",teller, btn_state, pinSonarRightEcho);
}

void  OdometryLeftISR2() {
        	int btn_state = gpio_get_level((gpio_num_t) pinOdometryLeft);
	uint32_t teller=esp_log_timestamp();

	ets_printf(" %d button %d is %d \n",teller, btn_state, pinOdometryLeft);
}


void IRAM_ATTR OdometryRightISR2() {
       	int btn_state = gpio_get_level((gpio_num_t) pinOdometryRight);
	uint32_t teller=esp_log_timestamp();

	ets_printf(" %d button %d is %d \n",teller, btn_state, pinOdometryRight);
}
void startInterrupts(){
  pinMode(pinSonarLeftEcho , INPUT);
  pinMode(pinSonarCenterEcho , INPUT);
  pinMode(pinSonarRightEcho , INPUT);
  pinMode(pinOdometryRight , INPUT_PULLUP);
  pinMode(pinOdometryLeft , INPUT_PULLUP);
  attachInterrupt(pinSonarLeftEcho, echoLeft, CHANGE);
  attachInterrupt(pinSonarCenterEcho, echoCenter, CHANGE);
  attachInterrupt(pinSonarRightEcho, echoRight, CHANGE);
  attachInterrupt(pinOdometryLeft, OdometryLeftISR2, CHANGE);  
  attachInterrupt(pinOdometryRight, OdometryRightISR2, CHANGE);  
}