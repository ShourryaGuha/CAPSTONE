import serial
import time

ser = serial.Serial('/dev/ttyAMA4', 115200, timeout=1)  # Adjust baud rate as needed

while True:
    msg = b'Hello UART\n'
    ser.write(msg)  # Send data
    time.sleep(0.5)  # Small delay

    if ser.in_waiting:
        received = ser.read(ser.in_waiting)  # Read incoming data
        print(f"Received: {received.decode(errors='ignore')}")  # Print received data
