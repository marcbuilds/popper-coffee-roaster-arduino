// Libraries
#include <max6675.h>
#include <ModbusRtu.h>

// --- Pin Definitions ---
const int SSR_PIN = 5; // Heater control

// Thermocouple 1 Pins
const int TC1_DO_PIN = 2;
const int TC1_CS_PIN = 3;
const int TC1_CLK_PIN = 4;

// Thermocouple 2 Pins
const int TC2_DO_PIN = 8;
const int TC2_CS_PIN = 9;
const int TC2_CLK_PIN = 7;

// Fan Control Pin Definition
const int FAN_CONTROL_PIN = 6;

// --- Global Configuration Constants ---
const int MIN_FAN_PERCENT_IF_HEATER_ON = 20;

// --- MAX6675 Thermocouple Objects ---
MAX6675 thermocouple1(TC1_CLK_PIN, TC1_CS_PIN, TC1_DO_PIN);
MAX6675 thermocouple2(TC2_CLK_PIN, TC2_CS_PIN, TC2_DO_PIN);

// --- Modbus Configuration ---
uint16_t au16data[16];
Modbus slave(1, 0, 0);

// --- Time-Proportional Control Variables (for Heater) ---
unsigned long lastCycleStartTime = 0;
const unsigned long CYCLE_LENGTH_MS = 1000;

// --- Thermocouple Reading Timer ---
unsigned long lastTempReadTime = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 1000;

void setup()
{
  Serial.begin(19200);
  slave.begin(19200);

  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);

  pinMode(FAN_CONTROL_PIN, OUTPUT);
  analogWrite(FAN_CONTROL_PIN, 0);

  au16data[2] = 0;
  au16data[3] = 0;
  au16data[4] = 0;
  au16data[5] = 0;

  // Ensure MAX6675 CS pins are initially high (deselected)
  // The library constructor should do this, but being explicit doesn't hurt.
  digitalWrite(TC1_CS_PIN, HIGH);
  digitalWrite(TC2_CS_PIN, HIGH);

  delay(500); // Allow sensors to stabilize

  Serial.println("Arduino Modbus Heater & Fan Control Ready.");
  Serial.print("SSR Pin (Heater): ");
  Serial.println(SSR_PIN);
  Serial.print("Fan Control Pin (PWM): ");
  Serial.println(FAN_CONTROL_PIN);
  Serial.print("Min Fan % if Heater ON: ");
  Serial.println(MIN_FAN_PERCENT_IF_HEATER_ON);
  Serial.print("Heater Cycle Length: ");
  Serial.print(CYCLE_LENGTH_MS);
  Serial.println(" ms");
  Serial.print("Temp Read Interval: ");
  Serial.print(TEMP_READ_INTERVAL_MS);
  Serial.println(" ms");
  Serial.println("---------------------------------------------");
}

void loop()
{
  unsigned long currentTime = millis(); // Get current time once per loop

  slave.poll(au16data, 16);

  if (currentTime - lastTempReadTime >= TEMP_READ_INTERVAL_MS)
  {
    lastTempReadTime = currentTime;
    float tempC1 = NAN, tempC2 = NAN;

    digitalWrite(TC2_CS_PIN, HIGH); // Ensure other thermocouple is deselected
    delayMicroseconds(50);          // Short delay for CS lines to settle
    tempC1 = thermocouple1.readCelsius();
    digitalWrite(TC1_CS_PIN, HIGH);

    if (!isnan(tempC1))
    {
      au16data[2] = (uint16_t)(tempC1 * 100);
    }

    delay(20); // Brief pause between reading different SPI devices

    digitalWrite(TC1_CS_PIN, HIGH); // Ensure other thermocouple is deselected
    delayMicroseconds(50);          // Short delay for CS lines to settle
    tempC2 = thermocouple2.readCelsius();
    digitalWrite(TC2_CS_PIN, HIGH);

    if (!isnan(tempC2))
    {
      au16data[3] = (uint16_t)(tempC2 * 100);
    }
  }

  // --- Power Control Logic ---
  int heaterPowerPercent = constrain(au16data[4], 0, 100);
  unsigned long elapsedTimeInCycle = currentTime - lastCycleStartTime;

  if (elapsedTimeInCycle >= CYCLE_LENGTH_MS)
  {
    lastCycleStartTime = currentTime;
    elapsedTimeInCycle = 0;
  }

  unsigned long heaterOnTimeMs = (CYCLE_LENGTH_MS * heaterPowerPercent) / 100;

  if (elapsedTimeInCycle < heaterOnTimeMs)
  {
    digitalWrite(SSR_PIN, HIGH);
  }
  else
  {
    digitalWrite(SSR_PIN, LOW);
  }

  int requestedFanPowerPercent = constrain(au16data[5], 0, 100);
  int actualFanPowerPercent = requestedFanPowerPercent;

  if (heaterPowerPercent > 0)
  {
    if (actualFanPowerPercent < MIN_FAN_PERCENT_IF_HEATER_ON)
    {
      actualFanPowerPercent = MIN_FAN_PERCENT_IF_HEATER_ON;
    }
  }

  int fanPwmValue = map(actualFanPowerPercent, 0, 100, 0, 255);
  analogWrite(FAN_CONTROL_PIN, fanPwmValue);

  static unsigned long lastDebugPrintTime = 0;
  if (currentTime - lastDebugPrintTime >= 2000)
  {
    lastDebugPrintTime = currentTime;

    Serial.print("HtrPwr(AU4):");
    Serial.print(au16data[4]);
    Serial.print("% Act:");
    Serial.print(heaterPowerPercent);
    Serial.print("% SSR:");
    Serial.print(digitalRead(SSR_PIN) == HIGH ? "ON" : "OFF");

    Serial.print(" | FanReq(AU5):");
    Serial.print(requestedFanPowerPercent);
    Serial.print("% Act:");
    Serial.print(actualFanPowerPercent);
    Serial.print("% PWM:");
    Serial.print(fanPwmValue);

    float debugTempC1 = au16data[2] / 100.0;
    float debugTempC2 = au16data[3] / 100.0;

    Serial.print(" | T1(AU2):");
    Serial.print(debugTempC1, 1);
    Serial.print("C, T2(AU3):");
    Serial.print(debugTempC2, 1);
    Serial.println("C");
  }
}