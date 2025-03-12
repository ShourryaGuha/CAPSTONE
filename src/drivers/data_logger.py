#!/usr/bin/env python3

import mmap
import struct
import os
import signal
import time
import datetime
import ctypes
from ctypes import Structure, c_int, c_float, c_double, Array, POINTER
from drone_ekf_copy import EKF_Drone
import numpy as np

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

    # Example usage:
    dt = 0.0225  # Time step based on logged delta_t values dt = sensor_data.delta_t - last_delta_t
    process_noise = 0.1  # Process noise variance (acceleration noise)
    measurement_noise = 1.0  # Measurement noise variance (GPS velocity noise)

    ekf = EKF_Drone(dt, process_noise, measurement_noise)

    roll = 0  # about y-axis in the direction of nose of the drone
    pitch = 0   # about x-axis in the direction of left side of the assembled drone
    yaw = 0

    # Complimentary filter
    alpha = 0.95  # greater the value more trust on the gyroscope     
    
    # Wait for the shared memory file to exist
    while retries < max_retries and running:
        if os.path.exists(shm_path):
            break
        print(f"Python logger: Waiting for shared memory to be created... ({retries+1}/{max_retries})")
        retries += 1
        time.sleep(1)
    
    if retries >= max_retries:
        print("Python logger: Failed to find shared memory after multiple attempts")
        return
    
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
        
        with open(log_filename, 'w') as logfile:
            # Write CSV header with correctly named fields
            logfile.write("timestamp,delta_t,")
            logfile.write("pressure,altitude,")
            logfile.write("latitude,longitude,height,utc_time,")
            logfile.write("velocity_x,velocity_y,velocity_z,")
            logfile.write("gps_accel_x,gps_accel_y,gps_accel_z,")
            logfile.write("imu_ax,imu_ay,imu_az,imu_gx,imu_gy,imu_gz,")
            logfile.write("distance\n")
            
            last_delta_t = -1
            
            print(f"Python logger: Logging sensor values to {log_filename}")
            
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
                    # dt = sensor_data.delta_t - last_delta_t

                    # roll, pitch, yaw = ekf.calculate_roll_pitch_complimentary(sensor_data.imu_data.ax, sensor_data.imu_data.ay, sensor_data.imu_data.az, sensor_data.imu_data.gx, sensor_data.imu_data.gy, sensor_data.imu_data.gz, dt, alpha, roll, pitch, yaw)


                    # predict step
                    ekf.predict(sensor_data.imu_data.ax, sensor_data.imu_data.ay, sensor_data.imu_data.az, sensor_data.imu_data.gx, sensor_data.imu_data.gy, sensor_data.imu_data.gz)

                    # update step
                    accel_roll = np.arctan2(-sensor_data.imu_data.ax, np.sqrt(sensor_data.imu_data.ay**2 + sensor_data.imu_data.az**2))
                    accel_pitch = np.arctan2(sensor_data.imu_data.ay, np.sqrt(sensor_data.imu_data.ax**2 + sensor_data.imu_data.az**2))

                    ekf.update(sensor_data.gps_data.velocity_x, sensor_data.gps_data.velocity_y, sensor_data.gps_data.velocity_z, accel_roll, accel_pitch)
                    
                    update_state, update_covariance = ekf.get_state()

                    roll = update_state[3]
                    pitch = update_state[4]
                    # print(f"Roll: {roll:.2f}, Pitch: {pitch:.2f}")

                    roll_degrees = np.degrees(roll)
                    pitch_degrees = np.degrees(pitch)

                    print(f"Roll: {roll:.2f} radians, Roll: {roll_degrees:.2f} degrees, Pitch: {pitch:.2f} radians, Pitch: {pitch_degrees:.2f} degrees")
                    # Format all data as CSV row
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
                    
                    # Convert GPS UTC time to human-readable format (if it's in seconds since epoch)
                    gps_time_str = "N/A"
                    if sensor_data.gps_data.utc_time > 0:
                        try:
                            gps_time_str = datetime.datetime.fromtimestamp(
                                sensor_data.gps_data.utc_time
                            ).strftime('%H:%M:%S')
                        except:
                            gps_time_str = f"{sensor_data.gps_data.utc_time:.1f}"
                    
                    # Print summary to console
                    # print(f"Logger: t={sensor_data.delta_t:.2f}s, "
                    #       f"Baro: {sensor_data.baro_data.pressure:.1f}hPa/{sensor_data.baro_data.altitude:.1f}m, "
                    #       f"GPS: ({sensor_data.gps_data.latitude:.5f}, {sensor_data.gps_data.longitude:.5f}, "
                    #       f"{sensor_data.gps_data.height:.1f}m), "
                    #       f"Distance: {sensor_data.distance:.2f}m")
                    
                    last_delta_t = sensor_data.delta_t
                
                # Sleep a short while to avoid busy waiting
                # time.sleep(0.001)
        
        # Clean up
        shared_mem.close()
        os.close(fd)
        print(f"Python logger: Shutdown complete. Data saved to {log_filename}")
        
    except Exception as e:
        print(f"Python logger: Error: {str(e)}")

if __name__ == "__main__":
    main()