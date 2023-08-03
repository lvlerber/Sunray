// #include "../../src/esp32-expanders-hal-custom.h"
#include <Arduino.h>
#include <../../config.h>
#include <pinman.h>
#include "SD.h"
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
extern String cmd;
extern bool ju;
bool batterySwitch=false;
    extern  int8_t pin_to_channel[SOC_GPIO_PIN_COUNT];
void task_WebConsoleInputHandler(void *args)
{

  int pin = 99;
  int val=-1;
  std::string scmd = cmd.c_str();
  Serial.println(cmd);
  int komma = scmd.find(",");
  if (komma != std::string::npos) {
    cmd=scmd.substr(0,komma).c_str();
    val=  atoi(scmd.substr(komma+1).c_str());
  } 
  delay(1000);
  
  pin = atoi(cmd.c_str());
  Serial.println(pin);
  delay(1000);
  // we gaan de pinnen testen volgens de defines, zodat we direct weten of het end to end goed zit.
  switch (pin)
  {

  case pinMotorLeftPWM:
    printf("Test pinMotorLeftPWM O PWM\n");

    analogWrite(pinMotorLeftPWM, 255);
    delay(10000);
    analogWrite(pinMotorLeftPWM, 0);
    break;
  case pinBatterySwitch:
    printf("Switching pinBatterySwitch O D to %i\n",batterySwitch);
   pinMode(pinBatterySwitch, OUTPUT);
    digitalWrite(pinBatterySwitch, batterySwitch);
    batterySwitch= !batterySwitch;
    break;
  case pinGsmReset:
    printf("Test pinGsmReset O D\n");
   pinMode(pinGsmReset, OUTPUT);
    digitalWrite(pinGsmReset, 1);
    delay(10000);
    digitalWrite(pinGsmReset, 0);
    break;
  case pinMotorRightPWM:
    printf("Test pinMotorRightPWM O PWM\n");
  //  pinMode(pinMotorRightPWM, OUTPUT);
    analogWrite(pinMotorRightPWM, 124);
    delay(10000);
    analogWrite(pinMotorRightPWM, 0);
    break;
  case pinGsmRx:
    printf("Test pinGsmRx I D\n");
   pinMode(pinGsmRx, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinGsmRx));
      delay(1000);
    }
    break;
  case pinGsmTx:
    printf("Test pinGsmTx O D\n");
   pinMode(pinGsmTx, OUTPUT);
    digitalWrite(pinGsmTx, 1);
    delay(10000);
    digitalWrite(pinGsmTx, 0);
    break;
  case pinMotorMowPWM:
    printf("Test pinMotorMowPWM O PWM\n");
  //  pinMode(pinMotorMowPWM, OUTPUT);
    analogWrite(pinMotorMowPWM, 128);
    delay(10000);
    digitalWrite(pinMotorMowPWM,1);
    delay(10000);
    analogWrite(pinMotorMowPWM, 0);
    break;
  case pinSdSclk:
    printf("Test pinSdSclk O D\n");
   pinMode(pinSdSclk, OUTPUT);
    digitalWrite(pinSdSclk, 1);
    delay(10000);
    digitalWrite(pinSdSclk, 0);
    break;
  case pinSdMiso:
    printf("Test pinSdMiso I D\n");
   pinMode(pinSdMiso, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinSdMiso));
      delay(1000);
    }
    break;
  case pinSdMosi:
    printf("Test pinSdMosi O D\n");
   pinMode(pinSdMosi, OUTPUT);
    digitalWrite(pinSdMosi, 1);
    delay(10000);
    digitalWrite(pinSdMosi, 0);
    break;
  case pinGpsTx:
    printf("Test pinGpsTx O D\n");
   pinMode(pinGpsTx, OUTPUT);
    digitalWrite(pinGpsTx, 1);
    delay(10000);
    digitalWrite(pinGpsTx, 0);
    break;
  case pinGpsRx:
    printf("Test pinGpsRx I D\n");
   pinMode(pinGpsRx, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinGpsRx));
      delay(1000);
    }
    break;
  case pinSonarRightEcho:
    printf("Test pinSonarRightEcho I D\n");
   pinMode(pinSonarRightEcho, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinSonarRightEcho));
      delay(1000);
    }
    break;
  case pinBuzzer:


    // for (int i=0;i<40;i++){printf("  %i-%i",analogGetChannel(i),pin_to_channel[i]);}
    
    printf("Test pinBuzzer O D\n");
    printf("hangt aan channel:%i\n",analogGetChannel(pinBuzzer));
    analogWrite(pinBuzzer, 0);
    ledcSetup(8,4400,8);
    ledcWrite(8,128);
    delay(1000);
    ledcWriteTone(analogGetChannel(pinBuzzer),2200);
    delay(1000);    
    analogWrite(pinBuzzer, 0);    
    break;
  case pinIrqExp:
    printf("Test pinIrqExp IU D\n");
   pinMode(pinIrqExp, INPUT_PULLUP);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinIrqExp));
      delay(1000);
    }
    break;
  case pinSonarLeftEcho:
    printf("Test pinSonarLeftEcho I D\n");
   pinMode(pinSonarLeftEcho, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinSonarLeftEcho));
      delay(1000);
    }
    break;
  case pinSonarCenterEcho:
    printf("Test pinSonarCenterEcho I D\n");
   pinMode(pinSonarCenterEcho, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinSonarCenterEcho));
      delay(1000);
    }
    break;
  case pinMotorEnable:
    printf("Test pinMotorEnable O D\n");
   pinMode(pinMotorEnable, OUTPUT);
    digitalWrite(pinMotorEnable, 1);
    delay(10000);
    digitalWrite(pinMotorEnable, 0);
    break;
  case pinSonarLeftTrigger:
    printf("Test pinSonarLeftTrigger O D\n");
   pinMode(pinSonarLeftTrigger, OUTPUT);
    digitalWrite(pinSonarLeftTrigger, 1);
    delay(10000);
    digitalWrite(pinSonarLeftTrigger, 0);
    break;
  case pinSonarCenterTrigger:
    printf("Test pinSonarCenterTrigger O D\n");
   pinMode(pinSonarCenterTrigger, OUTPUT);
    digitalWrite(pinSonarCenterTrigger, 1);
    delay(10000);
    digitalWrite(pinSonarCenterTrigger, 0);
    break;
  case pinMotorMowEnable:
    printf("Test pinMotorMowEnable O D\n");
   pinMode(pinMotorMowEnable, OUTPUT);
    digitalWrite(pinMotorMowEnable, 1);
    delay(10000);
    digitalWrite(pinMotorMowEnable, 0);
    break;
  case pinMotorMowDir:
    printf("Test pinMotorMowDir O D\n");
   pinMode(pinMotorMowDir, OUTPUT);
    digitalWrite(pinMotorMowDir, 1);
    delay(10000);
    digitalWrite(pinMotorMowDir, 0);
    break;
  case pinSonarRightTrigger:
    printf("Test pinSonarRightTrigger O D\n");
   pinMode(pinSonarRightTrigger, OUTPUT);
    digitalWrite(pinSonarRightTrigger, 1);
    delay(10000);
    digitalWrite(pinSonarRightTrigger, 0);
    break;
  case pinMotorLeftDir:
    printf("Test pinMotorLeftDir O D\n");
   pinMode(pinMotorLeftDir, OUTPUT);
    digitalWrite(pinMotorLeftDir, 1);
    delay(10000);
    digitalWrite(pinMotorLeftDir, 0);
    break;
  case pinMotorRightDir:
    printf("Test pinMotorRightDir O D\n");
   pinMode(pinMotorRightDir, OUTPUT);
    digitalWrite(pinMotorRightDir, 1);
    delay(10000);
    digitalWrite(pinMotorRightDir, 0);
    break;
  case pinMotorRightFault:
    printf("Test pinMotorRightFault I D\n");
   pinMode(pinMotorRightFault, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinMotorRightFault));
      delay(1000);
    }
    break;
  case pinMotorMowFault:
    printf("Test pinMotorMowFault I D\n");
   pinMode(pinMotorMowFault, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinMotorMowFault));
      delay(1000);
    }
    break;
  case pinMotorLeftFault:
    printf("Test pinMotorLeftFault I D\n");
   pinMode(pinMotorLeftFault, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinMotorLeftFault));
      delay(1000);
    }
    break;
  case pinBumperRight:
    printf("Test pinBumperRight IU D\n");
   pinMode(pinBumperRight, INPUT_PULLUP);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinBumperRight));
      delay(1000);
    }
    break;
  case pinBumperLeft:
    printf("Test pinBumperLeft IU D\n");
   pinMode(pinBumperLeft, INPUT_PULLUP);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinBumperLeft));
      delay(1000);
    }
    break;
  case pinOdometryRight:
    printf("Test pinOdometryRight IU D\n");
   pinMode(pinOdometryRight, INPUT_PULLUP);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinOdometryRight));
      delay(1000);
    }
    break;
  case pinOdometryLeft:
    printf("Test pinOdometryLeft IU D\n");
   pinMode(pinOdometryLeft, INPUT_PULLUP);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", digitalRead(pinOdometryLeft));
      delay(1000);
    }
    break;
  case pinMotorRightSense:
    printf("Test pinMotorRightSense I ADC\n");
   pinMode(pinMotorRightSense, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinMotorRightSense));
      delay(1000);
    }
    break;
  case pinMotorLeftSense:
    printf("Test pinMotorLeftSense I ADC\n");
   pinMode(pinMotorLeftSense, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinMotorLeftSense));
      delay(1000);
    }
    break;
  case pinBatteryVoltage:
    printf("Test pinBatteryVoltage I ADC\n");
   pinMode(pinBatteryVoltage, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinBatteryVoltage));
      delay(1000);
    }
    break;
  case pinMotorMowSense:
    printf("Test pinMotorMowSense I ADC\n");
   pinMode(pinMotorMowSense, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinMotorMowSense));
      delay(1000);
    }
    break;
  case pinChargeVoltage:
    printf("Test pinChargeVoltage I ADC\n");
   pinMode(pinChargeVoltage, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinChargeVoltage));
      delay(1000);
    }
    break;
  case pinChargeCurrent:
    printf("Test pinChargeCurrent I ADC\n");
   pinMode(pinChargeCurrent, INPUT);
    for (int i = 0; i < 10; i++)
    {
      printf("gelezen waarde %i\n", analogRead(pinChargeCurrent));
      delay(1000);
    }
    break;
  case pinChargeRelay:
    printf("Test pinChargeRelay O D*\n");
   pinModeX(pinChargeRelay, OUTPUT);
    analogWrite(pinChargeRelay, 255);
    delay(200000);
    analogWrite(pinChargeRelay, 0);
    break;
  case 100:
    printf("Before initializing SD the cardtype is %i\n",SD.cardType());
    listDir(SD,"/",0);
    printf("is t gelukt %i",SD.begin(0,SPI,4000000U,"/sd",10,false));
    printf("After initializing SD the cardtype is %i\n",SD.cardType());
        listDir(SD,"/",0);
    break;
  case 101:
    { ju=true;break;}
  default:
    printf("There is no pin defined as %s\n", cmd.c_str());
    break;
  }
  printf("einde test");
  vTaskDelete(NULL);
}