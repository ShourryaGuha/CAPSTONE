import serial
import time

# Set up the serial connection (make sure this matches the correct port and baud rate)
ser = serial.Serial('/dev/ttyAMA3', 19200, timeout=1)

# Wait for the connection to stabilize
time.sleep(2)

# Define the string to send
message = "cameron test"

# Send the message continuously at a set interval
try:
    while True:
        # Write the message to the UART
        ser.write(message.encode('utf-8'))  # Encode string to bytes before sending
        
        # Print to console for verification
        print(f"Sent: {message}")
        
        # Wait before sending the next message (set the rate as needed)
        time.sleep(1)  # 1 second interval, adjust as needed
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
