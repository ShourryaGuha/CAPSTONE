import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.spatial.transform import Rotation as R

# Load data from CSV file
# data = pd.read_csv("/home/shourryag/Documents/Shourrya_Capstone/CAPSTONE/Running_12th/sensor_log_W.csv")

# windows
data = pd.read_csv("c:/Users/guhas/src/CAPSTONE/Running_12th/sensor_log_W.csv")
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
    def __init__(self, dt):

        """
        For rotational acceleration instance variables that will be used to store prev_gyro rotational velocity
        To calculate delta_gyro = gyro - prev_gyro
        To calculate rotational acceleration = delta_gyro / dt
        """
        self.prev_gyro_x = 0
        self.prev_gyro_y = 0
        self.prev_gyro_z = 0

        '''
        For roll, pitch, yaw calculation using just gyroscope for debugging EKF
        '''
        self.prev_gyro_roll = 0
        self.prev_gyro_pitch = 0
        self.prev_gyro_yaw = 0

        '''
        Old gps velocities to be remembered since we are getting a bunch of zeroes in between. If we read a zero, we just use old gps velocities
        '''
        self.prev_gps_velocity_x = 0
        self.prev_gps_velocity_y = 0
        self.prev_gps_velocity_z = 0

        """
        Extended Kalman Filter for velocity estimation using accelerometer and GPS.

        Parameters:
        dt: float
            Time step (seconds)
        process_noise: float
            Process noise variance (for acceleration)
        measurement_noise: float
            Measurement noise variance (for GPS velocity)
        """
        self.dt = dt  # Time step
        
        # State vector [vx, vy, vz, phi (roll about y), theta (pitch about x)]
        self.x = np.zeros((5, 1))  # Initial state (velocity, roll, pitch)

        # State covariance matrix (initial uncertainty)
        self.P = np.diag([10, 10, 10, 0.01, 0.01])  # Larger uncertainty for roll and pitch

        # State transition matrix (F)
        self.F = np.eye(5)
        self.F[0, 3] = self.dt  # Link between roll and velocity in x
        self.F[1, 4] = self.dt  # Link between pitch and velocity in y

        # Control input matrix (B)
        self.B = np.zeros((5, 5))
        self.B[0, 0] = self.dt
        self.B[1, 1] = self.dt
        self.B[2, 2] = self.dt
        self.B[3, 3] = self.dt
        self.B[4, 4] = self.dt

        # Measurement matrix (H)
        self.H = np.eye(5)

        # Process noise covariance (Q)
        self.Q = np.diag([0.01, 0.01, 0.01, 0.0001, 0.0001])  # velocity in m²/s², angles in rad²

        # Measurement noise covariance (R)
        self.R =  np.diag([1.0, 1.0, 1.0, 0.0012, 0.0012])  # Velocity in m²/s², angles in rad²

    def predict(self, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z):
        # Control input (acceleration and angular velocity)
        # IMPORTANT gyro_x corresponds to pitch and gyro_y corresponds to roll
        # and state vector is [vx, vy, vz, roll, pitch]

        u = np.array([accel_x, accel_y, accel_z, gyro_y, gyro_x]).reshape(5, 1)

        # Predict state (velocity and angles)
        self.x = self.F @ self.x + self.B @ u

        # Normalize angles after update
        self.x[3, 0] = (self.x[3, 0] + np.pi) % (2 * np.pi) - np.pi  # Roll
        self.x[4, 0] = (self.x[4, 0] + np.pi) % (2 * np.pi) - np.pi  # Pitch

        # Predict covariance
        self.P = self.F @ self.P @ self.F.T + self.Q

    def update(self, gps_velocity_x, gps_velocity_y, gps_velocity_z, accel_roll, accel_pitch):
        if gps_velocity_x != 0 or gps_velocity_y != 0 or gps_velocity_z != 0:
            self.prev_gps_velocity_x = gps_velocity_x
            self.prev_gps_velocity_y = gps_velocity_y
            self.prev_gps_velocity_z = gps_velocity_z
        # Measurement vector (GPS velocity and accelerometer angles)
        z = np.array([self.prev_gps_velocity_x, self.prev_gps_velocity_y, self.prev_gps_velocity_z, accel_roll, accel_pitch]).reshape(5, 1)

        # Kalman Gain
        S = self.H @ self.P @ self.H.T + self.R
        K = self.P @ self.H.T @ np.linalg.inv(S)

        # Update state estimate
        self.x = self.x + K @ (z - self.H @ self.x)

        # Update covariance
        I = np.eye(5)
        self.P = (I - K @ self.H) @ self.P

    def get_state(self):
        """ Returns the estimated velocity and angles """
        return self.x.flatten(), self.P

    def get_rotational_acceleration(self, gyro_x, gyro_y, gyro_z):
        alpha_gyro_x = (gyro_x - self.prev_gyro_x) / self.dt
        alpha_gyro_y = (gyro_y - self.prev_gyro_y) / self.dt
        alpha_gyro_z = (gyro_z - self.prev_gyro_z) / self.dt

        self.prev_gyro_x = gyro_x
        self.prev_gyro_y = gyro_y
        self.prev_gyro_z = gyro_z

        return alpha_gyro_x, alpha_gyro_y, alpha_gyro_z

    def get_gyro_roll_pitch(self, gyro_x, gyro_y):
        gyro_roll = self.prev_gyro_roll + gyro_y * self.dt
        gyro_pitch = self.prev_gyro_pitch + gyro_x * self.dt

        self.prev_gyro_roll = gyro_roll
        self.prev_gyro_pitch = gyro_pitch

        return gyro_roll, gyro_pitch
        
    
    def calculate_roll_pitch_complimentary(self, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, dt, alpha, prev_roll, prev_pitch, prev_yaw):
        
        # print(f"Accel X: {accel_x:.2f}, Accel Y: {accel_y:.2f}, Accel Z: {accel_z:.2f}")
        # print(f"Gyro X: {gyro_x:.2f}, Gyro Y: {gyro_y:.2f}, Gyro Z: {gyro_z:.2f}")

        accel_roll = np.arctan2(-accel_x, np.sqrt(accel_y**2 + accel_z**2))  # Roll (about Y-axis)
        accel_pitch = np.arctan2(accel_y, np.sqrt(accel_x**2 + accel_z**2))  # Pitch (about X-axis)

        yaw_degrees = 0 # Placeholder for yaw calculation
        yaw = 0 # Placeholder for yaw calculation
        
        # print(f"Roll: {accel_roll:.2f} radians, Pitch: {accel_pitch:.2f} radians")
        # print(f"Roll: {np.degrees(accel_roll):.2f} degrees, Pitch: {np.degrees(accel_pitch):.2f}, Yaw: {yaw_degrees:.2f} degrees")
       
        if prev_roll == 0 or prev_pitch == 0:
            roll, pitch = accel_roll, accel_pitch
        else:
            # Gyroscope integration for smooth roll & pitch updates

            gyro_roll = prev_roll + gyro_y * dt
            gyro_pitch = prev_pitch + gyro_x * dt

            # print(f"Gyro Roll: {gyro_roll:.2f} radians, Gyro Pitch: {gyro_pitch:.2f} radians")
            # print(f"Gyro Roll: {np.degrees(gyro_roll):.2f} degrees, Gyro Pitch: {np.degrees(gyro_pitch):.2f}")

            roll = alpha * gyro_roll + (1 - alpha) * accel_roll
            pitch = alpha * gyro_pitch + (1 - alpha) * accel_pitch
            roll_degrees = np.degrees(roll)
            pitch_degrees = np.degrees(pitch)
            print(f"Roll: {roll_degrees:.2f} degrees, Pitch: {pitch_degrees:.2f} degrees , delta time: {dt}")

        # # Gyroscope-based Yaw calculation with complementary filter
        # # Integrate gyroscope yaw (gyro_z is in radians per second)
        # if prev_yaw == 0:
        #     yaw = gyro_z * dt  # Assuming the initial yaw is 0
        # else:
        #     # Integrating gyroscope yaw and applying complementary filter
        #     gyro_yaw = prev_yaw + gyro_z * dt
        #     yaw = alpha * gyro_yaw + (1 - alpha) * prev_yaw  # Since accelerometer doesn't give yaw, we use the previous yaw value

        # # Print the result for yaw in degrees
        # yaw_degrees = np.degrees(yaw)

        
        

        return roll, pitch, yaw 

# Example usage:
dt = 0.025  # Time step (100ms)
process_noise = 0.1  # Process noise variance (acceleration noise)
measurement_noise = 1.0  # Measurement noise variance (GPS velocity noise)

ekf = EKF_Drone(dt)

# Loop over the data (for each time step)
predicted_states = []

roll = 0  # about y-axis in the direction of nose of the drone
pitch = 0   # about x-axis in the direction of left side of the assembled drone

# Lists to store roll, pitch, and time values
roll_values = []
pitch_values = []
# time_values = []

for i in range(len(imu_ax_filtered)):

    # assuming correct coordinate transfomr from IMU to world frame from chatgpt

    # R_mat = np.array([
    #     [np.cos(pitch), 0, -np.sin(pitch)],
    #     [np.sin(roll) * np.sin(pitch), np.cos(roll), np.sin(roll) * np.cos(pitch)],
    #     [np.cos(roll) * np.sin(pitch), -np.sin(roll), np.cos(roll) * np.cos(pitch)]
    # ])

    # # Convert acceleration to world frame & remove gravity
    # accel_world = R_mat @ np.array([imu_ax_filtered[i], imu_ay_filtered[i], imu_az_filtered[i]])

    # # might need to multiple g = np.array([0, 0, -9.81]) with R_mat, check math and chatgpt
    # accel_linear = accel_world - np.array([0, 0, -9.81])

#     # Access x, y, and z values of accel_linear
#     accel_linear_x = accel_linear[0]
#     accel_linear_y = accel_linear[1]
#     accel_linear_z = accel_linear[2]

    ekf.predict(imu_ax_filtered[i], imu_ay_filtered[i], imu_az_filtered[i], imu_gyro_x[i], imu_gyro_y[i], imu_gyro_z[i])

    # update step
    accel_roll = np.arctan2(imu_ax_filtered[i], np.sqrt(imu_ay_filtered[i]**2 + imu_az_filtered[i]**2))
    accel_pitch = np.arctan2(-imu_ay_filtered[i], np.sqrt(imu_ax_filtered[i]**2 + imu_az_filtered[i]**2))

    ekf.update(velocity_x[i], velocity_y[i], velocity_z[i], accel_roll, accel_pitch)

    update_state, covariance = ekf.get_state()
    # convert to degrees
    update_state[3] = np.degrees(update_state[3])
    update_state[4] = np.degrees(update_state[4])

    predicted_states.append(update_state)


predicted_states = np.array(predicted_states)

roll = predicted_states[:, 3]
pitch = predicted_states[:, 4]

# Limit the data to the first 100 points
limit = 100
time_limited = time[:limit]
roll_limited = roll[:limit]
pitch_limited = pitch[:limit]

# Plot roll and pitch vs time
plt.figure(figsize=(10, 6))
plt.plot(time_limited, roll_limited, label='Roll (degrees)')
plt.plot(time_limited, pitch_limited, label='Pitch (degrees)')
plt.xlabel('Time')
plt.ylabel('Angle (degrees)')
plt.title('Roll and Pitch vs Time')
plt.legend()
plt.grid(True)
plt.show()

