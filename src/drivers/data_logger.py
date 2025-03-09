#!/usr/bin/env python3

import mmap
import struct
import ctypes
import csv
import time
import os
import signal
import datetime
from ctypes import Structure, c_double, c_float, c_int, c_long, Array, sizeof

# Flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    global running
    print("Shutting down logger...")
    running = False

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# Define structures to match the C code
class Timespec(Structure):
    _fields_ = [
        ("tv_sec", c_long),
        ("tv_nsec", c_long)
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

class BMP180Data(Structure):
    _fields_ = [
        ("pressure", c_float),
        ("altitude", c_float)
    ]

class Measurement(Structure):
    _fields_ = [
        ("distance", c_float),
        ("timestamp", Timespec)
    ]

class SensorPacket(Structure):
    _fields_ = [
        ("timestamp", Timespec),
        ("imu_data", MPU6050Data),
        ("gps_data", GPSData),
        ("baro_data", BMP180Data),
        ("laser_data", Measurement),
        ("sensors_present", c_int)
    ]

# sensor buffer size: 16112
BUFFER_SIZE = 100
SENSOR_BUFFER_SIZE = 16112

class SensorBuffer(Structure):
    _fields_ = [
        ("buffer", SensorPacket * BUFFER_SIZE),
        ("write_index", c_int),
        ("read_index", c_int),
        ("count", c_int),
        # Note: Mutex and condition variables can't be accessed from Python
        # We'll use the count and indices for synchronization in a simple way
        ("_mutex_padding", c_long * 10),
        ("_cond_padding", c_long * 10)
    ]

def timespec_to_datetime(ts):
    return datetime.datetime.fromtimestamp(ts.tv_sec + ts.tv_nsec / 1e9)

def main():
    # Wait for the shared memory to be created by the C program
    max_retries = 30
    retries = 0
    
    # This is the path where POSIX shared memory objects appear in Linux
    shm_path = "/dev/shm/sensor_buffer"
    
    # Wait for the shared memory file to exist
    while retries < max_retries and running:
        if os.path.exists(shm_path):
            break
        print(f"Waiting for shared memory to be created... ({retries+1}/{max_retries})")
        retries += 1
        time.sleep(1)  # Wait 1 second before retrying
    
    if retries >= max_retries:
        print("Failed to find shared memory after multiple attempts")
        return
    
    try:
        # Open the shared memory file directly
        fd = os.open(shm_path, os.O_RDWR)
        print(f"Successfully opened shared memory at {shm_path}")
        
        # Memory map the shared memory segment
        shared_mem = mmap.mmap(fd, SENSOR_BUFFER_SIZE, mmap.MAP_SHARED, mmap.PROT_READ)
        
        # Create a CSV file for logging
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        csv_filename = f"sensor_data.csv"
        
        with open(csv_filename, 'w', newline='') as csvfile:
            # Create the CSV writer with all possible fields
            fieldnames = [
                'timestamp', 
                # IMU fields
                'imu_ax', 'imu_ay', 'imu_az', 'imu_gx', 'imu_gy', 'imu_gz',
                # GPS fields 
                'gps_lat', 'gps_lon', 'gps_height', 'gps_utc', 
                'gps_vel_x', 'gps_vel_y', 'gps_vel_z',
                'gps_accel_x', 'gps_accel_y', 'gps_accel_z',
                # Barometer fields
                'baro_pressure', 'baro_altitude',
                # Laser TOF fields
                'laser_distance', 'laser_timestamp'
            ]
            
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            
            last_read_index = 0
            
            print(f"Logging data to {csv_filename}")
            
            while running:
                # Read the buffer's metadata
                shared_mem.seek(0)
                
                # Extract write_index, read_index, and count
                buffer_info = struct.unpack('iii', shared_mem.read(12))
                write_index = buffer_info[0]
                count = buffer_info[2]
                
                if count > 0:
                    # There's data to read
                    while last_read_index != write_index and running:
                        # Calculate the offset to the sensor packet
                        packet_offset = 40 + (last_read_index * sizeof(SensorPacket))  # 40 bytes for buffer metadata
                        shared_mem.seek(packet_offset)
                        
                        # Read the sensor packet
                        packet_data = shared_mem.read(sizeof(SensorPacket))
                        
                        # Create a sensor packet from the raw data
                        packet = SensorPacket.from_buffer_copy(packet_data)
                        
                        # Create the row data
                        row = {'timestamp': timespec_to_datetime(packet.timestamp).isoformat()}
                        
                        # Check which sensors are present and add their data
                        if packet.sensors_present & 0x01:  # IMU_PRESENT
                            row.update({
                                'imu_ax': packet.imu_data.ax,
                                'imu_ay': packet.imu_data.ay,
                                'imu_az': packet.imu_data.az,
                                'imu_gx': packet.imu_data.gx,
                                'imu_gy': packet.imu_data.gy,
                                'imu_gz': packet.imu_data.gz
                            })
                        
                        if packet.sensors_present & 0x02:  # GPS_PRESENT
                            row.update({
                                'gps_lat': packet.gps_data.latitude,
                                'gps_lon': packet.gps_data.longitude,
                                'gps_height': packet.gps_data.height,
                                'gps_utc': packet.gps_data.utc_time,
                                'gps_vel_x': packet.gps_data.velocity_x,
                                'gps_vel_y': packet.gps_data.velocity_y,
                                'gps_vel_z': packet.gps_data.velocity_z,
                                'gps_accel_x': packet.gps_data.accel_x,
                                'gps_accel_y': packet.gps_data.accel_y,
                                'gps_accel_z': packet.gps_data.accel_z
                            })
                        
                        if packet.sensors_present & 0x04:  # BARO_PRESENT
                            row.update({
                                'baro_pressure': packet.baro_data.pressure,
                                'baro_altitude': packet.baro_data.altitude
                            })
                        
                        if packet.sensors_present & 0x08:  # LASER_PRESENT
                            laser_ts = timespec_to_datetime(packet.laser_data.timestamp).isoformat()
                            row.update({
                                'laser_distance': packet.laser_data.distance,
                                'laser_timestamp': laser_ts
                            })
                        
                        # Write the row to the CSV
                        writer.writerow(row)
                        csvfile.flush()  # Ensure data is written to disk
                        
                        # Update the read index
                        last_read_index = (last_read_index + 1) % BUFFER_SIZE
                
                # Sleep a short while to avoid busy waiting
                time.sleep(0.01)  # 10ms
        
        # Clean up
        shared_mem.close()
        os.close(fd)
        print(f"Logger shutdown complete. Data saved to {csv_filename}")
        
    except Exception as e:
        print(f"Error in logger: {str(e)}")

if __name__ == "__main__":
    main()