import numpy as np
from scipy import interpolate
import os 
import pandas as pd
import matplotlib.pyplot as plt

# Calculate velocity using the extracted acceleration data and timestamps
def calculate_velocity_with_timestamps(accel_data, timestamps):
    velocity = np.zeros_like(accel_data)
    for i in range(1, len(timestamps)):
        dt = timestamps[i] - timestamps[i - 1]
        velocity[i] = velocity[i - 1] + accel_data[i - 1] * dt
    return velocity[:-1]  # Return one less point to match timestamps

def kalman_filter_speed(location_df, accelerometer_df):
    # Extract timestamps and measurements
    gps_times = location_df[location_df.columns[1]].values
    gps_speeds = location_df[location_df.columns[6]].values
    accel_times = accelerometer_df[accelerometer_df.columns[1]].values[:-1]  # Use one less point
    
    # Calculate acceleration magnitude
    accel_data = accelerometer_df.iloc[:, 2:5].values
    accel_magnitude = np.linalg.norm(accel_data, axis=1)
    
    # Initialize state [speed, acceleration]
    state = np.array([[gps_speeds[0]], [0.0]])
    
    # Initialize state covariance
    P = np.array([[100.0, 0.0],
                  [0.0, 100.0]])
    
    # Process noise covariance
    Q = np.array([[0.1, 0.0],
                  [0.0, 0.1]])
    
    # Measurement noise covariance
    R = np.array([[10.0]])
    
    # Measurement matrix (we only measure speed)
    H = np.array([[1.0, 0.0]])
    
    # Interpolate GPS speeds to match accelerometer timestamps
    gps_interp = interpolate.interp1d(gps_times, gps_speeds, 
                                    kind='linear', 
                                    fill_value='extrapolate')
    
    # Arrays to store results
    filtered_speeds = []
    timestamps = []
    
    # Process each timestamp
    for i in range(1, len(accel_times)):
        # Time delta
        dt = accel_times[i] - accel_times[i-1]
        
        # State transition matrix
        F = np.array([[1.0, dt],
                     [0.0, 1.0]])
        
        # Predict
        state = F @ state
        P = F @ P @ F.T + Q
        
        # Get interpolated GPS measurement
        measurement = np.array([[gps_interp(accel_times[i])]])
        
        # Update
        S = H @ P @ H.T + R
        K = P @ H.T @ np.linalg.inv(S)
        
        # Innovation
        y = measurement - H @ state
        
        # State update
        state = state + K @ y
        P = (np.eye(2) - K @ H) @ P
        
        filtered_speeds.append(state[0, 0])
        timestamps.append(accel_times[i])
    
    return np.array(timestamps), np.array(filtered_speeds)

# Define the path to the location file
location_file_path = os.path.join(os.getcwd(), 'Velocity Simulation', 'Location.csv')

# Read the location file into a DataFrame
location_df = pd.read_csv(location_file_path)

# Define the path to the accelerometer file
accelerometer_file_path = os.path.join(os.getcwd(), 'Velocity Simulation', 'Accelerometer.csv')

# Define the path to the accelerometer uncalibrated file
accelerometer_file_path_uncalibrated = os.path.join(os.getcwd(), 'Velocity Simulation', 'AccelerometerUncalibrated.csv')

# Read the accelerometer file into a DataFrame
accelerometer_df = pd.read_csv(accelerometer_file_path)

# Extract columns 4 to 2 (assuming they are the relevant acceleration data)
accel_data = accelerometer_df.iloc[:, 4:1:-1].values

# Initialize timestamps to column 1
timestamps = accelerometer_df.iloc[:, 1].values[:-1]  # Use one less point

velocity = calculate_velocity_with_timestamps(accel_data, accelerometer_df.iloc[:, 1].values)
speed = np.linalg.norm(velocity, axis=1)  # Speed magnitude at each time step

# Apply Kalman filter
kf_timestamps, filtered_speeds = kalman_filter_speed(location_df, accelerometer_df)

# Original GPS speed plot
plt.figure(figsize=(12, 4))
plt.plot(location_df[location_df.columns[1]], location_df[location_df.columns[6]], label='speed')
plt.xlabel('Unix Timestamp')
plt.ylabel('Speed')
plt.legend()
plt.title('GPS speed plot vs time')
plt.tight_layout()
plt.show()

# Integrated accelerometer speed plot
plt.figure(figsize=(12, 4))
plt.plot(timestamps, speed, label='Speed')
plt.xlabel(accelerometer_df.columns[0])
plt.ylabel('Speed')
plt.legend()
plt.title('Speed from Integrated Accelerometer Data')
plt.tight_layout()
plt.show()

# Raw accelerometer data plot
plt.figure(figsize=(12, 4))
for column in accelerometer_df.columns[2:5]:
    plt.plot(accelerometer_df[accelerometer_df.columns[0]], accelerometer_df[column], label=column)
plt.xlabel(accelerometer_df.columns[0])
plt.ylabel('Acceleration')
plt.legend()
plt.title('Accelerometer Data from Accelerometer.csv')
plt.tight_layout()
plt.show()

# Kalman filtered speed plot
plt.figure(figsize=(12, 4))
plt.plot(kf_timestamps, filtered_speeds, 'r-', label='Kalman Filtered Speed')
plt.plot(location_df[location_df.columns[1]], location_df[location_df.columns[6]], 'b.', alpha=0.3, label='GPS Speed')
plt.xlabel('Unix Timestamp')
plt.ylabel('Speed')
plt.legend()
plt.title('Kalman Filtered Speed vs GPS Speed')
plt.tight_layout()
plt.show()

# Calculate velocity using the extracted acceleration data and timestamps
# velocity = calculate_velocity_with_timestamps(accel_data, accelerometer_df.iloc[:, 1].values)
# speed = np.linalg.norm(velocity, axis=1)  # Speed magnitude at each time step

# # Apply Kalman filter
# kf_timestamps, filtered_speeds = kalman_filter_speed(location_df, accelerometer_df)

# # Create subplots
# fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(12, 16))

# # Original GPS speed plot
# ax1.plot(location_df[location_df.columns[1]], location_df[location_df.columns[6]], label='speed')
# ax1.set_xlabel('Unix Timestamp')
# ax1.set_ylabel('Speed')
# ax1.legend()
# ax1.set_title('Speed from Location.csv')

# # Integrated accelerometer speed plot
# ax3.plot(timestamps, speed, label='Speed')
# ax3.set_xlabel(accelerometer_df.columns[0])
# ax3.set_ylabel('Speed')
# ax3.legend()
# ax3.set_title('Speed from Integrated Accelerometer Data')

# # Raw accelerometer data plot
# for column in accelerometer_df.columns[2:5]:
#     ax4.plot(accelerometer_df[accelerometer_df.columns[0]], 
#              accelerometer_df[column], label=column)
# ax4.set_xlabel(accelerometer_df.columns[0])
# ax4.set_ylabel('Acceleration')
# ax4.legend()
# ax4.set_title('Accelerometer Data from Accelerometer.csv')

# # Kalman filtered speed plot
# ax2.plot(kf_timestamps, filtered_speeds, 'r-', label='Kalman Filtered Speed')
# ax2.plot(location_df[location_df.columns[1]], 
#     location_df[location_df.columns[6]], 
#      'b.', alpha=0.3, label='GPS Speed')
# ax2.set_xlabel('Unix Timestamp')
# ax2.set_ylabel('Speed')
# ax2.legend()
# ax2.set_title('Kalman Filtered Speed vs GPS Speed')

# plt.tight_layout()
# plt.show()