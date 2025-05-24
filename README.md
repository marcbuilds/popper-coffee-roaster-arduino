# Arduino Popper

## Arduino

### Libraries Required

- MAX6675-library https://github.com/adafruit/MAX6675-library
- Modbus-Master-Slave-for-Arduino https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino

MAX6675-library can be found in the Library Manager
Modbus-Master-Slave-for-Arduino needs to be added as a .zip. Download from github and then import using Arduino Library.

## Artisan

- Device = Modbus
- Use Slave 1
- Heater = write to register 4 (e.g. write([1,4,50]) is write to Slave 1, Register 4, value of 50)
- Air = write to register 5
- Thermocouples = read from registers 2/3

## Popper

### Parts

- Severin Popper
- Arduino Uno R3
- [SSR Relay](https://www.amazon.de/dp/B01N0Y4RJT)
- [PWM Controller](https://www.amazon.de/dp/B07911RQ2Z)
- [DC Booster](https://www.amazon.de/dp/B01EW5V24Q)
- 24v Laptop Charger for the fan
- DIY RC Low-Pass Filter for the fan

### Fan

#### RC Filter

Wiring Steps (Using a 3-Position Screw Terminal Block - T1, T2, T3):

1. POWER DOWN EVERYTHING: Ensure the Arduino and the Laptop Charger (powering the fan circuit) are completely disconnected from power.
2. Terminal T1 (Input from Arduino & R1 Start):

- Connect a wire from Arduino Digital Pin 6 (FAN_CONTROL_PIN) to terminal T1 of your screw block.
- Take your R1 resistor (e.g., 4.7kΩ). Insert one leg into terminal T1 (along with the wire from Pin 6) and tighten.

3. Terminal T2 (Node A - Smoothed Output, R1 End, C1 Positive, Output to Controller):

- Insert the other leg of R1 into terminal T2 and tighten.
- Take your C1 (10µF) capacitor. Identify the positive (+) leg. Insert this positive leg into terminal T2 and tighten.
- Take a new wire. Insert one end into terminal T2 and tighten. The other end of this wire will go to the Fan Motor Controller's Potentiometer WIPER/SIGNAL input terminal/pin.

4. Terminal T3 (Common Ground):

- Identify the negative (-) leg of C1. Insert this negative leg into terminal T3 and tighten.
- Take a wire. Connect one end to an Arduino GND pin. Insert the other end into terminal T3 and tighten.
- Take another new wire. Insert one end into terminal T3 and tighten. The other end of this wire will go to the Fan Motor Controller's Potentiometer GND input terminal/pin.

### Dimensions

- Roast Chamber 7cm
