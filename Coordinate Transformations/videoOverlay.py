import cv2
import numpy as np
import serial
from cameraTransform import project_3d_to_2d
from arduinoInterface import read_arduino_data, send_arduino_release_command

# Define a single 3D point in camera coordinates
point_3d = np.array([3.0, 1.0, 10.0])  # Example: (X, Y, Z)

# Intrinsic parameter matrix (camera matrix K)
K = np.array([[477.00580736,   0,  511.86699298],
              [  0, 625.7461253,  371.52458483],
              [  0,   0,    1]])

# Distortion coefficients vector
dist = np.array([-0.29936293, 0.10959204, 0.00269389, -0.00213927, -0.02094805])

# Open the video feed from the EWRF OTG receiver
cap = cv2.VideoCapture(0)  # Adjust index if needed

if not cap.isOpened():
    print("Error: Could not open video stream.")
    exit()

# Define colour indicators for confirmation status
# Red: Not confirmed, Yellow: Pending, Green: Confirmed
colourIndicators = [(0, 0, 255), (0, 255, 255), (0, 255, 0)]

# initialize values for data received 
distance, servo1, servo2, confirmation, connected = 0, 0, 0, 0, 1

# Flag to check if release command has been sent
released = False

ser = serial.Serial('COM7', baudrate=9600, timeout=0.05)

while True:

    point_3d += 0.1
    # initialize values for data received 
    # distance, servo1, servo2, confirmation, connected = 0, 0, 0, 0, 0

    # Read data from Arduino
    if not released:
        arduino_data = read_arduino_data(ser=ser)  # Call the function from arduino_reader.py
    
    if arduino_data:
        distance, servo1, servo2, confirmation, connected = arduino_data
        print(f"Distance: {distance}, Servo1: {servo1}, Servo2: {servo2}, Confirmation: {confirmation}")
    else:
        print("No data received from Arduino.")

    #distance, servo1, servo2, confirmation, connected = 0, 0, 0, 0, 0

    print(distance, servo1, servo2, confirmation, connected)

    # If connected is 0, just show "No Connection"
    if connected == 0:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to capture frame.")
            break

        # Resize the frame to 1010x757 (Based on calibration)
        frame = cv2.resize(frame, (1010, 757))

        # Display "No Connection" on the frame
        font = cv2.FONT_HERSHEY_SIMPLEX
        cv2.putText(frame, "No Connection", (700, 100), font, 1, (0, 0, 255), 2, cv2.LINE_AA)

        # Show the modified video feed
        cv2.imshow("Drone Feed with Overlay", frame)

    else:
        # --- 
        # Perform coordinate transformations if connected
        # TODO: Convert distance, servo1, and servo2 to 3D point coordinates
        # --- 
        
        # Project the 3D point to 2D
        pixel_coordinates = project_3d_to_2d(point_3d, K, dist)

        # Validate the result
        if pixel_coordinates is None or pixel_coordinates.shape != (2,):
            print("Error: Invalid projected pixel coordinates.")
            exit()

        # Extract (x, y) from the numpy array
        x, y = int(pixel_coordinates[0]), int(pixel_coordinates[1])

        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to capture frame.")
            break

        # Resize the frame to 1010x757 (Based on calibration)
        frame = cv2.resize(frame, (1010, 757))

        # Draw the projected point onto the frame
        # if 0 <= x < frame.shape[1] and 0 <= y < frame.shape[0]:  # Check if within bounds
            # if released:
            #     cv2.circle(frame, (x, y), 8, (128, 128, 128), -1)
            #     cv2.circle(frame, (x, y), 40, (128, 128, 128), 2)
            # else:
            #     cv2.circle(frame, (x, y), 8, colourIndicators[confirmation], -1)
            #     cv2.circle(frame, (x, y), 40, colourIndicators[confirmation], 2)

        # Show the modified video feed
        cv2.imshow("Drone Feed with Overlay", frame)

    # Break out of loop if 'q' is pressed
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break

    # Check if Enter is pressed to set release flag
    if key == ord('\r') or key == ord('\n'):  # Check for Enter key press
        print("Enter key pressed, sending release command to Arduino.")
        released = True

    # Send release command to Arduino if flag is set
    if released:
        ser.write(b'0x1')


# Release resources
ser.close()
cap.release()
cv2.destroyAllWindows()
