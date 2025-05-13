#include <ModbusRtu.h>
#include <max6675.h>

// Pin configuration
const int SSR_PIN = 8;
const int thermoCLK = 6;
const int thermoCS1 = 7;
const int thermoCS2 = 5;
const int thermoDO = 4;

// Thermocouples
MAX6675 thermocouple(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermocouple2(thermoCLK, thermoCS2, thermoDO);

// Modbus slave setup
Modbus slave(1, 0, 2); // Slave ID 1, Serial, DE/RE pin 2
uint16_t au16data[16]; // Modbus register buffer

// Timing for SSR
unsigned long lastCycleStart = 0;
const unsigned long cycleLengthMs = 1000; // 1 second total cycle

void setup()
{
  Serial.begin(9600);
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);
  slave.begin(9600);
}

void loop()
{
  // Read Modbus commands
  slave.poll(au16data, 16);

  // Read thermocouple temps into registers
  au16data[2] = (uint16_t)(thermocouple.readCelsius() * 100);
  au16data[3] = (uint16_t)(thermocouple2.readCelsius() * 100);

  // Get desired power level from Modbus register 4
  int powerPercent = constrain(au16data[4], 0, 100);

  // Time-proportional control
  unsigned long now = millis();
  unsigned long timeInCycle = now - lastCycleStart;

  // Restart cycle if 1s passed
  if (timeInCycle >= cycleLengthMs)
  {
    lastCycleStart = now;
    timeInCycle = 0;
  }

  // Calculate ON time in ms
  unsigned long onTime = (cycleLengthMs * powerPercent) / 100;

  // Switch SSR
  if (timeInCycle < onTime)
  {
    digitalWrite(SSR_PIN, HIGH);
  }
  else
  {
    digitalWrite(SSR_PIN, LOW);
  }
}
