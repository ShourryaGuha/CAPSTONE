import cv2
import numpy as np
from cameraTransform import project_3d_to_2d
from arduinoInterface import read_arduino_data

# Define a single 3D point in camera coordinates
point_3d = np.array([3.0, 1.0, 10.0])  # Example: (X, Y, Z)

# Intrinsic parameter matrix (camera matrix K)
K = np.array([[477.00580736,   0,  511.86699298],
              [  0, 625.7461253,  371.52458483],
              [  0,   0,    1]])

# Distortion coefficients vector
# Format: [k1, k2, p1, p2, k3]
dist = np.array([-0.29936293, 0.10959204, 0.00269389, -0.00213927, -0.02094805])

# Open the video feed from the EWRF OTG receiver
cap = cv2.VideoCapture(0)  # Adjust index if needed

if not cap.isOpened():
    print("Error: Could not open video stream.")
    exit()


while True:
    # Read data from Arduino
    arduino_data = read_arduino_data()  # Call the function from arduino_reader.py
    
    if arduino_data:
        distance, servo1, servo2, confirmation = arduino_data
        print(f"Distance: {distance}, Servo1: {servo1}, Servo2: {servo2}, Confirmation: {confirmation}")
    else:
        print("No data received from Arduino.")

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
    if 0 <= x < frame.shape[1] and 0 <= y < frame.shape[0]:  # Check if within bounds
        cv2.circle(frame, (x,y), 8, (0, 255, 0), -1)
        cv2.circle(frame, (x,y), 40, (0, 255, 0), 2)
        cv2.circle(frame, (x,y), 40, (0, 0, 0), 3)  
        cv2.circle(frame, (x,y), 40, (0, 255, 0), 2)  

    # Show the modified video feed
    cv2.imshow("Drone Feed with Overlay", frame)

    # Check if 'q' is pressed
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break

# Release resources
cap.release()
cv2.destroyAllWindows()
