import serial
import time
import struct
import subprocess
import os
import mmap

try:
    # Open the shared memory file
    angles_file_path = "/dev/shm/angles_data_file"
    
    if not os.path.exists(angles_file_path):
        print(f"Transmitter: Shared memory file not found at {angles_file_path}")
        print("Available shared memory segments:")
        os.system("ls -la /dev/shm/")
        exit(1)
    
    # Open the file for memory mapping
    angles_fd = os.open(angles_file_path, os.O_RDWR)
    angles_mmap = mmap.mmap(angles_fd, 3 * 8, mmap.MAP_SHARED, mmap.PROT_READ)
    
    print(f"Transmitter: Connected to shared memory at {angles_file_path}")
    
    # Open the serial port
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=0.1)
    print(f"Transmitter: Connected to serial port /dev/ttyACM0")
    
    while True:
        # Read the data from the shared memory
        angles_mmap.seek(0)
        data_bytes = angles_mmap.read(3 * 8)
        
        # Unpack as 3 doubles
        x, y, z = struct.unpack('ddd', data_bytes)
        
        # Convert to integers for transmission
        int1 = int(x)
        int2 = int(y)
        int3 = int(z)
        
        # Optional debugging output
        print(f"Transmitting - x: {int1}, y: {int2}, z: {int3}")
        
        # Define two 1-byte (8-bit) values
        byte1 = 1
        byte2 = 1
        
        # Pack them as 3 16-bit integers and 2 8-bit integers
        data = struct.pack('>3H2B', int1, int2, int3, byte1, byte2)
        
        # Send the packed data
        ser.write(data)
        
        # Read response
        response = ser.read(1)  # Read 1 byte
        
        if response:
            received_value = struct.unpack('B', response)[0]
            print(f"Received 8-bit Integer: 0x{received_value:02X}")
            
            if received_value == 0x01:
                try:
                    subprocess.run(["build/main"], check=True)
                    print("Solenoid released.")
                except subprocess.CalledProcessError as e:
                    print(f"Error executing solenoid control: {e}")
                print("Mission completed")
                break
        
        time.sleep(0.05)

except Exception as e:
    print(f"Transmitter error: {e}")

finally:
    if 'angles_mmap' in locals():
        angles_mmap.close()
    if 'angles_fd' in locals():
        os.close(angles_fd)
    if 'ser' in locals():
        ser.close()