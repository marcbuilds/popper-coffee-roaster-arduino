import serial
import requests
import json
import math


def round_down_to_nearest_5(num):
    return math.floor(num / 10) * 10


# Get the arduino output
arduino = serial.Serial("COM3", 19200)
print(arduino)
datas = arduino.readline()
print(datas)
output = datas.decode("utf8")
arduino.close()

# Get the shelly power reading
shelly_data = requests.get("http://192.168.178.52/meter/0").text
shelly_power = json.loads(shelly_data)["power"]
rounded_shelly_power = round_down_to_nearest_5(round(shelly_power / 10, 0))

# print the arduino output with the shelly power
print(output + "," + str(rounded_shelly_power))
