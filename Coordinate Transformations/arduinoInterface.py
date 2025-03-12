# arduinoInterface.py
import serial

def read_arduino_data(arduino_port='/dev/ttyUSB0', baud_rate=9600):
    """
    Reads data from the Arduino and returns the parsed values.
    :param arduino_port: Serial port of the Arduino (default is '/dev/ttyUSB0')
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
            data = ser.read(6)
            distance = int.from_bytes(data[0:2], byteorder='little')
            servo1 = int.from_bytes(data[2:4], byteorder='little')
            servo2 = int.from_bytes(data[4:6], byteorder='little')
            confirmation = data[6]

            ser.close()  # Close the serial connection after reading
            return distance, servo1, servo2, confirmation
        else:
            print("Error: No data available.")
            return None
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None
