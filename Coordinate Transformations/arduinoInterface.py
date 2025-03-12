import serial
import time
import struct

def read_arduino_data(arduino_port='COM7', baud_rate=115200):
    """
    Reads data from the Arduino and returns the parsed values.
    :param arduino_port: Serial port of the Arduino (default is 'COM11')
    :param baud_rate: Baud rate for the serial communication (default is 115200)
    :return: A tuple containing (distance, servo1, servo2, confirmation)
    """
    ser = serial.Serial(arduino_port, baud_rate, timeout=1)
    data_format = "<HHHBB"  # Little-endian: 3 uint16 + 2 uint8
    data_size = struct.calcsize(data_format)

    try:
        while True:
            if ser.in_waiting >= data_size:
                raw_data = ser.read(data_size)
                unpacked_data = struct.unpack(data_format, raw_data)

                distance, servo1, servo2, confirmation, connected = unpacked_data
                print(
                    f"Distance: {distance}, Servo1: {servo1}, Servo2: {servo2}, Confirmation: {confirmation}, Connected: {connected}")
                
                return distance, servo1, servo2, confirmation, connected
            
    except KeyboardInterrupt:
        print("Stopping serial read...")
    finally:
        ser.close()

def send_arduino_release_command(value, arduino_port='COM7', baud_rate=115200):
    """
    Sends a value to the Arduino over serial communication.
    :param value: The value to send to the Arduino (int or byte)
    :param arduino_port: Serial port of the Arduino (default is 'COM11')
    :param baud_rate: Baud rate for the serial communication (default is 115200)
    """
    try:
        # Set up serial communication with Arduino
        ser = serial.Serial(arduino_port, baud_rate)
        if not ser.is_open:
            ser.open()

        # Send the value to the Arduino as a single byte
        ser.write(bytes([value]))  # Send value as a byte
        print(f"Sent value {value} to Arduino")

        ser.close()  # Close the serial connection after sending
    except serial.SerialException as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    while True:
        # Read data from Arduino
        arduino_data = read_arduino_data()  # Call the function from arduino_reader.py

