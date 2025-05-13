// Libraries
#include <max6675.h>
#include <ModbusRtu.h>

// --- Pin Definitions ---
// SSR Control Pin (connected to SSR input)
const int SSR_PIN = 5; // Was 'led' in your original sketch

// Thermocouple 1 Pins (from your original sketch)
const int TC1_DO_PIN = 2;
const int TC1_CS_PIN = 3;
const int TC1_CLK_PIN = 4;

// Thermocouple 2 Pins (from your original sketch)
const int TC2_DO_PIN = 8;
const int TC2_CS_PIN = 9;
const int TC2_CLK_PIN = 7;

// --- MAX6675 Thermocouple Objects ---
// IMPORTANT: The original sketch had thermocouple and thermocouple2 somewhat swapped in naming
// Let's be explicit:
MAX6675 thermocouple1(TC1_CLK_PIN, TC1_CS_PIN, TC1_DO_PIN); // Uses pins 4, 3, 2
MAX6675 thermocouple2(TC2_CLK_PIN, TC2_CS_PIN, TC2_DO_PIN); // Uses pins 7, 9, 8

// --- Modbus Configuration ---
// Data array for Modbus network sharing
// Register map:
// au16data[0] - Unused (or for other data)
// au16data[1] - Unused (or for other data)
// au16data[2] - Thermocouple 1 Temperature (*100)
// au16data[3] - Thermocouple 2 Temperature (*100)
// au16data[4] - Power Output Setpoint (0-100%)
// ...
uint16_t au16data[16]; // Modbus register buffer, max 16 registers

/**
 * Modbus object declaration
 * u8id : node id = 1..247 for slave
 * u8serno : serial port (0 for Serial, 1 for Serial1, etc.)
 * u8txenpin : 0 for RS-232/USB-FTDI (no explicit transmit enable)
 *             or any pin number > 1 for RS-485 DE/RE pin
 */
Modbus slave(1, 0, 0); // Slave ID 1, Serial0 (standard USB/TTL serial), No TX_ENABLE pin

// --- Time-Proportional Control Variables ---
unsigned long lastCycleStartTime = 0;
const unsigned long CYCLE_LENGTH_MS = 1000; // 1000 ms = 1 second cycle for PWM. Adjust if needed.
                                            // Shorter can be smoother but puts more stress on SSR.

void setup()
{
  Serial.begin(19200); // For debugging output
  // Modbus slave communication
  slave.begin(19200); // Must match your Modbus master's baud rate

  // SSR Pin
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW); // Ensure heater is OFF at startup

  // Initialize Modbus registers (optional, but good practice for settable values)
  au16data[4] = 0; // Default power to 0%

  delay(500); // Allow thermocouples to stabilize if needed (MAX6675 is quick)
  Serial.println("Arduino Modbus Heater Control Ready.");
  Serial.print("SSR Pin: ");
  Serial.println(SSR_PIN);
  Serial.print("Cycle Length: ");
  Serial.print(CYCLE_LENGTH_MS);
  Serial.println(" ms");
}

void loop()
{
  // --- Modbus Communication ---
  // Poll for incoming Modbus requests and update au16data if master writes
  slave.poll(au16data, 16);

  // --- Read Thermocouples ---
  // It's good practice to read sensors at a reasonable interval, not necessarily every loop iteration
  // MAX6675 takes about 220ms to do a conversion.
  // For simplicity here, we read them every loop, but for very fast loops, you might add a timer.
  float tempC1 = thermocouple1.readCelsius();
  float tempC2 = thermocouple2.readCelsius();

  // Check for NaN or error readings (MAX6675 library might return specific values for errors)
  if (!isnan(tempC1))
  {
    au16data[2] = (uint16_t)(tempC1 * 100); // Store as integer (e.g., 25.50C becomes 2550)
  }
  else
  {
    // Handle error, e.g., keep last known good value or set to an error code
    // For now, let's indicate an error with a high value if you prefer, or 0
    // au16data[2] = 9999; // Example error indicator
    Serial.println("Error reading Thermocouple 1!");
  }

  if (!isnan(tempC2))
  {
    au16data[3] = (uint16_t)(tempC2 * 100);
  }
  else
  {
    // au16data[3] = 9999; // Example error indicator
    Serial.println("Error reading Thermocouple 2!");
  }

  // --- Power Control Logic ---
  // Get desired power level from Modbus register 4 (0-100%)
  int powerPercent = constrain(au16data[4], 0, 100);

  unsigned long currentTime = millis();
  unsigned long elapsedTimeInCycle = currentTime - lastCycleStartTime;

  // Check if the current PWM cycle has ended
  if (elapsedTimeInCycle >= CYCLE_LENGTH_MS)
  {
    lastCycleStartTime = currentTime; // Start a new cycle
    elapsedTimeInCycle = 0;           // Reset elapsed time for the new cycle
  }

  // Calculate how long the SSR should be ON in the current cycle
  unsigned long onTimeMs = (CYCLE_LENGTH_MS * powerPercent) / 100;

  // Control the SSR
  if (elapsedTimeInCycle < onTimeMs)
  {
    digitalWrite(SSR_PIN, HIGH); // Turn heater ON
  }
  else
  {
    digitalWrite(SSR_PIN, LOW); // Turn heater OFF
  }

  // --- Debugging Output (optional, can be removed or reduced for performance) ---
  static unsigned long lastDebugPrintTime = 0;
  if (currentTime - lastDebugPrintTime >= 2000)
  { // Print debug info every 2 seconds
    lastDebugPrintTime = currentTime;
    Serial.print("Modbus Power Set (Reg4): ");
    Serial.print(au16data[4]);
    Serial.print(" -> Actual Power%: ");
    Serial.print(powerPercent);
    Serial.print(", ON_Time_ms: ");
    Serial.print(onTimeMs);
    Serial.print(", SSR State: ");
    Serial.println(digitalRead(SSR_PIN) == HIGH ? "ON" : "OFF");
    Serial.print("Temp1: ");
    Serial.print(tempC1);
    Serial.print("C (Reg2: ");
    Serial.print(au16data[2]);
    Serial.println(")");
    Serial.print("Temp2: ");
    Serial.print(tempC2);
    Serial.print("C (Reg3: ");
    Serial.print(au16data[3]);
    Serial.println(")");
    Serial.println("---");
  }
}