# import serial
# import time

# # Open the serial port
# ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

# while True:
#     # Write "helloworld" to the serial port
#     ser.write(b'helloworld\n')  # Send as bytes

#     # Optional: Read response if needed
#     response = ser.readline().decode('utf-8', errors='ignore').strip()
#     if response:
#         print("Received:", response)

#     # Sleep to avoid flooding the port
#     time.sleep(1)

import serial
import time
import struct  # Used for packing integers into bytes
import subprocess

# Open the serial port
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=0.1)

while True:
    # Define four 2-byte (16-bit) integers
    int1 = 50    # Example value
    int2 = 90
    int3 = 100
    
    # Define two 1-byte (8-bit) values
    byte1 = 1     # Example 8-bit value (0-255)
    byte2 = 1    # Example 8-bit value (0-255)

    # Pack them as 3 little-endian 16-bit integers and 2 little-endian 8-bit integers
    data = struct.pack('>3H2B', int1, int2, int3, byte1, byte2)

    # Send the packed data
    ser.write(data)

    # Read **1 byte** response as an unsigned 8-bit integer
    response = ser.read(1)  # Read 1 byte
    # print(response)

    if response:
        received_value = struct.unpack('B', response)[0]  # 'B' = Unsigned 8-bit int
        print(f"Received 8-bit Integer: 0x{received_value:02X}")  # Format as hex

        if received_value == 0x01:  # Assuming 0x01 triggers the solenoid release
            try:
                # Run the solenoid control executable (./main) from the build directory
                subprocess.run(["build/main"], check=True)  # Use relative path to main
                print("Solenoid released.")
            except subprocess.CalledProcessError as e:
                print(f"Error executing solenoid control: {e}")
            while 1:
                print("Mission completed\n")

    # Sleep to avoid flooding the port
    # print("sent\n")
    # time.sleep(0.05)