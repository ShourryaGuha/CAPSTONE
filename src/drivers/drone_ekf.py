### TODO: implement quarternion stuff to remove influence of gravity

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.spatial.transform import Rotation as R

# Load data from CSV file
data = pd.read_csv("/home/shourryag/Documents/Shourrya_Capstone/CAPSTONE/Car_Ride_10th/sensor_log40ish.csv")
time = pd.to_datetime(data['timestamp'])  # Convert timestamp to datetime for better handling
imu_ax = data['imu_ax']
imu_ay = data['imu_ay']
imu_az = data['imu_az']

imu_gyro_x = data['imu_gx']
imu_gyro_y = data['imu_gy']
imu_gyro_z = data['imu_gz']

velocity_x = data['velocity_x']
velocity_y = data['velocity_y']
velocity_z = data['velocity_z']

# Apply a simple low-pass filter (moving average)
def low_pass_filter(data_series, window_size=5):
    return data_series.rolling(window=window_size, min_periods=1, center=True).mean()

# Apply low-pass filter to IMU acceleration and rotational velocity data
imu_ax_filtered = low_pass_filter(imu_ax)
imu_ay_filtered = low_pass_filter(imu_ay)
imu_az_filtered = low_pass_filter(imu_az)

class EKF_Drone:
    def __init__(self, dt, process_noise, measurement_noise):
        """
        Extended Kalman Filter for velocity estimation using GPS and accelerometer.

        Parameters:
        dt: float
            Time step (seconds)
        process_noise: float
            Process noise variance (for acceleration)
        measurement_noise: float
            Measurement noise variance (for GPS velocity)
        """
        self.dt = dt  # Time step
        
        # State vector [vx, vy, vz]
        self.x = np.zeros((3, 1))  # Initial velocity estimate (vx, vy, vz)

        # State covariance matrix (initial uncertainty)
        self.P = np.diag([10, 10, 10])  # Large uncertainty initially

        # State transition matrix (F) - Identity since velocity remains unless changed by acceleration
        self.F = np.eye(3)

        # Control input matrix (B) - Models effect of acceleration on velocity
        self.B = np.eye(3) * dt

        # Measurement matrix (H) - Direct mapping from velocity state to measurement
        self.H = np.eye(3)

        # Process noise covariance (Q) - Uncertainty in acceleration
        self.Q = np.eye(3) * process_noise

        # Measurement noise covariance (R) - Uncertainty in GPS velocity
        self.R = np.eye(3) * measurement_noise

    def predict(self, accel_x, accel_y, accel_z):
        # Convert acceleration to control input (3x1 vector)
        u = np.array([accel_x, accel_y, accel_z]).reshape(3, 1)

        # Predict state (velocity update based on acceleration)
        self.x = self.F @ self.x + self.B @ u

        # Predict covariance
        self.P = self.F @ self.P @ self.F.T + self.Q

    def update(self, gps_velocity_x, gps_velocity_y, gps_velocity_z):
        # Convert GPS velocity to measurement (3x1 vector)
        z = np.array([gps_velocity_x, gps_velocity_y, gps_velocity_z]).reshape(3, 1)

        # Compute Kalman Gain
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)

        # Update state estimate
        self.x = self.x + K @ (z - self.H @ self.x)

        # Update covariance
        I = np.eye(3)
        self.P = (I - K @ self.H) @ self.P

    def get_state(self):
        """ Returns the estimated velocity (vx, vy, vz) """
        return self.x.flatten()
    
    def calculate_roll_pitch(self, accel_x, accel_y, accel_z, gyro_x, gyro_y, dt, alpha, prev_roll, prev_pitch):
        accel_roll = np.arctan2(-accel_x, np.sqrt(accel_y**2 + accel_z**2))  # Roll (about Y-axis)
        accel_pitch = np.arctan2(accel_y, np.sqrt(accel_x**2 + accel_z**2))  # Pitch (about X-axis)

        # roll_degrees = np.degrees(accel_roll)
        # pitch_degrees = np.degrees(accel_pitch)
        
        # print(f"Roll: {accel_roll:.2f} radians, Pitch: {accel_pitch:.2f} radians")
        # print(f"Roll: {roll_degrees:.2f} degrees, Pitch: {pitch_degrees:.2f}")

        if prev_roll == 0 or prev_pitch == 0:
            roll, pitch = accel_roll, accel_pitch
        else:
            # Gyroscope integration for smooth roll & pitch updates
            roll = alpha * (prev_roll + gyro_y * dt) + (1 - alpha) * accel_roll
            pitch = alpha * (prev_pitch + gyro_x * dt) + (1 - alpha) * accel_pitch

        return roll, pitch 

# # Example usage:
# dt = 0.1  # Time step (100ms)
# process_noise = 0.1  # Process noise variance (acceleration noise)
# measurement_noise = 1.0  # Measurement noise variance (GPS velocity noise)

# ekf = EKF_Drone(dt, process_noise, measurement_noise)

# # Loop over the data (for each time step)
# predicted_states = []

# roll = 0  # about y-axis in the direction of nose of the drone
# pitch = 0   # about x-axis in the direction of left side of the assembled drone

# # Complimentary filter
# alpha = 0.98

# # Lists to store roll, pitch, and time values
# roll_values = []
# pitch_values = []
# # time_values = []

# for i in range(len(imu_ax_filtered)):

#     roll, pitch = ekf.calculate_roll_pitch(imu_ax_filtered[i], imu_ay_filtered[i], imu_az_filtered[i], imu_gyro_x[i], imu_gyro_y[i], dt, alpha, roll, pitch)

#     #  # Print roll and pitch angles
#     # print(f"Step {i}: Roll = {np.degrees(roll):.2f} degrees, Pitch = {np.degrees(pitch):.2f} degrees")

#     # Store roll, pitch, and time values
#     roll_values.append(np.degrees(roll))
#     pitch_values.append(np.degrees(pitch))
#     # time_values.append(time[i])

#     # assuming correct coordinate transfomr from IMU to world frame from chatgpt

#     # R_mat = np.array([
#     #     [np.cos(pitch), 0, -np.sin(pitch)],
#     #     [np.sin(roll) * np.sin(pitch), np.cos(roll), np.sin(roll) * np.cos(pitch)],
#     #     [np.cos(roll) * np.sin(pitch), -np.sin(roll), np.cos(roll) * np.cos(pitch)]
#     # ])

#     # # Convert acceleration to world frame & remove gravity
#     # accel_world = R_mat @ np.array([imu_ax_filtered[i], imu_ay_filtered[i], imu_az_filtered[i]])

#     # # might need to multiple g = np.array([0, 0, -9.81]) with R_mat, check math and chatgpt
#     # accel_linear = accel_world - np.array([0, 0, -9.81])

#     # # Access x, y, and z values of accel_linear
#     # accel_linear_x = accel_linear[0]
#     # accel_linear_y = accel_linear[1]
#     # accel_linear_z = accel_linear[2]

#     # ekf.predict(accel_linear_x, accel_linear_y, accel_linear_z)

#     # ekf.update(velocity_x[i], velocity_y[i], velocity_z[i])

#     # predicted_state = ekf.get_state()
#     # predicted_states.append(predicted_state)

# # Plot roll and pitch vs time
# plt.figure(figsize=(10, 6))
# plt.plot(time, roll_values, label='Roll (degrees)')
# plt.plot(time, pitch_values, label='Pitch (degrees)')
# plt.xlabel('Time')
# plt.ylabel('Angle (degrees)')
# plt.title('Roll and Pitch vs Time')
# plt.legend()
# plt.grid(True)
# plt.show()

