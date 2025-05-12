import smbus
import time

SLAVE_ADDRESS = 0x08  # Arduino I2C address or Raspberry Pi receiver's I2C address
bus = smbus.SMBus(1)  # Use I2C bus 1 (default for Raspberry Pi)

def send_to_receiver(data):
    try:
        bus.write_byte(SLAVE_ADDRESS, data)  # Send a single byte to the receiver
        print(f"Sent: {hex(data)}")
    except Exception as e:
        print(f"Error sending to receiver: {e}")

# Example loop to send data
while True:
    data = 0x42  # Example byte to send (you can change this)
    send_to_receiver(data)
    time.sleep(1)  # Delay before sending the next byte