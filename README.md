# ArduinoOBD
Arduino OBD Serial Communication with ELM327 (over Bluetooth)

this sketch contains all PID requests and their associated calculations from the responded hex value to an actually useable int/float that were available for my Opel Corsa D (2014) (34 PIDs)

to obtain all PIDs available for your specific vehicle send "0100", "0120", ... via Serial Monitor and translate the response as described here: https://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_1_PID_00 (translate the last 8 digits to binary)

for example:"0100" -> "41 00 BE 3F B8 13" --> B = 1011, E = 1110 so PID numbers 1,3,4,5,6,7 are supported, while 2 and 8 are not. repeat for all ranges (there are hex and dec pid numbers!)

for initial bluetooth setup between hc05 and elm327 use https://sites.google.com/site/grcbyte/electronica/arduino/obdii-bluetooth and google translator  (enter AT mode, "AT+ROLE=1", "AT+CMODE=0", "AT+INIT", "AT+INQ", wait, translate address (+INQ:00:12:11:24:93:70  -> 12,11,249370), "AT+INQC", check if it's the right device by getting the name: "AT+RNAME?12,11,249370", "AT+BIND=12,11,249370", "AT+FSAD=12,11,249370", "AT+PAIR=12,11,249370,10" (10 sec timeout), "AT+LINK=12,11,249370")

try this for usb http://www.instructables.com/id/Hack-an-ELM327-Cable-to-make-an-Arduino-OBD2-Scann/


