#!/usr/bin/env python3

import mmap
import struct
import os
import signal
import time
import datetime
import ctypes
from ctypes import Structure, c_int, c_float, c_double, Array, POINTER
from drone_ekf_actual import EKF_Drone
from low_pass_filter_ema import LowPassFilterEMA
import numpy as np
import kinematic_sim as kin
from multiprocessing import shared_memory, Process, Value, Array, resource_tracker

# Flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    global running
    print("Python logger: Shutting down...")
    running = False

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# Define structures for sensor data matching the C structures
class BMP180Data(Structure):
    _fields_ = [
        ("pressure", c_float),    # in hPa
        ("altitude", c_float)     # in meters (calculated)
    ]

class GPSData(Structure):
    _fields_ = [
        ("latitude", c_double),
        ("longitude", c_double),
        ("height", c_double),
        ("utc_time", c_double),
        ("velocity_x", c_double),
        ("velocity_y", c_double),
        ("velocity_z", c_double),
        ("accel_x", c_double),
        ("accel_y", c_double),
        ("accel_z", c_double)
    ]

class MPU6050Data(Structure):
    _fields_ = [
        ("ax", c_float),
        ("ay", c_float),
        ("az", c_float),
        ("gx", c_float),
        ("gy", c_float),
        ("gz", c_float)
    ]

class SensorSuite(Structure):
    _fields_ = [
        ("baro_data", BMP180Data),
        ("gps_data", GPSData),
        ("imu_data", MPU6050Data),
        ("distance", c_float),
        ("delta_t", c_double),
        # Skip mutex in Python as we're only reading
        ("_mutex_padding", c_int * 10)  # Padding for the mutex (approximate)
    ]

BUFFER_SIZE = 100

def main():
    # Wait for the shared memory to be created by the C program
    max_retries = 30
    retries = 0
    
    # This is the path where POSIX shared memory objects appear in Linux
    shm_path = "/dev/shm/simple_counter"
    angles_file_path = "/dev/shm/angles_data_file"

    # Example usage:
    dt = 0.0225  # Time step based on logged delta_t values dt = sensor_data.delta_t - last_delta_t

    ekf = EKF_Drone(dt)

    # Create low-pass filter instances for each axis
    gyro_x_filter = LowPassFilterEMA(alpha=0.1, initial_value=0.0)
    gyro_y_filter = LowPassFilterEMA(alpha=0.1, initial_value=0.0)
    gyro_z_filter = LowPassFilterEMA(alpha=0.1, initial_value=0.0)

    roll = 0  # about y-axis in the direction of nose of the drone
    pitch = 0   # about x-axis in the direction of left side of the assembled drone
    yaw = 0

    # Initialize previous velocity values to store kf vx, vy and vz values to be used to find ax, ay and az
    prev_vx, prev_vy, prev_vz = 0.0, 0.0, 0.0

    final_state_vector = []
    
    # Complimentary filter
    alpha = 0.95  # greater the value more trust on the gyroscope     
    
    # Create a named shared memory segment for angles data to be used by transmit.py
    # This replaces the previous Array approach which only works within the same process hierarchy
    try:
        angles_file_path = "/dev/shm/angles_data_file"
        
        # Create the file if it doesn't exist and initialize it with zeros
        with open(angles_file_path, "wb") as f:
            # Initialize with zeros - 3 doubles (x, y, z) = 24 bytes
            f.write(b'\x00' * (3 * 8))

        retries = 0
        while retries < max_retries and running:
            if os.path.exists(angles_file_path):
                break
            print("Python logger: Failed to find shared memory after multiple attempts")
            retries += 1
            time.sleep(1)
            return
        
        # Open the file for memory mapping
        angles_fd = os.open(angles_file_path, os.O_RDWR)
        angles_mmap = mmap.mmap(angles_fd, 3 * 8, mmap.MAP_SHARED, mmap.PROT_WRITE | mmap.PROT_READ)
        
        print(f"Python logger: Created manual shared memory file at {angles_file_path}")
        
        # Now, when we want to update the angles, we can write directly to the mmap:
        # Function to update the angles in shared memory
        def update_angles_shared_memory(x, y, z):
            angles_mmap.seek(0)
            # Pack 3 doubles in little-endian format
            angles_mmap.write(struct.pack('ddd', x, y, z))
            angles_mmap.flush()
        
        # Use this function instead of updating angles_shared_array
        # For example, replace the line that updates angles_shared_array with:
        # update_angles_shared_memory(final_position[0], final_position[1], final_position[2])
        
    except Exception as e:
        print(f"Python logger: Error creating manual shared memory: {e}")

    # Wait for the shared memory file to exist
    retries = 0
    while retries < max_retries and running:
        if os.path.exists(shm_path):
            break
        print(f"Python logger: Waiting for shared memory to be created... ({retries+1}/{max_retries})")
        retries += 1
        time.sleep(1)
    
    try:
        # Open the shared memory file
        fd = os.open(shm_path, os.O_RDWR)
        print(f"Python logger: Successfully opened shared memory at {shm_path}")
        
        # Memory map the shared memory segment
        shared_mem_size = ctypes.sizeof(SensorSuite) * BUFFER_SIZE
        shared_mem = mmap.mmap(fd, shared_mem_size, mmap.MAP_SHARED, mmap.PROT_READ)
        
        # Create a log file
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        log_filename = f"sensor_log.csv"
        ekf_log_filename = f"ekf_state_log.csv"
        
        shared_mem.seek(0)
        sensor_data_bytes = shared_mem.read(ctypes.sizeof(SensorSuite))
                
        # Create a SensorSuite instance from the bytes
        sensor_data = SensorSuite.from_buffer_copy(sensor_data_bytes)
        init_height = sensor_data.baro_data.altitude

        with open(log_filename, 'w') as logfile, open(ekf_log_filename, 'w') as ekf_logfile:
            # Write CSV header with correctly named fields
            logfile.write("timestamp,delta_t,")
            logfile.write("pressure,altitude,")
            logfile.write("latitude,longitude,height,utc_time,")
            logfile.write("velocity_x,velocity_y,velocity_z,")
            logfile.write("gps_accel_x,gps_accel_y,gps_accel_z,")
            logfile.write("imu_ax,imu_ay,imu_az,imu_gx,imu_gy,imu_gz,")
            logfile.write("distance\n")
            
            # Write EKF state log header
            ekf_logfile.write("timestamp,")
            ekf_logfile.write("kf_vx,kf_vy,kf_vz,kf_roll,kf_pitch,")
            ekf_logfile.write("gyro_roll_degrees,gyro_pitch_degrees,")
            ekf_logfile.write("accel_roll_degrees,accel_pitch_degrees,")
            ekf_logfile.write("alpha_x,alpha_y,alpha_z,")
            ekf_logfile.write("filtered_gyro_x,filtered_gyro_y,filtered_gyro_z\n")
            
            last_delta_t = -1
            
            print(f"Python logger: Logging sensor values to {log_filename}")
            print(f"Python logger: Logging EKF state values to {ekf_log_filename}")
            
            while running:
                # Read just the first element for now
                shared_mem.seek(0)
                sensor_data_bytes = shared_mem.read(ctypes.sizeof(SensorSuite))
                
                # Create a SensorSuite instance from the bytes
                sensor_data = SensorSuite.from_buffer_copy(sensor_data_bytes)
                
                # Only log if the data has changed (using delta_t as indicator)
                if sensor_data.delta_t != last_delta_t:
                    # Get current timestamp for the log
                    timestamp = datetime.datetime.now().isoformat()

                    # roll, pitch, yaw = ekf.calculate_roll_pitch_complimentary(sensor_data.imu_data.ax, sensor_data.imu_data.ay, sensor_data.imu_data.az, sensor_data.imu_data.gx, sensor_data.imu_data.gy, sensor_data.imu_data.gz, dt, alpha, roll, pitch, yaw)
                    '''
                    Get low pass filtered gyro rotational vel values
                    '''
                    gyro_x_lf = gyro_x_filter.filter(sensor_data.imu_data.gx)
                    gyro_y_lf = gyro_y_filter.filter(sensor_data.imu_data.gy)
                    gyro_z_lf = gyro_z_filter.filter(sensor_data.imu_data.gz)

                    '''
                    Getting rotational acceleration by differentiating rotational velocities of gyro
                    '''
                    gyro_alpha_x, gyro_alpha_y, gyro_alpha_z = ekf.get_rotational_acceleration(sensor_data.imu_data.gx, sensor_data.imu_data.gy, sensor_data.imu_data.gz)

                    '''
                    Get yaw from tan inverse of gps velocity in y and x direction
                    '''
                    yaw = np.arctan2(sensor_data.gps_data.velocity_y, sensor_data.gps_data.velocity_x)

                    '''
                    Get and lof gyro roll and pitch standalon for visual debugging purposes
                    '''
                    gyro_roll, gyro_pitch= ekf.get_gyro_roll_pitch(sensor_data.imu_data.gx, sensor_data.imu_data.gy)
                    gyro_roll_degrees = np.degrees(gyro_roll)
                    gyro_pitch_degrees = np.degrees(gyro_pitch)
                    '''
                    Kalman filter section
                    '''
                    # predict step
                    ekf.predict(sensor_data.imu_data.ax, sensor_data.imu_data.ay, sensor_data.imu_data.az, sensor_data.imu_data.gx, sensor_data.imu_data.gy, sensor_data.imu_data.gz)

                    # update step
                    accel_roll = np.arctan2(sensor_data.imu_data.ax, np.sqrt(sensor_data.imu_data.ay**2 + sensor_data.imu_data.az**2))
                    accel_pitch = np.arctan2(-sensor_data.imu_data.ay, np.sqrt(sensor_data.imu_data.ax**2 + sensor_data.imu_data.az**2))

                    ekf.update(sensor_data.gps_data.velocity_x, sensor_data.gps_data.velocity_y, sensor_data.gps_data.velocity_z, accel_roll, accel_pitch)
                    
                    update_state, update_covariance = ekf.get_state()

                    ## Update state single vector storing vx, vy, vz, phi (roll about y), theta (pitch about x)
                    roll = update_state[3]
                    pitch = update_state[4]
                    # print(f"Roll: {roll:.2f}, Pitch: {pitch:.2f}")

                    # Calculate accelerations by differentiating velocities
                    ax = (update_state[0] - prev_vx) / dt
                    ay = (update_state[1] - prev_vy) / dt
                    az = (update_state[2] - prev_vz) / dt

                    # Update previous velocity values
                    prev_vx = update_state[0]
                    prev_vy = update_state[1]
                    prev_vz = update_state[2]
                    
                    # final state vector order: vx, vy, vz, roll, pitch, yaw, ax, ay, ax, rotational_vel_x, rotational_vel_y, rotational_vel_z, rotational_accel_x, rotational_accel_y, rotational_accel_z
                    '''
                    vx, vy, vz, roll, pitch come from the KF
                    ax, ay, az come from differentiating the vz, vy and vz values
                    gyro_x_lf, gyro_y_lf, gyro_z_lf come from low pass filtering the gyro rotational rates
                    gyro_alpha_x, gyro_alpha_y, gyro_alpha_z come from differentiating the gyro rotational rates
                    '''
                    final_state_vector = list(update_state)
                    final_state_vector.extend([yaw, ax, ay, az, gyro_x_lf, gyro_y_lf, gyro_z_lf, gyro_alpha_x, gyro_alpha_y, gyro_alpha_z])

                    roll_degrees = np.degrees(roll)
                    pitch_degrees = np.degrees(pitch)

                    updated_height = sensor_data.baro_data.altitude-init_height

                    # NOTE remove when no simulating
                    final_state_vector[0] = final_state_vector[0] + 8
                    updated_height = 15

                    a_payload_G, v_payload_G = kin.payload_lin_a_v(v_imu_G = [final_state_vector[0], final_state_vector[1], final_state_vector[2]],
                                        roll = final_state_vector[3],
                                        pitch = final_state_vector[4],
                                        yaw = final_state_vector[5],
                                        a_imu_G = [final_state_vector[6], final_state_vector[7], final_state_vector[8]],
                                        omega_drone = [final_state_vector[9], final_state_vector[10], final_state_vector[11]],
                                        alpha_drone = [final_state_vector[12], final_state_vector[13], final_state_vector[14]],
                                        r_drone = (1/1000)*np.array([15.45, -162.19, 28.41]))

                    final_position = kin.simulate_freefall(h0 = updated_height, 
                                          a0 = [a_payload_G[0], a_payload_G[1], a_payload_G[2]], 
                                          v0 = [v_payload_G[0], v_payload_G[1], v_payload_G[2]],
                                          m = 0.1)

                    print("x:", final_position[0], "y:", final_position[1], "z:", final_position[2])
                    
                    angles_dist = kin.inverse_kinematics_jacobian(*final_position)

                    # print("yaw: ", angles_dist[0], " pitch: ", angles_dist[1], " dist: ", angles_dist[2])

                    # Update the shared memory with position or angles data (whichever is needed by transmit.py)
                    update_angles_shared_memory(final_position[0], final_position[1], final_position[2])
                    
                    # Format all data as CSV row for sensor log
                    log_row = (
                        f"{timestamp},{sensor_data.delta_t:.6f},"
                        f"{sensor_data.baro_data.pressure:.2f},{sensor_data.baro_data.altitude:.2f},"
                        f"{sensor_data.gps_data.latitude:.7f},{sensor_data.gps_data.longitude:.7f},"
                        f"{sensor_data.gps_data.height:.2f},{sensor_data.gps_data.utc_time:.6f},"
                        f"{sensor_data.gps_data.velocity_x:.4f},{sensor_data.gps_data.velocity_y:.4f},"
                        f"{sensor_data.gps_data.velocity_z:.4f},"
                        f"{sensor_data.gps_data.accel_x:.4f},{sensor_data.gps_data.accel_y:.4f},"
                        f"{sensor_data.gps_data.accel_z:.4f},"
                        f"{sensor_data.imu_data.ax:.4f},{sensor_data.imu_data.ay:.4f},"
                        f"{sensor_data.imu_data.az:.4f},{sensor_data.imu_data.gx:.4f},"
                        f"{sensor_data.imu_data.gy:.4f},{sensor_data.imu_data.gz:.4f},"
                        f"{sensor_data.distance:.4f}\n"
                    )
                    
                    logfile.write(log_row)
                    logfile.flush()  # Ensure data is written to disk
                    
                    # Format all data as CSV row for EKF state log
                    ekf_log_row = (
                        f"{timestamp},"
                        f"{update_state[0]:.4f},{update_state[1]:.4f},{update_state[2]:.4f},{roll_degrees:.4f},{pitch_degrees:.4f}," 
                        f"{gyro_roll_degrees:.4f},{gyro_pitch_degrees:.4f},"
                        f"{np.degrees(accel_roll):.4f},{np.degrees(accel_pitch):.4f},"
                        f"{gyro_alpha_x:.4f},{gyro_alpha_y:.4f},{gyro_alpha_z:.4f},"
                        f"{gyro_x_lf:.4f},{gyro_y_lf:.4f},{gyro_z_lf:.4f}\n"
                    )
                    
                    ekf_logfile.write(ekf_log_row)
                    ekf_logfile.flush()  # Ensure data is written to disk
                    
                    last_delta_t = sensor_data.delta_t
                
                # Sleep a short while to avoid busy waiting
                time.sleep(0.001)
        
    except Exception as e:
        print(f"Python logger: Error: {str(e)}")
    finally:
        # Clean up resources
        if 'shared_mem' in locals():
            shared_mem.close()
        if 'fd' in locals():
            os.close(fd)

if __name__ == "__main__":
    main()