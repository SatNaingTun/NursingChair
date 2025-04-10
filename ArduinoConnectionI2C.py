import smbus
import time

SLAVE_ADDRESS = 0x08  # Arduino I2C address
bus = smbus.SMBus(1)   # Use I2C bus 1 (default for Raspberry Pi)

def read_from_arduino():
    try:
        data = bus.read_byte(SLAVE_ADDRESS)  # Read a single byte from Arduino
        return data
    except Exception as e:
        print(f"Error reading from Arduino: {e}")
        return None

while True:
    data = read_from_arduino()
    if data is not None:
        print(f"Received from Arduino: {hex(data)}")  # Print in hex format
    else:
        print("No data received.")
    time.sleep(1)
