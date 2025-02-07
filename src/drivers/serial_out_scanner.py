import serial
import time

# Set up the serial connection (make sure this matches the correct port, baud rate, and settings)
ser = serial.Serial('/dev/ttyAMA3', 19200, timeout=1)

# Wait for the connection to stabilize
time.sleep(2)

# Read and display data continuously
try:
    while True:
        # Check if there's any data available to read
        if ser.in_waiting > 0:
            # Read one byte of data
            byte = ser.read(1)
            print(f"Received: {byte.hex()}")
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
