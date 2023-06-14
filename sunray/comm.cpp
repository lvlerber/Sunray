#include "comm.h"
#include "config.h"
#include "robot.h"
#include "StateEstimator.h"
#include "LineTracker.h"
#include "Stats.h"
#include "src/op/op.h"
#include "reset.h"

#ifdef __linux__
  #include <BridgeClient.h>
  #include <Process.h>
  #include <WiFi.h>
  #include <sys/resource.h>
#else
  #include "src/esp/WiFiEsp.h"
#endif
#include "RingBuffer.h"

//#define VERBOSE 1

unsigned long nextInfoTime = 0;
bool triggerWatchdog = false;
bool bleConnected = false;
unsigned long bleConnectedTimeout = 0;

int encryptMode = 0; // 0=off, 1=encrypt
int encryptPass = PASS; 
int encryptChallenge = 0;
int encryptKey = 0;

bool simFaultyConn = false; // simulate a faulty connection?
int simFaultConnCounter = 0;

String cmd;
String cmdResponse;

// use a ring buffer to increase speed and reduce memory allocation
ERingBuffer buf(8);
int reqCount = 0;                // number of requests received
unsigned long stopClientTime = 0;
float statControlCycleTime = 0; 
float statMaxControlCycleTime = 0; 

// mqtt
#define MSG_BUFFER_SIZE	(50)
char mqttMsg[MSG_BUFFER_SIZE];
unsigned long nextMQTTPublishTime = 0;
unsigned long nextMQTTLoopTime = 0;

// wifi client
// WiFiEspClient wifiClient;
unsigned long nextWifiClientCheckTime = 0;


// answer Bluetooth with CRC
String cmdAnswer(String s){  
  byte crc = 0;
  for (int i=0; i < s.length(); i++) crc += s[i];
  s += F(",0x");
  if (crc <= 0xF) s += F("0");
  s += String(crc, HEX);  
  s += F("\r\n");        
  CONSOLE.print("Answered with: ")     ;
  CONSOLE.println(s);  
  return s;
}

// request tune param
String cmdTuneParam(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
  int counter = 0;
  int paramIdx = -1;
  int lastCommaIdx = 0;
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){
      float floatValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();
      if (counter == 1){                            
          paramIdx = floatValue;
      } else if (counter == 2){                                      
          CONSOLE.print("tuneParam ");
          CONSOLE.print(paramIdx);
          CONSOLE.print("=");
          CONSOLE.println(floatValue);    
          switch (paramIdx){
            case 0: 
              stanleyTrackingNormalP = floatValue;
              break;
            case 1:
              stanleyTrackingNormalK = floatValue;
              break;
            case 2:
              stanleyTrackingSlowP = floatValue;
              break;
            case 3: 
              stanleyTrackingSlowK = floatValue;
              break;
          } 
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }      
  String s = F("CT");
  return cmdAnswer(s);
}

// request operation
String cmdControl(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
  int counter = 0;
  int lastCommaIdx = 0;
  int mow=-1;          
  int op = -1;
  bool restartRobot = false;
  float wayPerc = -1;  
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){
      int intValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toInt();
      float floatValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();
      if (counter == 1){                            
          if (intValue >= 0) {
            motor.enableMowMotor = (intValue == 1);
            motor.setMowState( (intValue == 1) );
          }
      } else if (counter == 2){                                      
          if (intValue >= 0) op = intValue; 
      } else if (counter == 3){                                      
          if (floatValue >= 0) setSpeed = floatValue; 
      } else if (counter == 4){                                      
          if (intValue >= 0) fixTimeout = intValue; 
      } else if (counter == 5){
          if (intValue >= 0) finishAndRestart = (intValue == 1);
      } else if (counter == 6){
          if (floatValue >= 0) {
            maps.setMowingPointPercent(floatValue);
            restartRobot = true;
          }
      } else if (counter == 7){
          if (intValue > 0) {
            maps.skipNextMowingPoint();
            restartRobot = true;
          }
      } else if (counter == 8){
          if (intValue >= 0) sonar.enabled = (intValue == 1);
      }
      counter++;
      lastCommaIdx = idx;
    }    
  }      
  /*CONSOLE.print("linear=");
  CONSOLE.print(linear);
  CONSOLE.print(" angular=");
  CONSOLE.println(angular);*/    
  OperationType oldStateOp = stateOp;
  if (restartRobot){
    // certain operations may require a start from IDLE state (https://github.com/Ardumower/Sunray/issues/66)
    setOperation(OP_IDLE);    
  }
  if (op >= 0) setOperation((OperationType)op, false); // new operation by operator
    else if (restartRobot){     // no operation given by operator, continue current operation from IDLE state
      setOperation(oldStateOp);    
    }  
  String s = F("C");
  return cmdAnswer(s);
}

// request motor 
String cmdMotor(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
  int counter = 0;
  int lastCommaIdx = 0;
  float linear=0;
  float angular=0;
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){
      float value = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();
      if (counter == 1){                            
          linear = value;
      } else if (counter == 2){
          angular = value;
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }      
  /*CONSOLE.print("linear=");
  CONSOLE.print(linear);
  CONSOLE.print(" angular=");
  CONSOLE.println(angular);*/
  motor.setLinearAngularSpeed(linear, angular, false);
  String s = F("M");
  return cmdAnswer(s);
}

String cmdMotorTest(){
  String s = F("E");
  motor.test();    
  return cmdAnswer(s);
}

String cmdMotorPlot(){
  String s = F("Q");
  motor.plot();    
  return cmdAnswer(s);
}

String cmdSensorTest(){
  String s = F("F");
  sensorTest();   
  return cmdAnswer(s);
}


// request waypoint (perim,excl,dock,mow,free)
// W,startidx,x,y,x,y,x,y,x,y,...
String cmdWaypoint(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6");  
  int counter = 0;
  int lastCommaIdx = 0;
  int widx=0;  
  float x=0;
  float y=0;
  bool success = true;
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){            
      float intValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toInt();
      float floatValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();
      if (counter == 1){                            
          widx = intValue;
      } else if (counter == 2){
          x = floatValue;
      } else if (counter == 3){
          y = floatValue;
          if (!maps.setPoint(widx, x, y)){
            success = false;
            break;
          }          
          widx++;
          counter = 1;
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }      
  /*CONSOLE.print("waypoint (");
  CONSOLE.print(widx);
  CONSOLE.print("/");
  CONSOLE.print(count);
  CONSOLE.print(") ");
  CONSOLE.print(x);
  CONSOLE.print(",");
  CONSOLE.println(y);*/  

  if (!success){   
    stateSensor = SENS_MEM_OVERFLOW;
    setOperation(OP_ERROR);
  } 
    
  String s = F("W,");
  s += widx;              
  return cmdAnswer(s);       
  
}


// request waypoints count
// N,#peri,#excl,#dock,#mow,#free
String cmdWayCount(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6");   
  int counter = 0;
  int lastCommaIdx = 0;
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){            
      float intValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toInt();
      float floatValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();      
      if (counter == 1){                            
          if (!maps.setWayCount(WAY_PERIMETER, intValue)) return "";                
      } else if (counter == 2){
          if (!maps.setWayCount(WAY_EXCLUSION, intValue)) return "";                
      } else if (counter == 3){
          if (!maps.setWayCount(WAY_DOCK, intValue)) return "";                
      } else if (counter == 4){
          if (!maps.setWayCount(WAY_MOW, intValue)) return "";                
      } else if (counter == 5){
          if (!maps.setWayCount(WAY_FREE, intValue)) return "";                
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }        
  String s = F("N");    
  return cmdAnswer(s);         
  //maps.dump();
}


// request exclusion count
// X,startidx,cnt,cnt,cnt,cnt,...
String cmdExclusionCount(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
  int counter = 0;
  int lastCommaIdx = 0;
  int widx=0;  
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){            
      float intValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toInt();
      float floatValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toFloat();
      if (counter == 1){                            
          widx = intValue;
      } else if (counter == 2){
          if (!maps.setExclusionLength(widx, intValue)) return "";          
          widx++;
          counter = 1;
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }        
  String s = F("X,");
  s += widx;              
  return cmdAnswer(s);         
}


// request position mode
String cmdPosMode(String cmd){
  if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
  int counter = 0;
  int lastCommaIdx = 0;  
  for (int idx=0; idx < cmd.length(); idx++){
    char ch = cmd[idx];
    //Serial.print("ch=");
    //Serial.println(ch);
    if ((ch == ',') || (idx == cmd.length()-1)){
      int intValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toInt();
      double doubleValue = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1).toDouble();
      if (counter == 1){                            
          absolutePosSource = bool(intValue);
      } else if (counter == 2){                                      
          absolutePosSourceLon = doubleValue; 
      } else if (counter == 3){                                      
          absolutePosSourceLat = doubleValue; 
      } 
      counter++;
      lastCommaIdx = idx;
    }    
  }        
  CONSOLE.print("absolutePosSource=");
  CONSOLE.print(absolutePosSource);
  CONSOLE.print(" lon=");
  CONSOLE.print(absolutePosSourceLon, 8);
  CONSOLE.print(" lat=");
  CONSOLE.println(absolutePosSourceLat, 8);
  String s = F("P");
  return cmdAnswer(s);
}

// request version
String cmdVersion(){  
#ifdef ENABLE_PASS
  if (encryptMode == 0){
    encryptMode = 1;
    randomSeed(millis());
    while (true){
      encryptChallenge = random(0, 200); // random number between 0..199
      encryptKey = encryptPass % encryptChallenge;   
      if ( (encryptKey >= 1) && (encryptKey <= 94) ) break;  // random key between 1..94
    }
  }
#endif
  String s = F("V,");
  s += F(VER);
  s += F(",");  
  s += encryptMode;
  s += F(",");
  s += encryptChallenge;
  s += F(",");
  s += BOARD;
  s += F(",");
  #ifdef DRV_SERIAL_ROBOT
    s += "SR";
  #elif DRV_ARDUMOWER
    s += "AM";
  #else 
    s += "XX";
  #endif
  String id = "";
  String mcuFwName = "";
  String mcuFwVer = ""; 
  robotDriver.getRobotID(id);
  robotDriver.getMcuFirmwareVersion(mcuFwName, mcuFwVer);
  s += F(",");  
  s += mcuFwName;
  s += F(",");
  s += mcuFwVer;
  s += F(",");  
  s += id;
  CONSOLE.print("sending encryptMode=");
  CONSOLE.print(encryptMode);
  CONSOLE.print(" encryptChallenge=");  
  CONSOLE.println(encryptChallenge);
  return cmdAnswer(s);
}

// request add obstacle
String cmdObstacle(){
  triggerObstacle();    
  String s = F("O");
  return cmdAnswer(s);  
}

// request rain
String cmdRain(){
  activeOp->onRainTriggered();  
  String s = F("O2");
  return cmdAnswer(s);  
}

// request battery low
String cmdBatteryLow(){
  activeOp->onBatteryLowShouldDock();
  String s = F("O3");
  return cmdAnswer(s);    
}

// perform pathfinder stress test
String cmdStressTest(){
  maps.stressTest();  
  String s = F("Z");
  return cmdAnswer(s);  
}

// perform hang test (watchdog should trigger and restart robot)
String cmdTriggerWatchdog(){
  setOperation(OP_IDLE);
  #ifdef __linux__
    Process p;
    p.runShellCommand("reboot");    
  #else
    triggerWatchdog = true;  
  #endif
  String s = F("Y");
  return cmdAnswer(s);  
}

// perform hang test (watchdog should trigger)
String cmdGNSSReboot(){
  CONSOLE.println("GNNS reboot");
  gps.reboot();
  String s = F("Y2");
  return cmdAnswer(s);  
}

// switch-off robot
void cmdSwitchOffRobot(){
  setOperation(OP_IDLE);
  battery.switchOff();
  // String s = F("Y3");
  // return cmdAnswer(s);  
}

// kidnap test (kidnap detection should trigger)
String cmdKidnap(){
  CONSOLE.println("kidnapping robot - kidnap detection should trigger");
  stateX = 0;  
  stateY = 0;
  String s = F("K");
  return cmdAnswer(s);  
}

// toggle GPS solution (invalid,float,fix) for testing
String cmdToggleGPSSolution(){
  CONSOLE.println("toggle GPS solution");
  switch (gps.solution){
    case SOL_INVALID:  
      gps.solutionAvail = true;
      gps.solution = SOL_FLOAT;
      gps.relPosN = stateY - 2.0;  // simulate pos. solution jump
      gps.relPosE = stateX - 2.0;
      lastFixTime = millis();
      stateGroundSpeed = 0.1;
      break;
    case SOL_FLOAT:  
      gps.solutionAvail = true;
      gps.solution = SOL_FIXED;
      stateGroundSpeed = 0.1;
      gps.relPosN = stateY + 2.0;  // simulate undo pos. solution jump
      gps.relPosE = stateX + 2.0;
      break;
    case SOL_FIXED:  
      gps.solutionAvail = true;
      gps.solution = SOL_INVALID;
      break;
  }
  String s = F("G");
  return cmdAnswer(s);    
}


// request obstacles
String cmdObstacles(){
  String s = F("S2,");
  s += maps.obstacles.numPolygons;
  for (int idx=0; idx < maps.obstacles.numPolygons; idx++){
    s += ",0.5,0.5,1,"; // red,green,blue (0-1)    
    s += maps.obstacles.polygons[idx].numPoints;    
    for (int idx2=0 ; idx2 < maps.obstacles.polygons[idx].numPoints; idx2++){
      s += ",";
      s += maps.obstacles.polygons[idx].points[idx2].x();
      s += ",";
      s += maps.obstacles.polygons[idx].points[idx2].y();
    }    
  }

  return cmdAnswer(s);
}

// request summary
String cmdSummary(){
  String s = F("S,");
  s += battery.batteryVoltage;  
  s += ",";
  s += stateX;
  s += ",";
  s += stateY;
  s += ",";
  s += stateDelta;
  s += ",";
  s += gps.solution;
  s += ",";
  s += stateOp;
  s += ",";
  s += maps.mowPointsIdx;
  s += ",";
  s += (millis() - gps.dgpsAge)/1000.0;
  s += ",";
  s += stateSensor;
  s += ",";
  s += maps.targetPoint.x();
  s += ",";
  s += maps.targetPoint.y();  
  s += ",";
  s += gps.accuracy;  
  s += ",";
  s += gps.numSV;  
  s += ",";
  if (stateOp == OP_CHARGE) {
    s += "-";
    s += battery.chargingCurrent;
  } else {
    s += motor.motorsSenseLP;
  }
  s += ",";
  s += gps.numSVdgps;  
  s += ",";
  s += maps.mapCRC;
  s += ",";
  s += lateralError;
  return cmdAnswer(s);  
}

// request statistics
String cmdStats(){
  String s = F("T,");
  s += statIdleDuration;  
  s += ",";
  s += statChargeDuration;
  s += ",";
  s += statMowDuration;
  s += ",";
  s += statMowDurationFloat;
  s += ",";
  s += statMowDurationFix;
  s += ",";
  s += statMowFloatToFixRecoveries;
  s += ",";  
  s += statMowDistanceTraveled;  
  s += ",";  
  s += statMowMaxDgpsAge;
  s += ",";
  s += statImuRecoveries;
  s += ",";
  s += statTempMin;
  s += ",";
  s += statTempMax;
  s += ",";
  s += gps.chksumErrorCounter;
  s += ",";
  //s += ((float)gps.dgpsChecksumErrorCounter) / ((float)(gps.dgpsPacketCounter));
  s += gps.dgpsChecksumErrorCounter;
  s += ",";
  s += statMaxControlCycleTime;
  s += ",";
  s += SERIAL_BUFFER_SIZE;
  s += ",";
  s += statMowDurationInvalid;
  s += ",";
  s += statMowInvalidRecoveries;
  s += ",";
  s += statMowObstacles;
  s += ",";
  s += freeMemory();
  s += ",";
  s += getResetCause();
  s += ",";
  s += statGPSJumps;
  s += ",";
  s += statMowSonarCounter;
  s += ",";
  s += statMowBumperCounter;
  s += ",";
  s += statMowGPSMotionTimeoutCounter;
  s += ",";
  s += statMowDurationMotorRecovery;
  return cmdAnswer(s);  
}

// clear statistics
String cmdClearStats(){
  String s = F("L");
  statMowDurationMotorRecovery = 0;
  statIdleDuration = 0;
  statChargeDuration = 0;
  statMowDuration = 0;
  statMowDurationInvalid = 0;
  statMowDurationFloat = 0;
  statMowDurationFix = 0;
  statMowFloatToFixRecoveries = 0;
  statMowInvalidRecoveries = 0;
  statMowDistanceTraveled = 0;
  statMowMaxDgpsAge = 0;
  statImuRecoveries = 0;
  statTempMin = 9999;
  statTempMax = -9999;
  gps.chksumErrorCounter = 0;
  gps.dgpsChecksumErrorCounter = 0;
  statMaxControlCycleTime = 0;
  statMowObstacles = 0;
  statMowBumperCounter = 0; 
  statMowSonarCounter = 0;
  statMowLiftCounter = 0;
  statMowGPSMotionTimeoutCounter = 0;
  statGPSJumps = 0;
  return cmdAnswer(s);  
}

// scan WiFi networks
String cmdWiFiScan(){
  CONSOLE.println("cmdWiFiScan");
  String s = F("B1,");  
  #ifdef __linux__    
  int numNetworks = WiFi.scanNetworks();
  CONSOLE.print("numNetworks=");
  CONSOLE.println(numNetworks);
  for (int i=0; i < numNetworks; i++){
      CONSOLE.println(WiFi.SSID(i));
      s += WiFi.SSID(i);
      if (i < numNetworks-1) s += ",";
  }
  #endif  
  return cmdAnswer(s);
}

// setup WiFi
String cmdWiFiSetup(){
  CONSOLE.println("cmdWiFiSetup");
  #ifdef __linux__
    if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
    int counter = 0;
    int lastCommaIdx = 0;    
    String ssid = "";
    String pass = "";
    for (int idx=0; idx < cmd.length(); idx++){
      char ch = cmd[idx];
      //Serial.print("ch=");
      //Serial.println(ch);
      if ((ch == ',') || (idx == cmd.length()-1)){
        String str = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1);
        if (counter == 1){                            
            ssid = str;
        } else if (counter == 2){
            pass = str;
        } 
        counter++;
        lastCommaIdx = idx;
      }    
    }      
    /*CONSOLE.print("ssid=");
    CONSOLE.print(ssid);
    CONSOLE.print(" pass=");
    CONSOLE.println(pass);*/
    WiFi.begin((char*)ssid.c_str(), (char*)pass.c_str());    
  #endif
  String s = F("B2");
  return cmdAnswer(s);
}

// request WiFi status
String cmdWiFiStatus(){
  String s = F("B3,");  
  #ifdef __linux__
  IPAddress addr = WiFi.localIP();
	s += addr[0];
	s += ".";
	s += addr[1];
	s += ".";
	s += addr[2];
	s += ".";
	s += addr[3];		
  #endif  
  return cmdAnswer(s);
}


// request firmware update
String cmdFirmwareUpdate(){
  String s = F("U1");  
  #ifdef __linux__
    if (cmd.length()<6) return cmdAnswer("cmdlength <6"); 
    int counter = 0;
    int lastCommaIdx = 0;    
    String fileURL = "";
    for (int idx=0; idx < cmd.length(); idx++){
      char ch = cmd[idx];
      if ((ch == ',') || (idx == cmd.length()-1)){
        String str = cmd.substring(lastCommaIdx+1, ch==',' ? idx : idx+1);
        if (counter == 1){                            
            fileURL = str;
        } 
        counter++;
        lastCommaIdx = idx;
      }    
    }          
    CONSOLE.print("applying firmware update: ");
    CONSOLE.println(fileURL);
    Process p;
    p.runShellCommand("/home/pi/sunray_install/update.sh --apply --url " + fileURL + " &");
  #endif  
  return cmdAnswer(s);
}

// process request
// changed definition because of possibility that bleutooth, web and console are treated simultaneously
// cmd and cmdResponse are now parameter and returned value

String processCmd(String cmd, bool checkCrc, bool decrypt){
  String cmdResponse = "niet gewijzigd";      
  if (!cmd.startsWith("AT+"))
    {
      void WebConsoleInputHandler(uint8_t *data, size_t len);
        WebConsoleInputHandler((uint8_t *)cmd.c_str(), strlen(cmd.c_str()));
        return  "submitted";
    }
  if (cmd.length() < 4) return cmdResponse;
#ifdef ENABLE_PASS
  if (decrypt){
    String s = cmd.substring(0,4);
    if ( s != "AT+V"){
      if (encryptMode == 1){
        // decrypt        
        for (int i=0; i < cmd.length(); i++) {
          if ( (byte(cmd[i]) >= 32) && (byte(cmd[i]) <= 126) ){  // ASCII between 32..126
            int code = byte(cmd[i]);
            code -= encryptKey;
            if (code <= 31) code = 126 + (code-31);
            cmd[i] = char(code);  
          }
        }
        #ifdef VERBOSE
          CONSOLE.print("decrypt:");
          CONSOLE.println(cmd);
        #endif
      }
    } 
  }
#endif
  byte expectedCrc = 0;
  int idx = cmd.lastIndexOf(',');
  if (idx < 1){
    if (checkCrc){
      CONSOLE.print("COMM CRC ERROR: ");
      CONSOLE.println(cmd);
      return cmdResponse;
    }
  } else {
    CONSOLE.println("p1---------------");
    for (int i=0; i < idx; i++) expectedCrc += cmd[i];  
    String s = cmd.substring(idx+1, idx+5);
    int crc = strtol(s.c_str(), NULL, 16);
    bool crcErr = false;
    simFaultConnCounter++;
    if ((simFaultyConn) && (simFaultConnCounter % 10 == 0)) crcErr = true;
    if ((expectedCrc != crc) && (checkCrc)) crcErr = true;      
    CONSOLE.print(cmd);
        CONSOLE.println("----p2---------------");
    if (crcErr) {
      CONSOLE.print("CRC ERROR");
      CONSOLE.print(crc,HEX);
      CONSOLE.print(",");
      CONSOLE.print(expectedCrc,HEX);
      CONSOLE.println();
      return "crc error";        
    } else {
      // remove CRC
      cmd = cmd.substring(0, idx);
      CONSOLE.println(cmd);
    }    
  }     
  if (cmd[0] != 'A') return "not A";
  if (cmd[1] != 'T') return "not T";
  if (cmd[2] != '+') return "not +";
  if (cmd[3] == 'S') {
    if (cmd.length() <= 4){
      return cmdSummary(); 
    } else {
      if (cmd[4] == '2') return cmdObstacles();      
    }
  }
  if (cmd[3] == 'M') return cmdMotor(cmd);
  if (cmd[3] == 'C'){ 
    if ((cmd.length() > 4) && (cmd[4] == 'T')) return cmdTuneParam(cmd);
    else return cmdControl(cmd);
  }
  if (cmd[3] == 'W') return cmdWaypoint(cmd);
  if (cmd[3] == 'N') return cmdWayCount(cmd);
  if (cmd[3] == 'X') return cmdExclusionCount(cmd);
  if (cmd[3] == 'V') return cmdVersion();  
  if (cmd[3] == 'P') return cmdPosMode(cmd);  
  if (cmd[3] == 'T') return cmdStats();
  if (cmd[3] == 'L') return cmdClearStats();
  if (cmd[3] == 'E') return cmdMotorTest();  
  if (cmd[3] == 'Q') return cmdMotorPlot();  
  if (cmd[3] == 'O'){
    if (cmd.length() <= 4){
      return cmdObstacle();   // for developers
    } else {
      if (cmd[4] == '2') return cmdRain();   // for developers
      if (cmd[4] == '3') return cmdBatteryLow();   // for developers
    }    
  }   
  if (cmd[3] == 'F') return cmdSensorTest(); 
  if (cmd[3] == 'B') {
    if (cmd[4] == '1') return cmdWiFiScan();
    if (cmd[4] == '2') return cmdWiFiSetup();   
    if (cmd[4] == '3') return cmdWiFiStatus();     
  }
  if (cmd[3] == 'U'){ 
    if ((cmd.length() > 4) && (cmd[4] == '1')) return cmdFirmwareUpdate();
  }
  if (cmd[3] == 'G') return cmdToggleGPSSolution();   // for developers
  if (cmd[3] == 'K') return cmdKidnap();   // for developers
  if (cmd[3] == 'Z') return cmdStressTest();   // for developers
  if (cmd[3] == 'Y') {
    if (cmd.length() <= 4){
      return cmdTriggerWatchdog();   // for developers
    } else {
      if (cmd[4] == '2') return cmdGNSSReboot();   // for developers
      if (cmd[4] == '3') { cmdSwitchOffRobot(); return  "Y3";} // for developers
    }
  }
  return "not a command";
}

// process console input
void processConsole(){
  char ch;      
  if (CONSOLE.available()){
    battery.resetIdle();  
    while ( CONSOLE.available() ){               
      ch = CONSOLE.read();          
      if ((ch == '\r') || (ch == '\n')) {        
        CONSOLE.print("CON:");
        CONSOLE.println(cmd);
        cmdResponse=processCmd(cmd,  false, false);              
        CONSOLE.print(cmdResponse);    
        cmd = "";
      } else if (cmd.length() < 500){
        cmd += ch;
      }
    }
  }     
}
  
// process Bluetooth input
#ifndef ESP32
// in case of esp32 bluetooth runs in separate task. See bluetooth.cpp
void processBLE(){
  char ch;   
  if (BLE.available()){
    battery.resetIdle();  
    bleConnected = true;
    bleConnectedTimeout = millis() + 5000;
    while ( BLE.available() ){    
      ch = BLE.read();      
      if ((ch == '\r') || (ch == '\n')) {   
        #ifdef VERBOSE
          CONSOLE.print("BLE:");     
          CONSOLE.println(cmd);        
        #endif
        cmdResponse= processCmd(cmd,  true, true);              
        BLE.print(cmdResponse);    
        cmd = "";
      } else if (cmd.length() < 500){
        cmd += ch;
      }
    }    
  } else {
    if (millis() > bleConnectedTimeout){
      bleConnected = false;
    }
  }  
}  
#endif

// process WIFI input (relay client)
// a relay server allows to access the robot via the Internet by transferring data from app to robot and vice versa
// client (app) --->  relay server  <--- client (robot)
void processWifiRelayClient(){

/*

  if (!wifiFound) return;
  if (!ENABLE_RELAY) return;
  if (!wifiClient.connected() || (wifiClient.available() == 0)){
    if (millis() > nextWifiClientCheckTime){   
      wifiClient.stop();
      CONSOLE.println("WIF: connecting..." RELAY_HOST);    
      if (!wifiClient.connect(RELAY_HOST, RELAY_PORT)) {
        CONSOLE.println("WIF: connection failed");
        nextWifiClientCheckTime = millis() + 10000;
        return;
      }
      CONSOLE.println("WIF: connected!");   
      String s = "GET / HTTP/1.1\r\n";
      s += "Host: " RELAY_USER "." RELAY_MACHINE "." RELAY_HOST ":";        
      s += String(RELAY_PORT) + "\r\n";
      s += "Content-Length: 0\r\n";
      s += "\r\n\r\n";
      wifiClient.print(s);
    } else return;
  }
  nextWifiClientCheckTime = millis() + 10000;     
  
  buf.init();                               // initialize the circular buffer   
  unsigned long timeout = millis() + 500;
    
  while (millis() < timeout) {              // loop while the client's connected    
    if (wifiClient.available()) {               // if there's bytes to read from the client,        
      char c = wifiClient.read();               // read a byte, then
      timeout = millis() + 200;
      buf.push(c);                          // push it to the ring buffer
      // you got two newline characters in a row
      // that's the end of the HTTP request, so send a response
      if (buf.endsWith("\r\n\r\n")) {
        cmd = "";
        while ((wifiClient.connected()) && (wifiClient.available()) && (millis() < timeout)) {
          char ch = wifiClient.read();
          timeout = millis() + 200;
          cmd = cmd + ch;
          gps.run();
        }
        CONSOLE.print("WIF:");
        CONSOLE.println(cmd);
        if (wifiClient.connected()) {
          processCmd(true,true);
          String s = "HTTP/1.1 200 OK\r\n";
            s += "Host: " RELAY_USER "." RELAY_MACHINE "." RELAY_HOST ":";        
            s += String(RELAY_PORT) + "\r\n";
            s += "Access-Control-Allow-Origin: *\r\n";              
            s += "Content-Type: text/html\r\n";              
            s += "Connection: close\r\n";  // the connection will be closed after completion of the response
            // "Refresh: 1\r\n"        // refresh the page automatically every 20 sec                                    
            s += "Content-length: ";
            s += String(cmdResponse.length());
            s += "\r\n\r\n";  
            s += cmdResponse;                      
            wifiClient.print(s);                                   
        }
        break;
      }
    }
  }
  */
}



// process WIFI input (App server)
// client (app) --->  server (robot)
void processWifiAppServer()
{
#ifndef ESP32  
  if (!wifiFound) return;
  if (!ENABLE_SERVER) return;
  // listen for incoming clients    
  if (client){
    if (stopClientTime != 0) {
      if (millis() > stopClientTime){
        #ifdef VERBOSE 
          CONSOLE.println("app stopping client");
        #endif
        client.stop();
        stopClientTime = 0;                   
      }
      return;    
    }     
  }
  if (!client){
    //CONSOLE.println("client is NULL");
    client = server.available();      
  }
  if (client) {                               // if you get a client,
    #ifdef VERBOSE
      CONSOLE.println("New client");             // print a message out the serial port
    #endif
    battery.resetIdle();
    buf.init();                               // initialize the circular buffer
    unsigned long timeout = millis() + 50;
    while ( (client.connected()) && (millis() < timeout) ) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,        
        char c = client.read();               // read a byte, then
        timeout = millis() + 50;
        buf.push(c);                          // push it to the ring buffer
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")) {
          cmd = "";
          while ((client.connected()) && (client.available()) && (millis() < timeout)) {
            char ch = client.read();
            timeout = millis() + 50;
            cmd = cmd + ch;
            gps.run();
          }
          #ifdef VERBOSE
            CONSOLE.print("WIF:");
            CONSOLE.println(cmd);
          #endif
          if (client.connected()) {
            cmdResponse=processCmd(cmd, true,true);
            client.print(
              "HTTP/1.1 200 OK\r\n"
              "Access-Control-Allow-Origin: *\r\n"              
              "Content-Type: text/html\r\n"              
              "Connection: close\r\n"  // the connection will be closed after completion of the response
              // "Refresh: 1\r\n"        // refresh the page automatically every 20 sec                        
              );
            client.print("Content-length: ");
            client.print(cmdResponse.length());
            client.print("\r\n\r\n");                        
            client.print(cmdResponse);                                   
          }
          break;
        }
      } 
      gps.run();
    }    
    // give the web browser time to receive the data
    stopClientTime = millis() + 100;
    //delay(10);
    // close the connection
    //client.stop();
    //CONSOLE.println("Client disconnected");
  }    
#endif              
}


void mqttReconnect() {
  // Loop until we're reconnected
  if (!mqttClient.connected()) {
    CONSOLE.println("MQTT: Attempting connection...");
    // Create a random client ID
    String clientId = "sunray-ardumower";
    // Attempt to connect
#ifdef MQTT_USER
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
#else
    if (mqttClient.connect(clientId.c_str())) {
#endif
      CONSOLE.println("MQTT: connected");
      // Once connected, publish an announcement...
      //mqttClient.publish("outTopic", "hello world");
      // ... and resubscribe
      CONSOLE.println("MQTT: subscribing " MQTT_TOPIC_PREFIX "/cmd");
      mqttClient.subscribe(MQTT_TOPIC_PREFIX "/cmd");
    } else {
      CONSOLE.print("MQTT: failed, rc=");
      CONSOLE.print(mqttClient.state());
    }
  }
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  CONSOLE.print("MQTT: Message arrived [");
  CONSOLE.print(topic);
  CONSOLE.print("] ");
  String cmd = ""; 
  for (int i = 0; i < length; i++) {
    cmd += (char)payload[i];    
  }
  CONSOLE.println(cmd);
  if (cmd == "dock") {
    setOperation(OP_DOCK, false);
  } else if (cmd ==  "stop") {
    setOperation(OP_IDLE, false);
  } else if (cmd == "start"){
    setOperation(OP_MOW, false);
  }
}

// define a macro so avoid repetitive code lines for sending single values via MQTT
#define MQTT_PUBLISH(VALUE, FORMAT, TOPIC) \
      snprintf (mqttMsg, MSG_BUFFER_SIZE, FORMAT, VALUE); \
      mqttClient.publish(MQTT_TOPIC_PREFIX TOPIC, mqttMsg);    


// process MQTT input/output (subcriber/publisher)
void processWifiMqttClient()
{
  if (!ENABLE_MQTT) return; 
  if (millis() >= nextMQTTPublishTime){
    nextMQTTPublishTime = millis() + 20000;
    if (mqttClient.connected()) {
      updateStateOpText();
      // operational state
      //CONSOLE.println("MQTT: publishing " MQTT_TOPIC_PREFIX "/status");      
      MQTT_PUBLISH(stateOpText.c_str(), "%s", "/op")
      MQTT_PUBLISH(maps.percentCompleted, "%d", "/progress")

      // GPS related information
      snprintf (mqttMsg, MSG_BUFFER_SIZE, "%.2f, %.2f", gps.relPosN, gps.relPosE);          
      mqttClient.publish(MQTT_TOPIC_PREFIX "/gps/pos", mqttMsg);
      MQTT_PUBLISH(gpsSolText.c_str(), "%s", "/gps/sol")
      MQTT_PUBLISH(gps.iTOW, "%lu", "/gps/tow")
      
      MQTT_PUBLISH(gps.lon, "%.8f", "/gps/lon")
      MQTT_PUBLISH(gps.lat, "%.8f", "/gps/lat")
      MQTT_PUBLISH(gps.height, "%.1f", "/gps/height")
      MQTT_PUBLISH(gps.relPosN, "%.4f", "/gps/relNorth")
      MQTT_PUBLISH(gps.relPosE, "%.4f", "/gps/relEast")
      MQTT_PUBLISH(gps.relPosD, "%.2f", "/gps/relDist")
      MQTT_PUBLISH((millis()-gps.dgpsAge)/1000.0, "%.2f","/gps/ageDGPS")
      MQTT_PUBLISH(gps.accuracy, "%.2f", "/gps/accuracy")
      MQTT_PUBLISH(gps.groundSpeed, "%.4f", "/gps/groundSpeed")
      
      // power related information      
      MQTT_PUBLISH(battery.batteryVoltage, "%.2f", "/power/battery/voltage")
      MQTT_PUBLISH(motor.motorsSenseLP, "%.2f", "/power/motor/current")
      MQTT_PUBLISH(battery.chargingVoltage, "%.2f", "/power/battery/charging/voltage")
      MQTT_PUBLISH(battery.chargingCurrent, "%.2f", "/power/battery/charging/current")

      // map related information
      MQTT_PUBLISH(maps.targetPoint.x(), "%.2f", "/map/targetPoint/X")
      MQTT_PUBLISH(maps.targetPoint.y(), "%.2f", "/map/targetPoint/Y")
      MQTT_PUBLISH(stateX, "%.2f", "/map/pos/X")
      MQTT_PUBLISH(stateY, "%.2f", "/map/pos/Y")
      MQTT_PUBLISH(stateDelta, "%.2f", "/map/pos/Dir")

      // statistics
      MQTT_PUBLISH(statIdleDuration, "%d", "/stats/idleDuration")
      MQTT_PUBLISH(statChargeDuration, "%d", "/stats/chargeDuration")
      MQTT_PUBLISH(statMowDuration, "%d", "/stats/mow/totalDuration")
      MQTT_PUBLISH(statMowDurationInvalid, "%d", "/stats/mow/invalidDuration")
      MQTT_PUBLISH(statMowDurationFloat, "%d", "/stats/mow/floatDuration")
      MQTT_PUBLISH(statMowDurationFix, "%d", "/stats/mow/fixDuration")
      MQTT_PUBLISH(statMowFloatToFixRecoveries, "%d", "/stats/mow/floatToFixRecoveries")
      MQTT_PUBLISH(statMowObstacles, "%d", "/stats/mow/obstacles")
      MQTT_PUBLISH(statMowGPSMotionTimeoutCounter, "%d", "/stats/mow/gpsMotionTimeouts")
      MQTT_PUBLISH(statMowBumperCounter, "%d", "/stats/mow/bumperEvents")
      MQTT_PUBLISH(statMowSonarCounter, "%d", "/stats/mow/sonarEvents")
      MQTT_PUBLISH(statMowLiftCounter, "%d", "/stats/mow/liftEvents")
      MQTT_PUBLISH(statMowMaxDgpsAge, "%.2f", "/stats/mow/maxDgpsAge")
      MQTT_PUBLISH(statMowDistanceTraveled, "%.1f", "/stats/mow/distanceTraveled")
      MQTT_PUBLISH(statMowInvalidRecoveries, "%d", "/stats/mow/invalidRecoveries")
      MQTT_PUBLISH(statImuRecoveries, "%d", "/stats/imuRecoveries")
      MQTT_PUBLISH(statGPSJumps, "%d", "/stats/gpsJumps")      
      MQTT_PUBLISH(statTempMin, "%.1f", "/stats/tempMin")
      MQTT_PUBLISH(statTempMax, "%.1f", "/stats/tempMax")
      MQTT_PUBLISH(stateTemp, "%.1f", "/stats/curTemp")

    } else {
      mqttReconnect();  
    }
  }
  if (millis() > nextMQTTLoopTime){
    nextMQTTLoopTime = millis() + 20000;
    mqttClient.loop();
  }
}


void processComm(){
  processConsole();     
  #ifndef ESP32
  processBLE();     
  if (!bleConnected){
    processWifiAppServer();
    processWifiRelayClient();
    processWifiMqttClient();
  }
  #endif
  if (triggerWatchdog) {
    CONSOLE.println("hang test - watchdog should trigger and perform a reset");
    while (true){
      // do nothing, just hang    
    }
  }
}


// output summary on console
void outputConsole(){
  //return;
  if (millis() > nextInfoTime){        
    bool started = (nextInfoTime == 0);
    nextInfoTime = millis() + 5000;                   
    unsigned long totalsecs = millis()/1000;
    unsigned long totalmins = totalsecs/60;
    unsigned long hour = totalmins/60;
    unsigned long min = totalmins % 60;
    unsigned long sec = totalsecs % 60;
    CONSOLE.print (hour);        
    CONSOLE.print (":");    
    CONSOLE.print (min);        
    CONSOLE.print (":");    
    CONSOLE.print (sec);     
    CONSOLE.print (" ctlDur=");        
    //if (!imuIsCalibrating){
    if (!started){
      if (controlLoops > 0){
        statControlCycleTime = 1.0 / (((float)controlLoops)/5.0);
      } else statControlCycleTime = 5;
      statMaxControlCycleTime = max(statMaxControlCycleTime, statControlCycleTime);    
    }
    controlLoops=0;    
    CONSOLE.print (statControlCycleTime);        
    CONSOLE.print (" op=");    
    CONSOLE.print (activeOp->getOpChain());    
    //CONSOLE.print (stateOp);
    #ifdef __linux__
      CONSOLE.print (" mem=");
      struct rusage r_usage;
      getrusage(RUSAGE_SELF,&r_usage);
      CONSOLE.print(r_usage.ru_maxrss);
    #elif ESP32
    #else
      CONSOLE.print (" freem=");
      CONSOLE.print (freeMemory());  
      uint32_t *spReg = (uint32_t*)__get_MSP();   // stack pointer
      CONSOLE.print (" sp=");
      CONSOLE.print (*spReg, HEX);
    #endif
    CONSOLE.print(" bat=");
    CONSOLE.print(battery.batteryVoltage);
    CONSOLE.print(",");
    CONSOLE.print(battery.batteryVoltageSlope, 3);    
    CONSOLE.print("(");    
    CONSOLE.print(motor.motorsSenseLP);    
    CONSOLE.print(") chg=");
    CONSOLE.print(battery.chargingVoltage);    
    CONSOLE.print("(");
    CONSOLE.print(battery.chargingCurrent);    
    CONSOLE.print(") diff=");
    CONSOLE.print(battery.chargingVoltBatteryVoltDiff, 3);
    CONSOLE.print(" tg=");
    CONSOLE.print(maps.targetPoint.x());
    CONSOLE.print(",");
    CONSOLE.print(maps.targetPoint.y());
    CONSOLE.print(" x=");
    CONSOLE.print(stateX);
    CONSOLE.print(" y=");
    CONSOLE.print(stateY);
    CONSOLE.print(" delta=");
    CONSOLE.print(stateDelta);    
    CONSOLE.print(" tow=");
    CONSOLE.print(gps.iTOW);
    CONSOLE.print(" lon=");
    CONSOLE.print(gps.lon,8);
    CONSOLE.print(" lat=");
    CONSOLE.print(gps.lat,8);    
    CONSOLE.print(" h=");
    CONSOLE.print(gps.height,1);    
    CONSOLE.print(" n=");
    CONSOLE.print(gps.relPosN);
    CONSOLE.print(" e=");
    CONSOLE.print(gps.relPosE);
    CONSOLE.print(" d=");
    CONSOLE.print(gps.relPosD);
    CONSOLE.print(" sol=");    
    CONSOLE.print(gps.solution);
    CONSOLE.print(" age=");    
    CONSOLE.print((millis()-gps.dgpsAge)/1000.0);
    CONSOLE.println();
    //logCPUHealth();    
  }
}
