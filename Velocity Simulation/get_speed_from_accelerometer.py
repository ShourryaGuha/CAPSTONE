import pandas as pd
import os
import matplotlib.pyplot as plt
from scipy.integrate import cumulative_trapezoid
import numpy as np

# Define the path to the location file
location_file_path = os.path.join(os.getcwd(), 'Location.csv')

# Read the location file into a DataFrame
location_df = pd.read_csv(location_file_path)

# Define the path to the accelerometer file
accelerometer_file_path = os.path.join(os.getcwd(), 'Accelerometer.csv')

# Define the path to the accelerometer uncalibrated file
accelerometer_file_path_uncalibrated = os.path.join(os.getcwd(), 'AccelerometerUncalibrated.csv')

# Read the accelerometer file into a DataFrame
accelerometer_df = pd.read_csv(accelerometer_file_path)

# Create subplots
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 12))

# Plot the speed column from location.csv in the first subplot using Unix timestamp
ax1.plot(location_df[location_df.columns[1]], location_df[location_df.columns[6]], label='speed')
ax1.set_xlabel('Unix Timestamp')
ax1.set_ylabel('Speed')
ax1.legend()
ax1.set_title('Speed from Location.csv')

# Plot columns 2 to 4 against column 0 from accelerometer.csv in the second subplot
for column in accelerometer_df.columns[2:5]:
    ax3.plot(accelerometer_df[accelerometer_df.columns[0]], accelerometer_df[column], label=column)
ax3.set_xlabel(accelerometer_df.columns[0])
ax3.set_ylabel('Acceleration')
ax3.legend()
ax3.set_title('Accelerometer Data from Accelerometer.csv')

def calculate_velocity_with_timestamps(accel_data, timestamps):
    """
    Calculate velocity from accelerometer data with timestamps.
    
    Parameters:
    accel_data (np.array): Nx3 array where each row is [a_x, a_y, a_z].
    timestamps (np.array): 1D array of timestamps corresponding to the accelerometer data.
    
    Returns:
    np.array: Nx3 array of velocity in x, y, z directions.
    """
    velocity = np.zeros_like(accel_data)
    for i in range(1, len(timestamps)):
        # Calculate time difference in seconds
        dt = timestamps[i] - timestamps[i - 1]
        
        # Integrate acceleration to update velocity
        velocity[i] = velocity[i - 1] + accel_data[i - 1] * dt
    
    return velocity

# Read the accelerometer file into a DataFrame
accelerometer_df = pd.read_csv(accelerometer_file_path)

# Extract columns 4 to 2 (assuming they are the relevant acceleration data)
accel_data = accelerometer_df.iloc[:, 4:1:-1].values

# Initialize timestamps to column 1
timestamps = accelerometer_df.iloc[:, 1].values

# Calculate velocity using the extracted acceleration data and timestamps
velocity = calculate_velocity_with_timestamps(accel_data, timestamps)
speed = np.linalg.norm(velocity, axis=1)  # Speed magnitude at each time step

# Plot the velocity in the third subplot
ax2.plot(timestamps, speed, label='Speed')
ax2.set_xlabel(accelerometer_df.columns[0])
ax2.set_ylabel('Speed')
ax2.legend()
ax2.set_title('Speed from Integrated Accelerometer Data')

# Adjust layout
plt.tight_layout()

# Show the plot
plt.show()