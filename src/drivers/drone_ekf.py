### TODO: implement quarternion stuff to remove influence of gravity

import numpy as np
import pandas as pd
from scipy.spatial.transform import Rotation as R

# Load data from CSV file
data = pd.read_csv("../../Car_Ride_10th/sensor_log40ish.csv")

imu_ax = data['imu_ax']
imu_ay = data['imu_ay']
imu_az = data['imu_az']

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

# Example usage:
dt = 0.1  # Time step (100ms)
process_noise = 0.1  # Process noise variance (acceleration noise)
measurement_noise = 1.0  # Measurement noise variance (GPS velocity noise)

ekf = EKF_Drone(dt, process_noise, measurement_noise)

# Loop over the data (for each time step)
predicted_states = []

for i in range(len(imu_ax_filtered)):
    ekf.predict(imu_ax_filtered[i], imu_ay_filtered[i], imu_az_filtered[i])

    ekf.update(velocity_x, velocity_y, velocity_z)

    predicted_state = ekf.get_state()
    predicted_states.append(predicted_state)



