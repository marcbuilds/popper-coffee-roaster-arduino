#include <ModbusRtu.h>
#include <max6675.h>

// Pin configuration
const int SSR_PIN = 8; // SSR control pin
const int thermoCLK = 6;
const int thermoCS1 = 7;
const int thermoCS2 = 5;
const int thermoDO = 4;

// Thermocouples
MAX6675 thermocouple(thermoCLK, thermoCS1, thermoDO);
MAX6675 thermocouple2(thermoCLK, thermoCS2, thermoDO);

// Modbus slave
Modbus slave(1, 0, 2); // slave ID, Serial port (0 = Serial), DE/RE pin = 2
uint16_t au16data[16]; // Modbus register buffer

// PWM control variables
#define PWM_CYCLE_MS 1000  // 1 second total cycle
#define PWM_RESOLUTION 100 // 100 steps = 1% precision
unsigned long lastPwmUpdate = 0;
int pwmStep = 0;

void setup()
{
  // Serial setup
  Serial.begin(9600);

  // Pin modes
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW); // ensure SSR is off initially

  // Modbus start
  slave.begin(9600); // Baud rate must match Artisan config
}

void loop()
{
  // Read Artisan commands
  slave.poll(au16data, 16);

  // Read thermocouples
  au16data[2] = (uint16_t)(thermocouple.readCelsius() * 100);  // Register 2
  au16data[3] = (uint16_t)(thermocouple2.readCelsius() * 100); // Register 3

  // Get PWM target from Artisan (assume slider writes to register 4)
  int pwmTarget = au16data[4]; // Value from 0 to 100

  // Time per PWM step
  unsigned long stepInterval = PWM_CYCLE_MS / PWM_RESOLUTION;

  // PWM update logic (non-blocking)
  if (millis() - lastPwmUpdate >= stepInterval)
  {
    lastPwmUpdate = millis();
    pwmStep = (pwmStep + 1) % PWM_RESOLUTION;

    if (pwmStep < pwmTarget)
    {
      digitalWrite(SSR_PIN, HIGH);
    }
    else
    {
      digitalWrite(SSR_PIN, LOW);
    }
  }
}
