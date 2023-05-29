# Sunray with Arduino replaced by ESP32-board

This fork is about replacing the Arduino, the bluetooth and wifi (an esp32) and the communication to an ntrip caster by one single esp32 with a sim800 attached.

The esp32, the gprs (sim800), an sd card expanders and a power supply are built on a board with arduino format and can be plugged in on pcb's 1.3 and 1.4 of Ardumower.

The esp32 can switch off the rest of the pcb, including the ublox (which seems to cunsume a lot of energy)

## Download
__WARNING__: Do not use the master version (via download button), that is 'code we work on' and it may be unstable - use one release version instead (click on 'releases' link below)!

https://github.com/Ardumower/Sunray/releases

## Description
An alternative Firmware (experimental) for Ardumower kit mowing and gear motors, PCB 1.3, Adafruit Grand Central M4 (or Arduino Due) and ArduSimple RTK kit

The robot mower uses RTK to localize itself (without a perimeter wire)

## Warning
All software and hardware, software and motor components are designed and optimized as a whole, if you try to replace or exclude some component not as designed, you risk to damage your hardware with the software

## Wiki
http://wiki.ardumower.de/index.php?title=Ardumower_Sunray

## Demo video
https://www.youtube.com/watch?v=yDcmS5zdj90

## License
Ardumower Sunray 
Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH

Licensed GPLv3 for open source use
or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)
    
