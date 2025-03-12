import serial
import time

def read_arduino_data(arduino_port='COM11', baud_rate=115200):
    """
    Reads data from the Arduino and returns the parsed values.
    :param arduino_port: Serial port of the Arduino (default is 'COM11')
    :param baud_rate: Baud rate for the serial communication (default is 115200)
    :return: A tuple containing (distance, servo1, servo2, confirmation)
    """
    # Set up serial communication with Arduino
    try:
        ser = serial.Serial(arduino_port, baud_rate, timeout=1)
        if not ser.is_open:
            ser.open()

        time.sleep(0.1)

        # Read a line from the serial port
        line = ser.readline().decode('utf-8').strip()

        # Read 5 values from string: distance, servo1, servo2, confirmation, connected
        if line:
            values = line.split(',')
            if len(values) == 5:  # Ensure we received 5 values
                distance = int(values[0])
                servo1 = int(values[1])
                servo2 = int(values[2])
                confirmation = int(values[3])
                connected = int(values[4])
                return distance, servo1, servo2, confirmation, connected
            else:
                print("Error: Received incorrect number of values.")
                return None
        else:
            print("Error: No data available.")
            return None
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def send_arduino_release_command(value, arduino_port='COM11', baud_rate=115200):
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

    time.sleep(0.1)
