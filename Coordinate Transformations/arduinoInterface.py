import serial
import time

def read_arduino_data(arduino_port='COM7', baud_rate=9600):
    """
    Reads data from the Arduino and returns the parsed values.
    :param arduino_port: Serial port of the Arduino (default is 'COM7')
    :param baud_rate: Baud rate for the serial communication (default is 9600)
    :return: A tuple containing (distance, servo1, servo2, confirmation)
    """
    # Set up serial communication with Arduino
    try:
        ser = serial.Serial(arduino_port, baud_rate)
        if not ser.is_open:
            ser.open()

        # Read 6 bytes: 2 for distance, 2 for servo1, 2 for servo2, and 1 for confirmation
        if ser.in_waiting > 0:
            data = ser.read(8)
            distance = int.from_bytes(data[0:2], byteorder='little')
            servo1 = int.from_bytes(data[2:4], byteorder='little')
            servo2 = int.from_bytes(data[4:6], byteorder='little')
            confirmation = data[6]
            connected = data[7]

            ser.close()  # Close the serial connection after reading
            return distance, servo1, servo2, confirmation, connected
        else:
            print("Error: No data available.")
            return None
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def send_arduino_release_command(value, arduino_port='COM7', baud_rate=9600):
    """
    Sends a value to the Arduino over serial communication.
    :param value: The value to send to the Arduino (int or byte)
    :param arduino_port: Serial port of the Arduino (default is 'COM7')
    :param baud_rate: Baud rate for the serial communication (default is 9600)
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
