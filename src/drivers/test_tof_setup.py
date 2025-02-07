import serial
import time

# Set up the serial connection (make sure this matches the correct port and baud rate)
ser = serial.Serial('/dev/ttyAMA3', 19200, timeout=1)

# Wait for the connection to stabilize
time.sleep(2)

# Define the byte to send
message = bytes([0x55])

# Infinite loop to send 0x55 and read the response
try:
    while True:
        # Write the message (0x55) to the UART
        print("writing 0x55")
        ser.write(message)
        
        # Read the response
        response = ser.read(1)  # Read 1 byte

        # If data was received, output the response
        if response:
            print(f"received {len(response)} byte(s): {response.hex()}")
        else:
            print("No response received.")
        
        # Wait before sending the next message (adjust interval if needed)
        time.sleep(1)  # 1 second interval
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
