import serial
import time
import struct

def read_arduino_data(arduino_port='COM7', baud_rate=9600):
    """
    Reads data from the Arduino and returns the parsed values.
    :param arduino_port: Serial port of the Arduino (default is 'COM7')
    :param baud_rate: Baud rate for the serial communication (default is 115200)
    :return: A tuple containing (distance, servo1, servo2, confirmation, connected)
    """
    ser = serial.Serial(arduino_port, baud_rate, timeout=0.05)
    data_format = ">3H2B"  # Big-endian: 3 signed 16-bit ints + 2 unsigned 8-bit ints
    data_size = struct.calcsize(data_format)

    try:
        while True:
            if ser.in_waiting >= data_size:
                raw_data = ser.read(data_size)
                unpacked_data = struct.unpack(data_format, raw_data)

                # Convert the received values into named variables
                distance = int(unpacked_data[0])
                servo1 = int(unpacked_data[1])
                servo2 = int(unpacked_data[2])
                confirmation = int(unpacked_data[3])
                connected = int(unpacked_data[4])
                
                print(f"Received: distance={distance}, servo1={servo1}, servo2={servo2}, confirmation={confirmation}, connected={connected}")
                return distance, servo1, servo2, confirmation, connected
            
    except KeyboardInterrupt:
        print("Stopping serial read...")
    finally:
        ser.close()

def send_arduino_release_command(value, arduino_port='COM7', baud_rate=9600):
    """
    Sends a value to the Arduino over serial communication.
    :param value: The value to send to the Arduino (int or byte)
    :param arduino_port: Serial port of the Arduino (default is 'COM11')
    :param baud_rate: Baud rate for the serial communication (default is 115200)
    """
    try:
        # Open the serial port using a context manager (ensures the port closes automatically)
        with serial.Serial(arduino_port, baud_rate, timeout=1) as ser:
            # time.sleep(2)  # Wait for the Arduino to reset after opening the port
            
            # Send the value as a single byte
            ser.write(bytes([value]))
            print(f"Sent value {value} to Arduino")
            
            # Wait a short moment for Arduino to process and respond
            # time.sleep(0.5)
            
            # Read and print any available response from the Arduino
            # while ser.in_waiting:
            #     response = ser.readline().decode().strip()
            #     print("Arduino says:", response)
                
    except serial.SerialException as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    while True:
        # Read data from Arduino
        arduino_data = send_arduino_release_command(1)  # Call the function from arduino_reader.py

