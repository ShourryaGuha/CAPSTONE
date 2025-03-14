import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load data from CSV file
data = pd.read_csv("c:/Users/guhas/src/CAPSTONE/Running_12th/ekf_state_log_W_fix.csv")

# Extract necessary columns
time = pd.to_datetime(data['timestamp'])  # Convert timestamp to datetime for better handling
kf_vx = data['kf_vx']
kf_vy = data['kf_vy']
kf_vz = data['kf_vz']
kf_roll = data['kf_roll']
kf_pitch = data['kf_pitch']
gyro_roll_degrees = data['gyro_roll_degrees']
gyro_pitch_degrees = data['gyro_pitch_degrees']
accel_roll_degrees = data['accel_roll_degrees']
accel_pitch_degrees = data['accel_pitch_degrees']
alpha_x = data['alpha_x']
alpha_y = data['alpha_y']
alpha_z = data['alpha_z']
filtered_gyro_x = data['filtered_gyro_x']
filtered_gyro_y = data['filtered_gyro_y']
filtered_gyro_z = data['filtered_gyro_z']

# Apply a simple low-pass filter (moving average)
def low_pass_filter(data_series, window_size=5):
    return data_series.rolling(window=window_size, min_periods=1, center=True).mean()

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


### PLOTTING ###

# 1. KF Roll and Pitch
plt.figure(figsize=(10, 6))
plt.plot(time, kf_roll, label='kf_roll_about_y', color='r')
plt.plot(time, kf_pitch, label='kf_pitch_about_x', color='g')
plt.title('Roll and Pitch Estimation (EKF)')
plt.xlabel('Time')
plt.ylabel('degrees')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 2. Gyro Roll and Pitch, Accel Roll and Pitch
plt.figure(figsize=(10, 6))
plt.plot(time, gyro_roll_degrees, label='gyro_roll_degrees', color='b')
plt.plot(time, gyro_pitch_degrees, label='gyro_pitch_degrees', color='c')
plt.plot(time, accel_roll_degrees, label='accel_roll_degrees', color='m')
plt.plot(time, accel_pitch_degrees, label='accel_pitch_degrees', color='y')
plt.title('Gyro and Accel Roll and Pitch')
plt.xlabel('Time')
plt.ylabel('degrees')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 3. Accel Roll and Pitch vs Time
plt.figure(figsize=(10, 6))
plt.plot(time, accel_roll_degrees, label='accel_roll_degrees', color='m')
plt.plot(time, accel_pitch_degrees, label='accel_pitch_degrees', color='y')
plt.title('Accel Roll and Pitch vs Time')
plt.xlabel('Time')
plt.ylabel('degrees')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()

# 4. Gyro Roll and Pitch vs Time
plt.figure(figsize=(10, 6))
plt.plot(time, gyro_roll_degrees, label='gyro_roll_degrees', color='b')
plt.plot(time, gyro_pitch_degrees, label='gyro_pitch_degrees', color='c')
plt.title('Gyro Roll and Pitch vs Time')
plt.xlabel('Time')
plt.ylabel('degrees')
plt.legend()
plt.grid()
plt.xticks([])  # Remove x-axis labels
plt.show()