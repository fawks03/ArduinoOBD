#include <SoftwareSerial.h>
//for initial bluetooth setup between hc05 and elm327 use https://sites.google.com/site/grcbyte/electronica/arduino/obdii-bluetooth and google translator  (enter AT mode, "AT+ROLE=1", "AT+CMODE=0", "AT+INIT", "AT+INQ", wait, translate address (+INQ:00:12:11:24:93:70  -> 12,11,249370), "AT+INQC", check if it's the right device by getting the name: "AT+RNAME?12,11,249370", "AT+BIND=12,11,249370", "AT+FSAD=12,11,249370", "AT+PAIR=12,11,249370,10" (10 sec timeout), "AT+LINK=12,11,249370")
//try this for usb http://www.instructables.com/id/Hack-an-ELM327-Cable-to-make-an-Arduino-OBD2-Scann/
//
//this sketch contains all PID requests and their associated calculations from the responded hex value to an actually useable int/float that were available for my Opel Corsa D (2014) (34 PIDs)
//to obtain all PIDs available for your specific vehicle send "0100", "0120", ... via Serial Monitor and translate the response as described here: https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_1_PID_00
//(translate the last 8 digits to binary) for example:"0100" -> "41 00 BE 3F B8 13" --> B = 1011, E = 1110 so PID numbers 1,3,4,5,6,7 are supported, while 2 and 8 are not. repeat for all ranges (there are hex and dec pid numbers!)


#define Print(a) Serial.print(a)      //for no debug: #define Print(a)      
#define Println(a) Serial.println(a)  //for no debug: #define Println(a)      

#define RxD 7 //Arduino pin connected to Tx of HC-05          
#define TxD 8 //Arduino pin connected to Rx of HC-05
#define sendDelay 100 //communication delay, maybe not necessary
SoftwareSerial obdSerial(RxD, TxD);

int inSize = -2; //int to store the length of SerialInput to later remove it from the Print (only necessary for Serial Monitor/loop() function)

void setup()
{
  Serial.begin(9600);

  pinMode(RxD, INPUT);  pinMode(TxD, OUTPUT); //maybe not necessary
  Println("initializing OBD connection...");  Println();

  Println(send("ATZ"));  delay(sendDelay);  //reset elm327
  Println(send("ATSP0"));  Println();  delay(sendDelay);  //protocol: auto
  Println(send("ATRV"));  delay(sendDelay); //read voltage
  send("0100");  Println(); Println();  delay(700); //initialize pids

  PrintAll();  //print all available pids
  delay(1000);
}

void loop() {
  if (Serial.available()) { //handle SerialMonitor input
    char inByte = Serial.read();
    obdSerial.print(inByte);
    inSize++; //count input length
  }

  if (obdSerial.available()) {  //handle obdSerial input
    char recvChar;
    boolean prompt;
    prompt = false;
    int count;
    while (!prompt) {                                   //while no prompt
      while ((obdSerial.available() > 0) && (!prompt)) {    //while there is data and not prompt
        recvChar = obdSerial.read();            //read from elm
        count++;
        if (count > inSize) //wait until input 'reprint' is done
          Serial.print(recvChar);
        if (recvChar == 62) {
          prompt = true;  //if received char is '>' then prompt is true
          Serial.println();
          inSize = -2;
        }
      }
    }
  }
}

String send(String input) {
  obdSerial.println(input);

  int i = 0; //to prevent infinite loop
  while (!obdSerial.available() && i < 100) { //wait for response usually ~10-20ms
    delay(1); i++;
  }

  String output;
  while (obdSerial.available()) {
    char recvChar;
    boolean prompt;
    prompt = false;
    while (!prompt) {                                   //while no prompt
      while ((obdSerial.available() > 0) && (!prompt)) {    //while there is data and not prompt
        recvChar = obdSerial.read();            //read from elm

        if (recvChar == 62) prompt = true;  //if received char is '>' then prompt is true
        else output += recvChar;
      }
    }
  }
  return output.substring(input.length());
}


//PIDs & calculations

int getFuelSystemState() {
  return strtol(&send("0103").substring(7, 9)[0], NULL, 16);
}
String getFuelSystemStatus() {
  switch (getFuelSystemState()) {
    case 0: return "Engine is off"; break;
    case 1: return "Open loop due to insufficient engine temperature"; break;
    case 2: return " Closed loop, using oxygen sensor feedback to determine fuel mix"; break;
    case 4: return "Open loop due to engine load OR fuel cut due to deceleration"; break;
    case 8: return "Open loop due to system failure"; break;
    case 16: return "Closed loop, using at least one oxygen sensor but there is a fault in the feedback system"; break;
  }
}

float getCalcLoad() {  //Indicates a percentage of peak available torque. Reaches 100% at wide open throttle at any altitude or RPM for both naturally aspirated and boosted engines. [100%]
  return strtol(&send("0104")[7], NULL, 16) / 255;
}

float getCoolantTemp() {  //Engine coolant temperature as read by the engine coolant temperature sensor. [°C]
  return strtol(&send("0105")[7], NULL, 16) - 40;
}

float getShortFuelTrim() {  //The correction factor being used in closed loop by the PCM to maintain a balanced fuel mixture. If the fuel system is open loop, 0% correction should be reported. [100%]
  return strtol(&send("0106")[7], NULL, 16) / 128 - 1;
}

float getLongFuelTrim() {  //The correction factor (percentage) being used by the fuel control system in both open and closed loop modes of operation. [100%]
  return strtol(&send("0107")[7], NULL, 16) / 128 - 1;
}

float getIntakePressure() {  //Pressure in the intake manifold derived from a Manifold Absolute Pressure (MAP) sensor. [kPa]
  return strtol(&send("010B")[7], NULL, 16) / 128 - 1;
}

int getRPM() {  //The current engine speed in revolutions per minute. [rpm]
  String in = send("010C").substring(7);
  return ((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 4;
}

int getSpeed() {  //The current speed. [km/h]
  return strtol(&send("010D")[7], NULL, 16);
}

float getTimingAdvance() {  //Degrees of ignition timing (spark) advance for #1 cylinder (not including mechanical advance). [°]
  return strtol(&send("010E")[7], NULL, 16) / 2 - 64;
}

float getIntakeTemp() {  //The temperature of the air in the intake manifold as read by the intake manifold air temperature sensor. [°C]
  return strtol(&send("010F")[7], NULL, 16) - 40;
}

float getAirFlow() {  //The airflow rate as measured by the mass air flow sensor (MAF). [grams/sec]
  String in = send("0110").substring(7);
  return ((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 100;
}

float getThrottlePosition() {  //Position of the throttle. [100%]
  return strtol(&send("0111")[7], NULL, 16) / 255;
}

float getOxygen1Voltage() { //The actual voltage being generated by the oxygen sensor 1. [V]
  return strtol(&send("0114").substring(7, 9)[0], NULL, 16) / 200;
}

float getOxygen1STFT() {  //Short Term Fuel Trim for oxygen sensor 1. [100%]
  return strtol(&send("0114").substring(10, 12)[0], NULL, 16) / 128 - 1;
}

float getOxygen2Voltage() { //The actual voltage being generated by the oxygen sensor 2. [V]
  return strtol(&send("0115").substring(7, 9)[0], NULL, 16) / 200;
}

float getOxygen2STFT() {  //Short Term Fuel Trim for oxygen sensor 2. [100%]
  return strtol(&send("0115").substring(10, 12)[0], NULL, 16) / 128 - 1;
}

int getOBDStandards() { //OBD standards this vehicle conforms to. [ID] --> https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_1_PID_1C
  return strtol(&send("011C")[7], NULL, 16);
}

int getRunTime() {  //Run time since engine start. [sec]
  String in = send("011F").substring(7);
  return (strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16);
}

int getDistanceMIL() {  //Distance with Malfunction Indicator Lamp on. [km]
  String in = send("0121").substring(7);
  return (strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16);
}

float getCommandedEvaporativePurge() {  //This value should read 0% when no purge is commanded and 100% at the maximum commanded purge position/flow. [100%]
  return strtol(&send("012E")[7], NULL, 16) / 255;
}

float getFuelLevel() {  //Fuel Tank Level Input. [100%]
  return strtol(&send("012F")[7], NULL, 16) / 255;
}

int getWarmupsSinceClear() {  //Number of warm-up cycles since all DTCs were cleared with a scan tool. [1]
  return strtol(&send("0130")[7], NULL, 16);
}

int getDistanceSinceClear() {  //How many miles the vehicle has been driven since any DTCs were cleared with a scan tool. [km]
  return strtol(&send("0131")[7], NULL, 16);
}

int getBarometricPressure() {  //Absolute Barometric Pressure. [kPa]
  return strtol(&send("0133")[7], NULL, 16);
}

float getCatalystTemp() {  //catalytic converter temperature: Bank 1, Sensor 1. [°C]
  String in = send("013C").substring(7);
  return (((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 10) - 40;
}

float getControlModuleVoltage() {  //Power input to the control module. Normally, this should show battery voltage minus any voltage drop between the battery and the control module. [V]
  String in = send("0142").substring(7);
  return ((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 1000;
}

float getAbsoluteLoadValue() {  //This is the normalized value of air mass per intake stroke displayed as a percent. [100%]
  String in = send("0143").substring(7);
  return ((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 255;
}

float getCommandedEquivalenceRatio() {  // Fuel systems that use conventional oxygen sensor displays the commanded open loop Fuel-Air equivalence ratio while the system is in open loop. Should report 100% when in closed loop fuel. [100%]
  String in = send("0144").substring(7);
  return ((strtol(&in.substring(0, 2)[0], NULL, 16) * 256) + strtol(&in[2], NULL, 16)) / 32768;
}

float getCommandedAFRatio() {  //To obtain the actual air/fuel ratio being commanded, multiply the stoichiometric A/F ratio by the equivalence ratio. [A/F]
  return getCommandedEquivalenceRatio() * 14.64f;
}

float getRelativeThrottlePosition() {  //Relative or learned throttle position. [100%]
  return strtol(&send("0145")[7], NULL, 16) / 255;
}

float getAmbientTemp() {  //The ambient air temperature as read by the air temperature sensor. [°C]
  return strtol(&send("0146")[7], NULL, 16) - 40;
}

float getAbsoluteThrottlePositionB() {  //The absolute throttle position (not the relative or learned) throttle position. Usually above 0% at idle and less than 100% at full throttle. [100%]
  return strtol(&send("0147")[7], NULL, 16) / 255;
}

float getAcceleratorPedalPositionD() {  //The absolute pedal position (not the relative or learned) pedal position. Usually above 0% at idle and less than 100% at full throttle. [100%]
  return strtol(&send("0149")[7], NULL, 16) / 255;
}

float getAcceleratorPedalPositionE() {  //The absolute pedal position (not the relative or learned) pedal position. Usually above 0% at idle and less than 100% at full throttle. [100%]
  return strtol(&send("014A")[7], NULL, 16) / 255;
}

float getCommandedThrottleActuator() {  //This value should be 0% when the throttle is commanded closed and 100% when the throttle is commanded open. [10s0%]
  return strtol(&send("014C")[7], NULL, 16) / 255;
}


float getPID(String in) {
  return strtol(&send(in)[7], NULL, 16);
}


void PrintAll() {
  Serial.println();  Serial.println(">> Available PID information:");
  long firstMillis = millis();
  Serial.print("Fuel System Status: "); Serial.println(getFuelSystemStatus()); delay(100);  //
  Serial.print("Calculated Load: "); Serial.println(getCalcLoad()); delay(100);   //
  Serial.print("Coolant Temperature: "); Serial.print(getCoolantTemp()); Println(" °C"); delay(100);  //
  Serial.print("Short Term Fuel Trim: "); Serial.println(getShortFuelTrim()); delay(100);
  Serial.print("Long Term Fuel Trim: "); Serial.println(getLongFuelTrim()); delay(100);
  Serial.print("Intake Pressure: "); Serial.print(getIntakePressure()); Println(" kPa"); delay(100);
  Serial.print("RPM: "); Serial.print(getRPM()); Println(" 1/min"); delay(100);  //
  Serial.print("Speed: "); Serial.print(getSpeed()); Println(" km/h"); delay(100);  //
  Serial.print("Timing Advance: "); Serial.print(getTimingAdvance()); Println(" °"); delay(100);  //
  Serial.print("Intake Temperature: "); Serial.print(getIntakeTemp()); Println(" °C"); delay(100);
  Serial.print("Air Flow Rate: "); Serial.print(getAirFlow()); Println(" g/s"); delay(100);
  Serial.print("ThrottlePosition: "); Serial.println(getThrottlePosition()); delay(100);  //
  Serial.print("Oxygen Sensor 1 Voltage: "); Serial.print(getOxygen1Voltage()); Println(" V"); delay(100);
  Serial.print("Oxygen Sensor 2 Voltage: "); Serial.print(getOxygen2Voltage()); Println(" V"); delay(100);
  Serial.print("Oxygen Sensor 1 Short Term Fuel Trim: "); Serial.println(getOxygen1STFT()); delay(100);
  Serial.print("Oxygen Sensor 2 Short Term Fuel Trim: "); Serial.println(getOxygen2STFT()); delay(100);
  Serial.print("OBD Standards - ID: "); Serial.println(getOBDStandards()); delay(100);
  Serial.print("Run Time: "); Serial.print(getRunTime()); Println(" sec"); delay(100);  //
  Serial.print("Distance with MIL on: "); Serial.print(getDistanceMIL()); Println(" km"); delay(100);
  Serial.print("Commanded Evaporative Purge: "); Serial.println(getCommandedEvaporativePurge()); delay(100);
  Serial.print("Fuel Tank Level: "); Serial.println(getFuelLevel()); delay(100);  //
  Serial.print("Warm-Up cycles since DTC clear: "); Serial.println(getWarmupsSinceClear()); delay(100);
  Serial.print("Distance since DTC clear: "); Serial.print(getDistanceSinceClear()); Println(" km"); delay(100);
  Serial.print("Catalytic Converter Temperature: "); Serial.print(getCatalystTemp()); Println(" °C"); delay(100);  //
  Serial.print("Control Module Voltage: "); Serial.print(getControlModuleVoltage()); Println(" V"); delay(100);
  Serial.print("Absolute Load Value: "); Serial.println(getAbsoluteLoadValue()); delay(100);
  Serial.print("Commanded Equivalence Ratio: "); Serial.println(getCommandedEquivalenceRatio()); delay(100);
  Serial.print("Commanded Air/Fuel Ratio: "); Serial.println(getCommandedAFRatio()); delay(100);  //
  Serial.print("Relative Throttle Position: "); Serial.println(getRelativeThrottlePosition()); delay(100);  //
  Serial.print("Ambient Temperature: "); Serial.print(getAmbientTemp()); Println(" °C"); delay(100);  //
  Serial.print("Absolute Throttle Position B: "); Serial.println(getAbsoluteThrottlePositionB()); delay(100);
  Serial.print("Accelerator Pedal Position D: "); Serial.println(getAcceleratorPedalPositionD()); delay(100);
  Serial.print("Accelerator Pedal Position E: "); Serial.println(getAcceleratorPedalPositionE()); delay(100);
  Serial.print("Commanded Throttle Actuator: "); Serial.println(getCommandedThrottleActuator()); delay(100);
  Serial.print(">> took "); Serial.println(millis() - firstMillis);
}
