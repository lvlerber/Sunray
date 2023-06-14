// Ardumower Sunray
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)

// MC33926 brushed driver: Why using 3900 Hz PWM frequency?
// At 3900 Hz, our MC33926 motor drivers give linear motor current results for all speeds (PWM/odometry values)
// http://wiki.ardumower.de/images/f/f9/Pwm_3khz.png
// At 490 Hz or 20 kHz, the reported values are not so good:
// http://wiki.ardumower.de/images/e/e0/Pwm_490hz.png
// http://wiki.ardumower.de/images/8/84/Pwm_20khz.png

#include "Wire.h"
#include "pinman.h"
#include "config.h"
#include "freertos/queue.h"
#include <functional>
#include <iostream>
#include <iomanip>
#include <bitset>

#ifdef ESP32

void analogWrite(uint8_t pin, int value) 
#include "WireMutex.h"
#include "MCP23017.h"

#include "Adafruit_MCP23X17.h"
#include "Adafruit_PCF8591.h"

void printAllRegisters(MCP23017 mcp, int adres);
static inline uint16_t __mapResolution(uint16_t value, uint8_t from, uint8_t to);
MCP23017 mcp1(MCP23017_1);
MCP23017 mcp2(MCP23017_2);
Adafruit_MCP23X17 digExp1;
Adafruit_MCP23X17 digExp2;
Adafruit_PCF8591 anaExp1;
Adafruit_PCF8591 anaExp2;

// PinManager pinMan;  //is defined in robot.cpp

static xQueueHandle expander_evt_queue = xQueueCreate(10, sizeof(uint32_t));

void expanderISR()
{
  uint32_t teller;
  teller = micros();
  if (teller = micros())
  {
  }
  xQueueSendFromISR(expander_evt_queue, &teller, NULL);
}

static void expander_evt_queue_handler(void *arg)
{
  uint32_t teller;
  for (;;)
  {
    if (xQueueReceive(expander_evt_queue, &teller, portMAX_DELAY))
    {
      uint8_t lastIrqPin1 = 255;
      uint8_t lastIrqPin2 = 255;
      // printf("event process started %d - %lu\n", teller, micros());

        while (!digitalRead(pinIrqExp))
        {
        // printf("at %d GPIO[%d] intr, val: %d\n", teller, pinIrqExp, digitalRead(pinIrqExp));
        uint8_t regval = mcp1.readRegister(MCP23017Register::INTF_B);
        // printf("mcp1 INTF_B : %02x\n", regval);
        uint8_t regval1 = mcp1.readRegister(MCP23017Register::INTCAP_B);
        // from here the interrupt is cleared
        for (int i = 0; i < 8; i++)
        {
          if bitRead (regval, i)
          {
            lastIrqPin1 = i + MINDIGX1 + 8;
              // This is the place where you call the interruptroutine for this pin         
              //since all interrupt pins are now native esp32 pins, this code will never be executed
              // this was meant to test the feasability of reading the odometry
              // Every interrupt handling (like above) takes several hundreds of µs, blocking the interrupts for the other wheel
            if (lastIrqPin1==pinOdometryLeft){

              //ticksLeft++;
            } else if (lastIrqPin1==pinOdometryRight){
              // ticksRight++;
            }            // printf("mcp1 INTCAP_B : %02x\n", regval1);
            // regval1 = mcp1.readRegister(MCP23017Register::GPIO_B);
            // printf("mcp1   GPIO_B : %02x\n", regval1);
          }
        }
        
        // not correct.  If the interrupt can be on port A then you have to test it the same way as 
        // and if interrupts are possible on mcp2 and on the same pinIrqExp, you have to test this too

        // if (lastIrqPin1 == 255 || !digitalRead(pinIrqExp))
        // {
        //   uint8_t regval = mcp1.readRegister(MCP23017Register::INTF_A);
        //   printf("mcp1 INTF_A : %02x\n", regval);
        //   if (regval)
        //   {
        //     regval = mcp1.readRegister(MCP23017Register::INTCAP_A);
        //     printf("mcp1 INTCAP_A : %02x\n", regval);
        //     lastIrqPin1 = regval + MINDIGX1;
        //   }
        // }

        // uint8_t lastIrqPin1=digExp1.getLastInterruptPin();
        // uint8_t lastIrqPin2=0;
        // printf("last interrupt on digital expander 1 and 2: %d - %d\n", lastIrqPin1, lastIrqPin2);
        // printf("exp1 values : %04X - %04X \n", digExp1.readGPIOAB(), digExp1.readGPIOAB());
        // printf("teller %d\n", teller);
        // printAllRegisters(mcp1, 1);
        // printAllRegisters(mcp2, 2);
      }
        giveWire();
      }
      else
      {
        printf("#@#{[[#@@@\n");
      };
    }
  }

void PinManager::begin()
{
  // PWM frequency
  analogWriteFrequency(3900);
  // Here we check which expanders are present and we create an object for each
  // Wire should be started
  takeWire();
  if (MCP23017_1)
  {
    if (!digExp1.begin_I2C(MCP23017_1))
    {
      Serial.println("the expander defined as MCP23017_1 was not found");
    }
  }
  if (MCP23017_2)
  {
    if (!digExp2.begin_I2C(MCP23017_2))
    {
      Serial.println("the expander defined as MCP23017_2 was not found");
    }
  }
  if (PCF8591_1)
  {
    if (!anaExp1.begin(PCF8591_1))
    {
      Serial.println("the expander defined as PCF8591_1 was not found");
    }
  }
  if (PCF8591_2)
  {
    if (!anaExp2.begin(PCF8591_2))
    {
      Serial.println("the expander defined as PCF8591_2 was not found");
    }
  }
  // This also happens to be the place to add the interrupts for expanders

  if (pinIrqExp)
  {
    pinMode(pinIrqExp, INPUT_PULLUP);

    // OPTIONAL - call this to override defaults
    // mirror INTA/B so only one wire required
    // active drive so INTA/B will not be floating
    // INTA/B will be signaled with a LOW

    // here we use open drain so that every expander can only pull down
    // on esp32 if we use pin 36 as interrupt pin, an external pull-up is needed!!!
    digExp1.setupInterrupts(true, true, LOW);
    // digExp2.setupInterrupts(true, true, LOW);
    // activate interrupt from expanders and start task to handle the events

    xTaskCreate(&expander_evt_queue_handler, "treatQueue", 2048, NULL, 10, NULL);
    mcp1.writeRegister(MCP23017Register::IODIR_A, 0x00000000);
    mcp1.writeRegister(MCP23017Register::IODIR_B, 0x00000000);
    mcp1.writeRegister(MCP23017Register::GPINTEN_A, 0x00000000);
    mcp1.writeRegister(MCP23017Register::GPINTEN_B, 0x00000000);
    mcp1.writeRegister(MCP23017Register::GPPU_A, 0x00000000);
    mcp1.writeRegister(MCP23017Register::GPPU_B, 0x00000000);
    mcp1.writeRegister(MCP23017Register::INTCON_A, 0x00000000);
    mcp1.writeRegister(MCP23017Register::INTCON_B, 0x00000000);    
    mcp1.writeRegister(MCP23017Register::DEFVAL_A, 0x00000000);
    mcp1.writeRegister(MCP23017Register::DEFVAL_B, 0x00000000);
    // printAllRegisters(mcp1,1);
    // if (MCP23017_2) {printAllRegisters(mcp2,2);};
    attachInterrupt(pinIrqExp, expanderISR, FALLING);
    Serial.println("PinManager started");
  }
  giveWire();
  // Manual assignment of channels to insure that buzzer has a different timer
  ledcAttachPin(pinBuzzer, 8);
  ledcAttachPin(pinMotorLeftPWM,2);
  ledcAttachPin(pinMotorRightPWM,3);
  ledcAttachPin(pinMotorMowPWM,4);    
}
int getChannel(uint8_t pin)
{
  switch (pin)
  {
  case pinMotorLeftPWM:
    return 0;
  case pinMotorRightPWM:
    return 1;
  case pinMotorMowPWM:
    return 2;
  default:
    printf("this pin is not configured for PWM: ", pin);
    return 99;
  }
}
/**
 * \brief Configures the specified pin to behave either as an input or an output. See the description of digital pins for details.
 *        ESP32 version
 * \param Pin The number of the pin whose mode you wish to set
 * \param Mode Can be INPUT, OUTPUT, INPUT_PULLUP or INPUT_PULLDOWN
 */
void pinModeX(uint8_t pin, uint8_t mode)
{
  if (pin >= MINANAX1)
  {
    if (((pin <= MINANAX1 + 3 || (pin >= MINANAX2 && pin <= MINANAX2 + 3)) && mode == INPUT)

        || (((pin == MINANAX1 + 4) || (pin == MINANAX2 + 4)) && mode == OUTPUT))
    {
    }
    else
    {
      Serial.printf("this pin doesn't have the mode corresponding to the hardware of a PCF8591: %i\n", pin);
    }
    if (pin == MINANAX1 + 4)
    {
      takeWire();
      anaExp1.enableDAC(true);
      giveWire();
    }
    else if (pin == MINANAX2 + 4)
    {
      takeWire();
      anaExp2.enableDAC(true);
      giveWire();
    }
    return;
  }
  if (pin < MINDIGX1)
  {
    // normal pins, except that PWM pins are defined as output without extra
    // We should never come here
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    // if (pin != pinMotorRightPWM && pin != pinMotorLeftPWM && pin != pinMotorMowPWM)
    // {
    //   pinMode(pin, mode);
    // }
    // else
    // {
    //   static int nextChannel;
    //   const int PWMFreq = 3900; /* 3.9 KHz */
    //   const int PWMResolution = 8;
    //   const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
    //   ledcSetup(nextChannel, PWMFreq, PWMResolution);
    //   ledcAttachPin(pin, getChannel(pin));
    // }
  }
  else
  {
    takeWire();
    if (pin < MINDIGX2)
    {
      digExp1.pinMode(pin - MINDIGX1, mode);
    }
    else
    {
      digExp2.pinMode(pin - MINDIGX2, mode);
    }
    giveWire();
  }
}
int digitalReadX(uint8_t pin)
{
  if (pin < MINDIGX1)
  {
    //should not come here
    return digitalRead(pin);
  }
  else
  {
    int val = 99;
    takeWire();
    if (pin < MINDIGX2)
    {
      val = digExp1.digitalRead(pin - MINDIGX1);
    }
    else if (pin < MINANAX1)
    {
      val = digExp2.digitalRead(pin - MINDIGX2);
    }
    giveWire();
    return val;
  }
}

void analogWriteX(uint8_t pin, int value)
{
  if (pin == MINANAX1 + 4)
  {
    anaExp1.analogWrite(value);
  }
  else if (pin == MINANAX2 + 4)
  {
    anaExp2.analogWrite(value);
  }
  else
  {

    if (getChannel(pin) > 15)
    {
      printf("This pin is not configured to support DAC : %i\n", pin);
    }
    else
    {
      ledcWrite(getChannel(pin), value);
    }
  }
};

void digitalWriteX(uint8_t pin, uint8_t val)
{
  if (pin >= MINANAX1)
  {
    if (pin == MINANAX1 + 4 || pin == MINANAX2 + 4)
    {
      analogWriteX(pin, 255);
    }
    else
    {
      Serial.print(F("terrible mistake to write analog digital input or output: "));
      Serial.println(pin);
    }
    return;
  }
  if (pin < MINDIGX1)
  {
    // should not come here
    digitalWrite(pin, val);
  }
  else
  {
    takeWire();
    if (pin < MINDIGX2)
    {
      digExp1.digitalWrite(pin - MINDIGX1, val);
    }
    else
    {
      digExp2.digitalWrite(pin - MINDIGX2, val);
    }
    giveWire();
  }
}

// void PinManager::attachInterruptX(uint8_t pin,  std::function<void(void)> intRoutine, int mode ){
void attachInterruptX(uint8_t pin, void (*intRoutine)(), int mode)
{
  if (pin >= MINANAX1)
  {
    Serial.println("terrible mistake to attach analog interrupt");
    return;
  }

  if (pin < MINDIGX1)
  {
    // should not come here
    attachInterrupt(pin, intRoutine, mode);
    printf("attached interrupt for pin %i with mode %i\n ", pin, mode);
  }
  else
  {
    takeWire();
    if (pin < MINDIGX2)
    {
      digExp1.setupInterruptPin(pin - MINDIGX1, mode);
      // Serial.printf("interrupt set for pin %d with mode %d", pin - MINDIGX1, mode);
      std::cout << "na attach interrupt " << std::dec << (int)+pin << " met mode " << +mode << " in mcp1" << std::endl
                << std::flush;
      // printAllRegisters(mcp1, 1);
      /**
       * Normally the interruptroutine should be linked to the pin here.  
       * As it is not possible to pass parameters, and with the specific limited activity for the odometry interrupts,
       * that is hardcoded in the interrupt process of the expander.  The routines 
      */
    }
    else
    {
      digExp2.setupInterruptPin(pin - MINDIGX2, mode);
      std::cout << "na attach interrupt " << std::dec << (int)+pin << " met mode " << +mode << " in mcp2" << std::endl
                << std::flush;
      printAllRegisters(mcp2, 2);
    }
    giveWire();
  }
};

uint16_t analogReadX(uint8_t pin)
{
  // printf("in analog readx for pin %i\n", pin);
  if (pin >= MINANAX1 && pin <= MINANAX1 + 3)
  {
    takeWire();
    uint8_t val = anaExp1.analogRead(pin - MINANAX1);
    // printf("gelezen: %i", val);
    giveWire();
    return (uint16_t)__mapResolution(val, 8, pinMan.requestedResolution);
  }
  else if (pin >= MINANAX2 && pin <= MINANAX2 + 3)
  {
    takeWire();
    uint8_t val = anaExp2.analogRead(pin - MINANAX2);
    // printf("gelezen: %i", val);
    giveWire();
    return (uint16_t)__mapResolution(val, 8, pinMan.requestedResolution);
  }
  else if (pin < MINDIGX1)
  {
    //should not come here
    return analogRead(pin);
  }
  else
  {
    printf("This pin doens't support ADC : %i\n", pin);
    return 9999;
  }
};


void setDebounce(int pin, int usecs); // reject spikes shorter than usecs on pin


void PinManager::analogWrite( uint32_t ulPin, uint32_t ulValue, byte pwmFreq )  {
  // Kept for compatibility with sunray
  ::analogWrite((uint8_t)ulPin,(int)ulValue);
} ; 

void PinManager::debug()
{
  takeWire();
  uint16_t valexp1 = digExp1.readGPIOAB();
  // uint16_t  valexp2=digExp2.readGPIOAB();
  giveWire();
  printf("debug exp1 values : %X - %X \t\t ", valexp1, valexp1);
  std::cout << std::bitset<16>(valexp1) << "\t" << std::bitset<16>(valexp1)
            << std::endl
            << std::flush;
}
std::string translateRegister(uint8_t reg)
{
  switch ((MCP23017Register)reg)
  {
  case MCP23017Register::IODIR_A:
    return "IODIR_A  ";
    break;
  case MCP23017Register::IODIR_B:
    return "IODIR_B  ";
    break;
  case MCP23017Register::IPOL_A:
    return "IPOL_A   ";
    break;
  case MCP23017Register::IPOL_B:
    return "IPOL_B   ";
    break;
  case MCP23017Register::GPINTEN_A:
    return "GPINTEN_A";
    break;
  case MCP23017Register::GPINTEN_B:
    return "GPINTEN_B";
    break;
  case MCP23017Register::DEFVAL_A:
    return "DEFVAL_A ";
    break;
  case MCP23017Register::DEFVAL_B:
    return "DEFVAL_B ";
    break;
  case MCP23017Register::INTCON_A:
    return "INTCON_A ";
    break;
  case MCP23017Register::INTCON_B:
    return "INTCON_B ";
    break;
  case MCP23017Register::IOCON:
    return "IOCON    ";
    break;
  case MCP23017Register::GPPU_A:
    return "GPPU_A   ";
    break;
  case MCP23017Register::GPPU_B:
    return "GPPU_B   ";
    break;
  case MCP23017Register::INTF_A:
    return "INTF_A   ";
    break;
  case MCP23017Register::INTF_B:
    return "INTF_B   ";
    break;
  case MCP23017Register::INTCAP_A:
    return "INTCAP_A ";
    break;
  case MCP23017Register::INTCAP_B:
    return "INTCAP_B ";
    break;
  case MCP23017Register::GPIO_A:
    return "GPIO_A   ";
    break;
  case MCP23017Register::GPIO_B:
    return "GPIO_B   ";
    break;
  case MCP23017Register::OLAT_A:
    return "OLAT_A   ";
    break;
  case MCP23017Register::OLAT_B:
    return "OLAT_B   ";
    break;
  default:
    return "";
    break;
  }
}
void printAllRegisters(MCP23017 mcp, int adres)
{
  std::cout << "resultaten voor bank " << std::dec << adres << std::endl
            << std::flush;
  takeWire();
  for (int i = 0; i < 0x16; i++)
  {
    std::cout << "register " << +i << "\t0x" << /*std::setfill('0') << std::setw(2)<<*/ std::hex << +i << " \t"
              << translateRegister(i) << "\t " << std::bitset<8>(mcp.readRegister((MCP23017Register)i))
              << std::endl
              << std::flush;
  }
  giveWire();
}

/*
 * \brief Set the resolution of analogRead return values. Default is 10 bits (range from 0 to 1023).
 *
 * \param res
 */
void analogReadResolutionX(uint8_t res)
{
  printf("analogReadResolution called with param %i",res);
  pinMan.requestedResolution = res;
  // ::analogReadResolution(res);
};
static inline uint16_t __mapResolution(uint16_t value, uint8_t from, uint8_t to)
{
  if (from == to)
  {
    return value;
  }
  if (from > to)
  {
    return value >> (from - to);
  }
  return value << (to - from);
}
#endif
#ifdef __SAMD51__
#include "wiring_private.h"
#endif

// --- pwm frequency ---
#if defined(MOTOR_DRIVER_BRUSHLESS) // ---- brushless motors use 29300 Hz -----
#if defined(__SAMD51__)
#define TCC_CTRLA_PRESCALER TCC_CTRLA_PRESCALER_DIV16 // PWM base frequency
#define TCC_TOP 0xFF                                  // PWM resolution
#elif defined(_SAM3XA_)
#define PWM_FREQUENCY 29300
#define TC_FREQUENCY 29300
#endif
#else // --------- brushed motors use 3900 Hz ---------------
#if defined(__SAMD51__)
#define TCC_CTRLA_PRESCALER TCC_CTRLA_PRESCALER_DIV64 // PWM base frequency
#define TCC_TOP 0xFF                                  // PWM resolution
#elif defined(_SAM3XA_)
#define PWM_FREQUENCY 3900
#define TC_FREQUENCY 3900
#elif defined(ESP32)
#define PWM_FREQUENCY 3900
#define TC_FREQUENCY 3900
#endif
static uint8_t TCChanEnabled[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static int _writeResolution = 8;
#endif

#if !defined(ESP32)
static uint8_t PWMEnabled = 0;
static uint8_t pinEnabled[PINS_COUNT];
static uint8_t TCChanEnabled[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

#if defined(__SAMD51__)
static int _writeResolution = 12;
static int _dacResolution = 12;
static bool dacEnabled[] = {false, false};
#elif defined(_SAM3XA_)
static int _writeResolution = 8;
// static int _dacResolution = 10;
#endif

void PinManager::setDebounce(int pin, int usecs)
{ // reject spikes shorter than usecs on pin
#if defined(_SAM3XA_)
  // CMSIS cortex m3 (sam3x8e)
  // C:\Users\alex\AppData\Local\Arduino15\packages\arduino\hardware\...
  //   sam\1.6.12\system\CMSIS\Device\ATMEL\sam3xa\include\component\component_pio.h
  // findstr /s /i "PIO_DIFSR" *.h

  if (usecs)
  {
    g_APinDescription[pin].pPort->PIO_IFER = g_APinDescription[pin].ulPin;
    g_APinDescription[pin].pPort->PIO_DIFSR |= g_APinDescription[pin].ulPin;
  }
  else
  {
    g_APinDescription[pin].pPort->PIO_IFDR = g_APinDescription[pin].ulPin;
    g_APinDescription[pin].pPort->PIO_DIFSR &= ~g_APinDescription[pin].ulPin;
    return;
  }
  int div = (usecs / 31) - 1;
  if (div < 0)
    div = 0;
  if (div > 16383)
    div = 16383;
  g_APinDescription[pin].pPort->PIO_SCDR = div;
#else // __SAMD51__
  // no implementation found for SAMD
  // CMSIS cortex m4f  (samd51p20)
  // C:\Users\alex\AppData\Local\Arduino15\packages\arduino\tools\CMSIS-Atmel\1.2.0\CMSIS\Device\ATMEL\samd51\include\component\port.h

  // get bitmask for register manipulation
  // int mask = digitalPinToBitMask(pin);
  // activate input filters for pin 24
  // REG_PIOA_IFER = mask;
  // choose debounce filter as input filter for pin 24
  // REG_PIOA_DIFSR = mask;
  // set clock divider for slow clock -> rejects pulses shorter than (DIV+1)*31µs and accepts pulses longer than 2*(DIV+1)*31µs
  // REG_PIOA_SCDR = 10;
#endif
}

void PinManager::begin()
{
  // PWM frequency
#ifdef __AVR__
  TCCR3B = (TCCR3B & 0xF8) | 0x02; // set PWM frequency 3.9 Khz (pin2,3,5)
#endif

  uint8_t i;
  for (i = 0; i < PINS_COUNT; i++)
    pinEnabled[i] = 0;
}

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
  if (from == to)
    return value;
  if (from > to)
    return value >> (from - to);
  else
    return value << (to - from);
}

#if defined(_SAM3XA_)
static void TC_SetCMR_ChannelA(Tc *tc, uint32_t chan, uint32_t v)
{
  tc->TC_CHANNEL[chan].TC_CMR = (tc->TC_CHANNEL[chan].TC_CMR & 0xFFF0FFFF) | v;
}

static void TC_SetCMR_ChannelB(Tc *tc, uint32_t chan, uint32_t v)
{
  tc->TC_CHANNEL[chan].TC_CMR = (tc->TC_CHANNEL[chan].TC_CMR & 0xF0FFFFFF) | v;
}
#endif

// sets PWM and frequency for pin for SAM and SAMD architectures - code found at
// C:\Users\alex\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.12\cores\arduino\wiring_analog.c
// and
// C:\Users\alex\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.6.0\cores\arduino\wiring_analog.c
//
// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.
void PinManager::analogWrite(uint32_t ulPin, uint32_t ulValue)
{

#if defined(_SAM3XA_)
  // ---------- SAM3X ----------------------------------------------------------------------------
  uint32_t attr = g_APinDescription[ulPin].ulPinAttribute;
  if ((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG)
  {
    EAnalogChannel channel = g_APinDescription[ulPin].ulADCChannelNumber;
    if (channel == DA0 || channel == DA1)
    {
      uint32_t chDACC = ((channel == DA0) ? 0 : 1);
      if (dacc_get_channel_status(DACC_INTERFACE) == 0)
      {
        /* Enable clock for DACC_INTERFACE */
        pmc_enable_periph_clk(DACC_INTERFACE_ID);
        /* Reset DACC registers */
        dacc_reset(DACC_INTERFACE);
        /* Half word transfer mode */
        dacc_set_transfer_mode(DACC_INTERFACE, 0);
        /* Power save:
           sleep mode  - 0 (disabled)
           fast wakeup - 0 (disabled)
        */
        dacc_set_power_save(DACC_INTERFACE, 0, 0);
        /* Timing:
           refresh        - 0x08 (1024*8 dacc clocks)
           max speed mode -    0 (disabled)
           startup time   - 0x10 (1024 dacc clocks)
        */
        dacc_set_timing(DACC_INTERFACE, 0x08, 0, 0x10);
        /* Set up analog current */
        dacc_set_analog_control(DACC_INTERFACE, DACC_ACR_IBCTLCH0(0x02) |
                                                    DACC_ACR_IBCTLCH1(0x02) |
                                                    DACC_ACR_IBCTLDACCORE(0x01));
      }
      /* Disable TAG and select output channel chDACC */
      dacc_set_channel_selection(DACC_INTERFACE, chDACC);
      if ((dacc_get_channel_status(DACC_INTERFACE) & (1 << chDACC)) == 0)
      {
        dacc_enable_channel(DACC_INTERFACE, chDACC);
      }
      // Write user value
      ulValue = mapResolution(ulValue, _writeResolution, DACC_RESOLUTION);
      dacc_write_conversion_data(DACC_INTERFACE, ulValue);
      while ((dacc_get_interrupt_status(DACC_INTERFACE) & DACC_ISR_EOC) == 0)
        ;
      return;
    }
  }

  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {
    ulValue = mapResolution(ulValue, _writeResolution, PWM_RESOLUTION);
    if (!PWMEnabled)
    {
      // PWM Startup code
      pmc_enable_periph_clk(PWM_INTERFACE_ID);
      PWMC_ConfigureClocks(PWM_FREQUENCY * PWM_MAX_DUTY_CYCLE, 0, VARIANT_MCK);
      PWMEnabled = 1;
    }
    uint32_t chan = g_APinDescription[ulPin].ulPWMChannel;
    if (!pinEnabled[ulPin])
    {
      // Setup PWM for this pin
      PIO_Configure(g_APinDescription[ulPin].pPort,
                    g_APinDescription[ulPin].ulPinType,
                    g_APinDescription[ulPin].ulPin,
                    g_APinDescription[ulPin].ulPinConfiguration);
      PWMC_ConfigureChannel(PWM_INTERFACE, chan, PWM_CMR_CPRE_CLKA, 0, 0);
      PWMC_SetPeriod(PWM_INTERFACE, chan, PWM_MAX_DUTY_CYCLE);
      PWMC_SetDutyCycle(PWM_INTERFACE, chan, ulValue);
      PWMC_EnableChannel(PWM_INTERFACE, chan);
      pinEnabled[ulPin] = 1;
    }
    PWMC_SetDutyCycle(PWM_INTERFACE, chan, ulValue);
    return;
  }

  if ((attr & PIN_ATTR_TIMER) == PIN_ATTR_TIMER)
  {
    // We use MCLK/2 as clock.
    const uint32_t TC = VARIANT_MCK / 2 / TC_FREQUENCY;

    // Map value to Timer ranges 0..255 => 0..TC
    ulValue = mapResolution(ulValue, _writeResolution, TC_RESOLUTION);
    ulValue = ulValue * TC;
    ulValue = ulValue / TC_MAX_DUTY_CYCLE;

    // Setup Timer for this pin
    ETCChannel channel = g_APinDescription[ulPin].ulTCChannel;
    static const uint32_t channelToChNo[] = {0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2};
    static const uint32_t channelToAB[] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    static Tc *channelToTC[] = {
        TC0, TC0, TC0, TC0, TC0, TC0,
        TC1, TC1, TC1, TC1, TC1, TC1,
        TC2, TC2, TC2, TC2, TC2, TC2};
    static const uint32_t channelToId[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8};
    uint32_t chNo = channelToChNo[channel];
    uint32_t chA = channelToAB[channel];
    Tc *chTC = channelToTC[channel];
    uint32_t interfaceID = channelToId[channel];

    if (!TCChanEnabled[interfaceID])
    {
      pmc_enable_periph_clk(TC_INTERFACE_ID + interfaceID);
      TC_Configure(chTC, chNo,
                   TC_CMR_TCCLKS_TIMER_CLOCK1 |
                       TC_CMR_WAVE |         // Waveform mode
                       TC_CMR_WAVSEL_UP_RC | // Counter running up and reset when equals to RC
                       TC_CMR_EEVT_XC0 |     // Set external events from XC0 (this setup TIOB as output)
                       TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR |
                       TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR);
      TC_SetRC(chTC, chNo, TC);
    }
    if (ulValue == 0)
    {
      if (chA)
        TC_SetCMR_ChannelA(chTC, chNo, TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_CLEAR);
      else
        TC_SetCMR_ChannelB(chTC, chNo, TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_CLEAR);
    }
    else
    {
      if (chA)
      {
        TC_SetRA(chTC, chNo, ulValue);
        TC_SetCMR_ChannelA(chTC, chNo, TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET);
      }
      else
      {
        TC_SetRB(chTC, chNo, ulValue);
        TC_SetCMR_ChannelB(chTC, chNo, TC_CMR_BCPB_CLEAR | TC_CMR_BCPC_SET);
      }
    }
    if (!pinEnabled[ulPin])
    {
      PIO_Configure(g_APinDescription[ulPin].pPort,
                    g_APinDescription[ulPin].ulPinType,
                    g_APinDescription[ulPin].ulPin,
                    g_APinDescription[ulPin].ulPinConfiguration);
      pinEnabled[ulPin] = 1;
    }
    if (!TCChanEnabled[interfaceID])
    {
      TC_Start(chTC, chNo);
      TCChanEnabled[interfaceID] = 1;
    }
    return;
  }

  // Defaults to digital write
  pinMode(ulPin, OUTPUT);
  ulValue = mapResolution(ulValue, _writeResolution, 8);
  if (ulValue < 128)
    digitalWrite(ulPin, LOW);
  else
    digitalWrite(ulPin, HIGH);

#elif __SAMD51__ // -------- SAMD ----------------------------------------------------------------------------
  PinDescription pinDesc = g_APinDescription[ulPin];
  uint32_t attr = pinDesc.ulPinAttribute;
  // ATSAMR, for example, doesn't have a DAC
#ifdef DAC
  if ((attr & PIN_ATTR_ANALOG) == PIN_ATTR_ANALOG)
  {
    // CONSOLE.println("DAC pin");
    //  DAC handling code
#if defined(__SAMD51__)
    if (ulPin == PIN_DAC0 || ulPin == PIN_DAC1)
    { // 2 DACs on A0 (PA02) and A1 (PA05)
#else // _SAM3XA_
    if (ulPin == PIN_DAC0)
    { // Only 1 DAC on A0 (PA02)
#endif

#if defined(__SAMD51__)
      ulValue = mapResolution(ulValue, _writeResolution, _dacResolution);
      uint8_t channel = (ulPin == PIN_DAC0 ? 0 : 1);
      pinPeripheral(ulPin, PIO_ANALOG);
      if (!dacEnabled[channel])
      {
        dacEnabled[channel] = true;
        while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST)
          ;
        DAC->CTRLA.bit.ENABLE = 0; // disable DAC
        while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST)
          ;
        DAC->DACCTRL[channel].bit.ENABLE = 1;
        while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST)
          ;
        DAC->CTRLA.bit.ENABLE = 1; // enable DAC
        if (channel == 0)
        {
          while (!DAC->STATUS.bit.READY0)
            ;
          while (DAC->SYNCBUSY.bit.DATA0)
            ;
          DAC->DATA[0].reg = ulValue;
        }
        else if (channel == 1)
        {
          while (!DAC->STATUS.bit.READY1)
            ;
          while (DAC->SYNCBUSY.bit.DATA1)
            ;
          DAC->DATA[1].reg = ulValue;
        }
        delayMicroseconds(10000);
      }
      // ERROR!
      while (!DAC->DACCTRL[channel].bit.ENABLE)
        ;
      if (channel == 0)
      {
        while (!DAC->STATUS.bit.READY0)
          ;
        while (DAC->SYNCBUSY.bit.DATA0)
          ;
        DAC->DATA[0].reg = ulValue; // DAC on 10 bits.
      }
      else if (channel == 1)
      {
        while (!DAC->STATUS.bit.READY1)
          ;
        while (DAC->SYNCBUSY.bit.DATA1)
          ;
        DAC->DATA[1].reg = ulValue; // DAC on 10 bits.
      }
#else
      syncDAC();
      DAC->DATA.reg = ulValue & 0x3FF; // DAC on 10 bits.
      syncDAC();
      DAC->CTRLA.bit.ENABLE = 0x01; // Enable DAC
      syncDAC();
#endif // __SAMD51__
      return;
    }
  }
#endif // DAC

#if defined(__SAMD51__)
  if (attr & (PIN_ATTR_PWM_E | PIN_ATTR_PWM_F | PIN_ATTR_PWM_G))
  {
    // CONSOLE.println("PWM pin");
    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
    static bool tcEnabled[TCC_INST_NUM + TC_INST_NUM];
    if (attr & PIN_ATTR_PWM_E)
      pinPeripheral(ulPin, PIO_TIMER);
    else if (attr & PIN_ATTR_PWM_F)
      pinPeripheral(ulPin, PIO_TIMER_ALT);
    else if (attr & PIN_ATTR_PWM_G)
      pinPeripheral(ulPin, PIO_TCC_PDEC);
    if (!tcEnabled[tcNum])
    {
      tcEnabled[tcNum] = true;
      GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos); // use clock generator 0
      // Set PORT
      if (tcNum >= TCC_INST_NUM)
      {
        // -- Configure TC
        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        // reset
        TCx->COUNT8.CTRLA.bit.SWRST = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.SWRST)
          ;
        // Disable TCx
        TCx->COUNT8.CTRLA.bit.ENABLE = 0;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE)
          ;
        // Set Timer counter Mode to 8 bits, normal PWM, prescaler 1/256
        TCx->COUNT8.CTRLA.reg = TC_CTRLA_MODE_COUNT8 | TC_CTRLA_PRESCALER_DIV256;
        TCx->COUNT8.WAVE.reg = TC_WAVE_WAVEGEN_NPWM;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0)
          ;
        // Set the initial value
        TCx->COUNT8.CC[tcChannel].reg = (uint8_t)ulValue;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0)
          ;
        // Set PER to maximum counter value (resolution : 0xFF)
        TCx->COUNT8.PER.reg = 0xFF;
        while (TCx->COUNT8.SYNCBUSY.bit.PER)
          ;
        // Enable TCx
        TCx->COUNT8.CTRLA.bit.ENABLE = 1;
        while (TCx->COUNT8.SYNCBUSY.bit.ENABLE)
          ;
      }
      else
      {
        // -- Configure TCC
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        TCCx->CTRLA.bit.SWRST = 1;
        while (TCCx->SYNCBUSY.bit.SWRST)
          ;
        // Disable TCCx
        TCCx->CTRLA.bit.ENABLE = 0;
        while (TCCx->SYNCBUSY.bit.ENABLE)
          ;
        // Set prescaler to 1/256
        TCCx->CTRLA.reg = TCC_CTRLA_PRESCALER | TCC_CTRLA_PRESCSYNC_GCLK;
        // Set TCx as normal PWM
        TCCx->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
        while (TCCx->SYNCBUSY.bit.WAVE)
          ;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1)
          ;
        // Set the initial value
        TCCx->CC[tcChannel].reg = (uint32_t)ulValue;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1)
          ;
        // Set PER to maximum counter value (resolution : 0xFF)
        TCCx->PER.reg = TCC_TOP;
        while (TCCx->SYNCBUSY.bit.PER)
          ;
        // Enable TCCx
        TCCx->CTRLA.bit.ENABLE = 1;
        while (TCCx->SYNCBUSY.bit.ENABLE)
          ;
      }
    }
    else
    {
      if (tcNum >= TCC_INST_NUM)
      {
        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        TCx->COUNT8.CC[tcChannel].reg = (uint8_t)ulValue;
        while (TCx->COUNT8.SYNCBUSY.bit.CC0 || TCx->COUNT8.SYNCBUSY.bit.CC1)
          ;
      }
      else
      {
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        while (TCCx->SYNCBUSY.bit.CTRLB)
          ;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1)
          ;
        TCCx->CCBUF[tcChannel].reg = (uint32_t)ulValue;
        while (TCCx->SYNCBUSY.bit.CC0 || TCCx->SYNCBUSY.bit.CC1)
          ;
        TCCx->CTRLBCLR.bit.LUPD = 1;
        while (TCCx->SYNCBUSY.bit.CTRLB)
          ;
      }
    }
    return;
  }
#else
  if ((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM)
  {
    ulValue = mapResolution(ulValue, _writeResolution, 16);
    uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
    uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
    static bool tcEnabled[TCC_INST_NUM + TC_INST_NUM];
    if (attr & PIN_ATTR_TIMER)
    {
#if !(ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10603)
      // Compatibility for cores based on SAMD core <=1.6.2
      if (pinDesc.ulPinType == PIO_TIMER_ALT)
      {
        pinPeripheral(ulPin, PIO_TIMER_ALT);
      }
      else
#endif
      {
        pinPeripheral(ulPin, PIO_TIMER);
      }
    }
    else if ((attr & PIN_ATTR_TIMER_ALT) == PIN_ATTR_TIMER_ALT)
    {
      // this is on an alt timer
      pinPeripheral(ulPin, PIO_TIMER_ALT);
    }
    else
    {
      return;
    }
    if (!tcEnabled[tcNum])
    {
      tcEnabled[tcNum] = true;
      uint16_t GCLK_CLKCTRL_IDs[] = {
          GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC0
          GCLK_CLKCTRL_ID(GCM_TCC0_TCC1), // TCC1
          GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TCC2
          GCLK_CLKCTRL_ID(GCM_TCC2_TC3),  // TC3
          GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC4
          GCLK_CLKCTRL_ID(GCM_TC4_TC5),   // TC5
          GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC6
          GCLK_CLKCTRL_ID(GCM_TC6_TC7),   // TC7
      };
      GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_IDs[tcNum]);
      while (GCLK->STATUS.bit.SYNCBUSY == 1)
        ;
      // Set PORT
      if (tcNum >= TCC_INST_NUM)
      {
        // -- Configure TC
        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        // Disable TCx
        TCx->COUNT16.CTRLA.bit.ENABLE = 0;
        syncTC_16(TCx);
        // Set Timer counter Mode to 16 bits, normal PWM
        TCx->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_NPWM;
        syncTC_16(TCx);
        // Set the initial value
        TCx->COUNT16.CC[tcChannel].reg = (uint32_t)ulValue;
        syncTC_16(TCx);
        // Enable TCx
        TCx->COUNT16.CTRLA.bit.ENABLE = 1;
        syncTC_16(TCx);
      }
      else
      {
        // -- Configure TCC
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        // Disable TCCx
        TCCx->CTRLA.bit.ENABLE = 0;
        syncTCC(TCCx);
        // Set TCCx as normal PWM
        TCCx->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM;
        syncTCC(TCCx);
        // Set the initial value
        TCCx->CC[tcChannel].reg = (uint32_t)ulValue;
        syncTCC(TCCx);
        // Set PER to maximum counter value (resolution : 0xFFFF)
        TCCx->PER.reg = 0xFFFF;
        syncTCC(TCCx);
        // Enable TCCx
        TCCx->CTRLA.bit.ENABLE = 1;
        syncTCC(TCCx);
      }
    }
    else
    {
      if (tcNum >= TCC_INST_NUM)
      {
        Tc *TCx = (Tc *)GetTC(pinDesc.ulPWMChannel);
        TCx->COUNT16.CC[tcChannel].reg = (uint32_t)ulValue;
        syncTC_16(TCx);
      }
      else
      {
        Tcc *TCCx = (Tcc *)GetTC(pinDesc.ulPWMChannel);
        TCCx->CTRLBSET.bit.LUPD = 1;
        syncTCC(TCCx);
        TCCx->CCB[tcChannel].reg = (uint32_t)ulValue;
        syncTCC(TCCx);
        TCCx->CTRLBCLR.bit.LUPD = 1;
        syncTCC(TCCx);
      }
    }
    return;
  }
#endif // __SAMD51__
  // -- Defaults to digital write
  pinMode(ulPin, OUTPUT);
  ulValue = mapResolution(ulValue, _writeResolution, 8);
  if (ulValue < 128)
  {
    digitalWrite(ulPin, LOW);
  }
  else
  {
    digitalWrite(ulPin, HIGH);
  }

#else // ----------------------------------------------------------------------------------------
  ::analogWrite(ulPin, ulValue);

#endif // _SAM3XA_
}
#endif //