import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# Parameters
time_steps = 100          # Number of data points
time_interval = 0.1       # Time interval between points (in seconds)
accel_noise_std = 0.1     # Standard deviation of the noise in accelerometer readings
gps_noise_std = 0.05      # Standard deviation of the noise in GPS readings

# Generate dummy accelerometer data (sinusoidal + noise)
true_acceleration_x = np.sin(np.linspace(0, 2 * np.pi, time_steps))  # True sinusoidal acceleration pattern
accel_noise = np.random.normal(0, accel_noise_std, time_steps)       # Gaussian noise
acceleration_x = true_acceleration_x + accel_noise                   # Accelerometer readings with noise

# Integrate acceleration to get the true velocity in x-direction
true_velocity_x = np.cumsum(true_acceleration_x) * time_interval

# Create GPS-corrected velocity data with additional noise
gps_noise = np.random.normal(0, gps_noise_std, time_steps)           # Smaller noise for GPS readings
gps_velocity_x = true_velocity_x + gps_noise                         # GPS-based velocity with corrective noise

# Kalman Filter Initialization
predicted_velocity = 0           # Initial velocity estimate
predicted_covariance = 1         # Initial estimate uncertainty
process_variance = accel_noise_std ** 2  # Process noise variance
measurement_variance = gps_noise_std ** 2  # Measurement noise variance

# Storage for filter results
kalman_velocity = []

for i in range(time_steps):
    # Prediction Step
    predicted_velocity += acceleration_x[i] * time_interval  # Projected velocity based on accelerometer
    predicted_covariance += process_variance                 # Increase uncertainty due to process noise

    # Update Step (with GPS correction)
    kalman_gain = predicted_covariance / (predicted_covariance + measurement_variance)
    predicted_velocity += kalman_gain * (gps_velocity_x[i] - predicted_velocity)  # Correct with GPS
    predicted_covariance *= (1 - kalman_gain)                                     # Update uncertainty

    # Save the Kalman filter velocity estimate
    kalman_velocity.append(predicted_velocity)

# Store data in DataFrame
data = pd.DataFrame({
    'Time (s)': np.arange(0, time_steps * time_interval, time_interval),
    'Accelerometer Acceleration X (m/s^2)': acceleration_x,
    'True Velocity X (m/s)': true_velocity_x,
    'GPS Corrected Velocity X (m/s)': gps_velocity_x,
    'Kalman Filter Velocity X (m/s)': kalman_velocity
})

# Plot the results
plt.figure(figsize=(12, 8))

# Plot accelerometer acceleration data
plt.subplot(3, 1, 1)
plt.plot(data['Time (s)'], data['Accelerometer Acceleration X (m/s^2)'], label='Accelerometer Acceleration X')
plt.xlabel('Time (s)')
plt.ylabel('Acceleration (m/s^2)')
plt.legend()
plt.title('Dummy Drone Accelerometer Acceleration Data with Noise')

# Plot true velocity and GPS corrected velocity data
plt.subplot(3, 1, 2)
plt.plot(data['Time (s)'], data['True Velocity X (m/s)'], label='True Velocity X')
plt.plot(data['Time (s)'], data['GPS Corrected Velocity X (m/s)'], label='GPS Corrected Velocity X', linestyle='dashed')
plt.xlabel('Time (s)')
plt.ylabel('Velocity (m/s)')
plt.legend()
plt.title('GPS Corrected Velocity Data')

# Plot Kalman Filtered Velocity vs True and GPS Corrected Velocity
plt.subplot(3, 1, 3)
plt.plot(data['Time (s)'], data['Kalman Filter Velocity X (m/s)'], label='Kalman Filter Velocity X', linestyle='dotted')
plt.xlabel('Time (s)')
plt.ylabel('Velocity (m/s)')
plt.legend()
plt.title('Kalman Filter Velocity Estimate')

plt.tight_layout()
plt.show()
