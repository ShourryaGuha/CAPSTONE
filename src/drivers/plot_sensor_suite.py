import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load data from CSV file
data = pd.read_csv("../../Car_Ride_10th/sensor_log40ish.csv")

# Extract necessary columns
time = pd.to_datetime(data['timestamp'])  # Convert timestamp to datetime for better handling
imu_ax = data['imu_ax']
imu_ay = data['imu_ay']
imu_az = data['imu_az']
imu_gx = data['imu_gx']
imu_gy = data['imu_gy']
imu_gz = data['imu_gz']
pressure = data['pressure']
altitude = data['altitude']
latitude = data['latitude']
longitude = data['longitude']
height = data['height']
velocity_x = data['velocity_x']
velocity_y = data['velocity_y']
velocity_z = data['velocity_z']
distance = data['distance']

# Apply a simple low-pass filter (moving average)
def low_pass_filter(data_series, window_size=5):
    return data_series.rolling(window=window_size, min_periods=1, center=True).mean()

# Apply low-pass filter to IMU acceleration and rotational velocity data
imu_ax_filtered = low_pass_filter(imu_ax)
imu_ay_filtered = low_pass_filter(imu_ay)
imu_az_filtered = low_pass_filter(imu_az)

imu_gx_filtered = low_pass_filter(imu_gx)
imu_gy_filtered = low_pass_filter(imu_gy)
imu_gz_filtered = low_pass_filter(imu_gz)

# Outlier rejection for distance
def reject_outliers(data_series, threshold=3.0):
    """Remove outliers based on Median Absolute Deviation (MAD) and apply a hard cutoff."""
    median = np.median(data_series)
    mad = np.median(np.abs(data_series - median))  # Compute MAD
    modified_z_scores = 0.6745 * (data_series - median) / mad  # Modified Z-score method
    
    # Apply MAD-based filtering + hard cutoff
    filtered_data = data_series.copy()
    filtered_data[(np.abs(modified_z_scores) > threshold) | (data_series > 600)] = np.nan  
    return filtered_data

distance_filtered = reject_outliers(distance)

### PLOTTING ###

# 1. Linear Accelerations
plt.figure(figsize=(10, 6))
plt.plot(time, imu_ax_filtered, label='imu_ax (filtered)', color='r')
plt.plot(time, imu_ay_filtered, label='imu_ay (filtered)', color='g')
plt.plot(time, imu_az_filtered, label='imu_az (filtered)', color='b')
plt.title('Linear Accelerations (Filtered)')
plt.xlabel('Time')
plt.ylabel('Acceleration (m/s²)')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 2. Rotational Velocities
plt.figure(figsize=(10, 6))
plt.plot(time, imu_gx_filtered, label='imu_gx (filtered)', color='r')
plt.plot(time, imu_gy_filtered, label='imu_gy (filtered)', color='g')
plt.plot(time, imu_gz_filtered, label='imu_gz (filtered)', color='b')
plt.title('Rotational Velocities (Filtered)')
plt.xlabel('Time')
plt.ylabel('Angular Velocity (rad/s)')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 3. Altitude vs. GPS Height
plt.figure(figsize=(10, 6))
plt.plot(time, altitude, label='Altitude (Barometer)', color='r')
plt.plot(time, height, label='Height (GPS)', color='g')
plt.title('Altitude vs Height')
plt.xlabel('Time')
plt.ylabel('Altitude / Height (m)')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 4. Distance (with Outlier Rejection)
plt.figure(figsize=(10, 6))
plt.plot(time, distance_filtered, label='Distance (Outliers Removed)', color='b')
plt.title('Distance (Filtered)')
plt.xlabel('Time')
plt.ylabel('Distance (m)')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 5. GPS Linear Velocity
plt.figure(figsize=(10, 6))
plt.plot(time, velocity_x, label='Velocity X', color='r')
plt.plot(time, velocity_y, label='Velocity Y', color='g')
plt.plot(time, velocity_z, label='Velocity Z', color='b')
plt.title('Linear Velocity (GPS)')
plt.xlabel('Time')
plt.ylabel('Velocity (m/s)')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()
