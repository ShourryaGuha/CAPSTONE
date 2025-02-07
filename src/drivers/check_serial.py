import serial

# Configure UART2 (ttyAMA2) with 9600 baud
ser = serial.Serial('/dev/ttyAMA3', baudrate=9600, timeout=1)

# Write to UART
message = "Hello, UART!\n"
ser.write(message.encode())

# Read from UART
response = ser.readline().decode().strip()

# Print response
print(f"Sent: {message.strip()}")
print(f"Received: {response}")

# Close the serial connection
ser.close()
