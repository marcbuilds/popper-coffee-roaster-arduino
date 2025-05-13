// this example is public domain. enjoy!
// www.ladyada.net/learn/sensors/thermocouple

#include <max6675.h>
#include <ModbusRtu.h>

// data array for modbus network sharing
uint16_t au16data[16] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1 };

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  u8serno : serial port (use 0 for Serial)
 *  u8txenpin : 0 for RS-232 and USB-FTDI 
 *               or any pin number > 1 for RS-485
 */
Modbus slave(1,0,0); // this is slave @1 and RS-232 or USB-FTDI



//Thermocouple 1
int thermoDO = 2;
int thermoCS = 3;
int thermoCLK = 4;

//Thermocouple 2
int thermoDO2 = 8; // Add a 2 to each int from first Thermocouple. Ex(ThermoDO to ThermoDO2).
int thermoCS2 = 9; // Change pins to the ones you use for your board. Can be any digital pins.
int thermoCLK2 = 7;

MAX6675 thermocouple2(thermoCLK, thermoCS, thermoDO);

MAX6675 thermocouple(thermoCLK2, thermoCS2, thermoDO2); //Thermocouple 2
  
int led = 5;  
  
void setup() {
  slave.begin( 19200 ); // 19200 baud, 8-bits, none, 1-bit stop
  // use Arduino pins 
  pinMode(led, OUTPUT);
 delay(500);
  
}

void loop() {
  // basic readout test, just print the current temp
  
   //Serial.print("C = "); 
   
   au16data[2] = (uint16_t) (thermocouple.readCelsius()*100); 
   
   au16data[3] = (uint16_t) (thermocouple2.readCelsius()*100);
   
   slave.poll( au16data, 16 );

   for(int i=1; i<=99; i++) {
    if(i<=au16data[4])
       digitalWrite(led, HIGH);
      else
        digitalWrite(led, LOW);
      if ( (i % 20) == 0) {
        slave.poll( au16data, 16 );
      }
    delay(40);
   }
   
}