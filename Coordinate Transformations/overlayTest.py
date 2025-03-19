import cv2
import random
import math

# Load the video
input_video_path = "Coordinate Transformations/videos/demo.mp4"
cap = cv2.VideoCapture(input_video_path)

# Get original FPS and frame size
fps = int(cap.get(cv2.CAP_PROP_FPS))
frame_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
frame_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

# Define output video
output_video_path = "output.mp4"
fourcc = cv2.VideoWriter_fourcc(*"mp4v")
out = cv2.VideoWriter(output_video_path, fourcc, fps, (frame_width, frame_height))

# Base target position (center horizontally)
base_x = frame_width // 2
base_y_initial = int(frame_height * 0.85)  # initial vertical base position
base_y_after_12 = int(frame_height * 0.92)   # after 12 seconds, target shifts further down

# Parameters for circular motion
amplitude_base = 20       # average amplitude (radius)
delta_angle = 0.2         # base angular increment (radians)
angle = 0.0               # initial angle

# Probability to trigger an outlier in any given frame (5% chance)
outlier_prob = 0.05

frame_count = 0

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    frame_count += 1
    time_sec = frame_count / fps

    # Only display overlay from 3 to 24 seconds
    if 10 <= time_sec <= 20:
        # Choose base vertical position depending on time
        current_base_y = base_y_initial if time_sec < 12 else base_y_after_12

        # Calculate normal (noisy circular) target motion:
        # Vary the angular increment slightly for randomness.
        angle += delta_angle + random.uniform(-0.1, 0.1)
        # Vary the amplitude randomly using a Gaussian distribution.
        amplitude = random.gauss(amplitude_base, 5)
        offset_x = amplitude * 1.5 * math.cos(angle) + random.uniform(-10, 10)
        offset_y = amplitude * math.sin(angle) + random.uniform(-10, 10)
        target_x = int(base_x + offset_x)
        target_y = int(current_base_y + offset_y)

        # Occasionally, override with an outlier target position.
        if random.random() < outlier_prob:
            target_x = random.randint(0, frame_width)
            target_y = random.randint(0, frame_height)
            # Optionally, print a debug message to indicate an outlier.
            # print("Outlier triggered:", target_x, target_y)

        # Draw red target overlay: a small filled circle and a larger outline.
        cv2.circle(frame, (target_x, target_y), 8, (0, 0, 255), -1)
        cv2.circle(frame, (target_x, target_y), 40, (0, 0, 255), 2)

    # Write the modified frame to the output video
    out.write(frame)

    # Optionally display the frame (press 'q' to exit early)
    cv2.imshow("Overlayed Video", frame)
    if cv2.waitKey(int(1000 / fps)) & 0xFF == ord('q'):
        break

cap.release()
out.release()
cv2.destroyAllWindows()
