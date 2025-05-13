// Libraries
#include <max6675.h>
#include <ModbusRtu.h>

// --- Pin Definitions ---
const int SSR_PIN = 5; // Was 'led' in your original sketch

// Thermocouple 1 Pins (ensure these match your physical wiring)
const int TC1_DO_PIN = 2;
const int TC1_CS_PIN = 3;
const int TC1_CLK_PIN = 4;

// Thermocouple 2 Pins (ensure these match your physical wiring)
const int TC2_DO_PIN = 8;
const int TC2_CS_PIN = 9;
const int TC2_CLK_PIN = 7;

// --- MAX6675 Thermocouple Objects ---
MAX6675 thermocouple1(TC1_CLK_PIN, TC1_CS_PIN, TC1_DO_PIN);
MAX6675 thermocouple2(TC2_CLK_PIN, TC2_CS_PIN, TC2_DO_PIN);

// --- Modbus Configuration ---
uint16_t au16data[16];
Modbus slave(1, 0, 0); // Slave ID 1, Serial0, No TX_ENABLE pin

// --- Time-Proportional Control Variables ---
unsigned long lastCycleStartTime = 0;
const unsigned long CYCLE_LENGTH_MS = 1000;

// --- Thermocouple Reading Timer ---
unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 1000; // Read temps every 1 second

void setup()
{
  Serial.begin(19200);
  slave.begin(19200);

  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);

  // Initialize Modbus registers
  au16data[4] = 0; // Default power to 0%
  // Initialize temp registers to a known "error" or "not yet read" state if desired
  // au16data[2] = 0; // Or some indicator like 99999 for error
  // au16data[3] = 0;

  // Ensure MAX6675 CS pins are initially high (deselected)
  // The library constructor should do this, but being explicit doesn't hurt.
  digitalWrite(TC1_CS_PIN, HIGH);
  digitalWrite(TC2_CS_PIN, HIGH);

  delay(500); // Allow sensors to stabilize
  Serial.println("Arduino Modbus Heater Control Ready.");
  Serial.print("SSR Pin: ");
  Serial.println(SSR_PIN);
  Serial.print("Cycle Length: ");
  Serial.print(CYCLE_LENGTH_MS);
  Serial.println(" ms");
  Serial.print("Temp Read Interval: ");
  Serial.print(TEMP_READ_INTERVAL_MS);
  Serial.println(" ms");
}

void loop()
{
  unsigned long currentTime = millis(); // Get current time once per loop

  // --- Modbus Communication ---
  slave.poll(au16data, 16);

  // --- Read Thermocouples on a Timer ---
  if (currentTime - lastTempReadTime >= TEMP_READ_INTERVAL_MS)
  {
    lastTempReadTime = currentTime;
    float tempC1 = NAN, tempC2 = NAN; // Initialize to NAN

    // Read Thermocouple 1
    digitalWrite(TC2_CS_PIN, HIGH); // Ensure other thermocouple is deselected
    delayMicroseconds(50);          // Short delay for CS lines to settle
    tempC1 = thermocouple1.readCelsius();
    digitalWrite(TC1_CS_PIN, HIGH); // Ensure this one is deselected (library should do this, but for clarity)

    if (!isnan(tempC1))
    {
      au16data[2] = (uint16_t)(tempC1 * 100);
    }
    else
    {
      // Optional: Keep last good value or set error code in au16data[2]
      // Serial.println("Error reading Thermocouple 1!");
    }

    delay(20); // Brief pause between reading different SPI devices

    // Read Thermocouple 2
    digitalWrite(TC1_CS_PIN, HIGH); // Ensure other thermocouple is deselected
    delayMicroseconds(50);          // Short delay
    tempC2 = thermocouple2.readCelsius();
    digitalWrite(TC2_CS_PIN, HIGH); // Ensure this one is deselected

    if (!isnan(tempC2))
    {
      au16data[3] = (uint16_t)(tempC2 * 100);
    }
    else
    {
      // Optional: Keep last good value or set error code in au16data[3]
      // Serial.println("Error reading Thermocouple 2!");
    }
  }

  // --- Power Control Logic ---
  int powerPercent = constrain(au16data[4], 0, 100);
  unsigned long elapsedTimeInCycle = currentTime - lastCycleStartTime;

  if (elapsedTimeInCycle >= CYCLE_LENGTH_MS)
  {
    lastCycleStartTime = currentTime;
    elapsedTimeInCycle = 0;
  }

  unsigned long onTimeMs = (CYCLE_LENGTH_MS * powerPercent) / 100;

  if (elapsedTimeInCycle < onTimeMs)
  {
    digitalWrite(SSR_PIN, HIGH);
  }
  else
  {
    digitalWrite(SSR_PIN, LOW);
  }

  // --- Debugging Output ---
  static unsigned long lastDebugPrintTime = 0;
  if (currentTime - lastDebugPrintTime >= 2000)
  { // Print debug info every 2 seconds
    lastDebugPrintTime = currentTime;

    Serial.print("Modbus PwrSet (AU4):");
    Serial.print(au16data[4]);
    Serial.print(" -> Actual %:");
    Serial.print(powerPercent);
    // Serial.print(", ON_Time_ms:"); Serial.print(onTimeMs); // onTimeMs might be from a previous cycle if power logic is offset
    Serial.print(", SSR:");
    Serial.print(digitalRead(SSR_PIN) == HIGH ? "ON" : "OFF");

    // Display temperatures from Modbus registers (what Artisan would see)
    float debugTempC1 = au16data[2] / 100.0;
    float debugTempC2 = au16data[3] / 100.0;

    Serial.print(" | T1(AU2):");
    Serial.print(debugTempC1, 2);
    Serial.print("C | T2(AU3):");
    Serial.print(debugTempC2, 2);
    Serial.println("C");
    Serial.println("---");
  }
}
