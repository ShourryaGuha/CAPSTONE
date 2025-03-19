#!/usr/bin/env python3

import os
import sys
import multiprocessing
import subprocess
import signal
import time
from multiprocessing import Process

# Configure paths - adjust these to match your actual executable paths
# C_PROGRAM_PATH = "build/main"
# PYTHON_LOGGER_PATH = "src/drivers/data_logger.py"
# PYTHON_TRANSMIT_PATH = "src/drivers/transmit.py"  # Add path to transmit.py

C_PROGRAM_PATH = "../../build/main"
PYTHON_LOGGER_PATH = "data_logger.py"
PYTHON_TRANSMIT_PATH = "transmit.py"  # Add path to transmit.py

# Flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    global running
    print("\nSupervisor: Shutting down all processes...")
    running = False

# Register signal handlers
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

def run_c_program(core_id):
    """Function to run the C program on a specific core"""
    pid = os.getpid()
    
    # Bind process to specified CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"Supervisor: C Program running on Core {core_id}, PID: {pid}")
    
    # Start the C program as a subprocess
    try:
        process = subprocess.Popen([C_PROGRAM_PATH])
        
        # Wait for the process to complete or for shutdown signal
        while process.poll() is None and running:
            time.sleep(0.1)
            
        # If we're here because of shutdown signal, terminate the process
        if running == False and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                
        print(f"Supervisor: C Program on Core {core_id} terminated")
        
    except Exception as e:
        print(f"Supervisor: Error running C program: {e}")
    
def run_python_logger(core_id):
    """Function to run the Python logger on a specific core"""
    pid = os.getpid()
    
    # Bind process to specified CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"Supervisor: Python Logger running on Core {core_id}, PID: {pid}")
    
    # Wait for the C program to start and set up shared memory
    time.sleep(2)
    
    # Start the Python logger as a subprocess
    try:
        process = subprocess.Popen([sys.executable, PYTHON_LOGGER_PATH])
        
        # Wait for the process to complete or for shutdown signal
        while process.poll() is None and running:
            time.sleep(0.1)
            
        # If we're here because of shutdown signal, terminate the process
        if running == False and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                
        print(f"Supervisor: Python Logger on Core {core_id} terminated")
        
    except Exception as e:
        print(f"Supervisor: Error running Python logger: {e}")

def run_python_transmitter(core_id):
    """Function to run the Python transmitter on a specific core"""
    pid = os.getpid()
    
    # Bind process to specified CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"Supervisor: Python Transmitter running on Core {core_id}, PID: {pid}")
    
    # Wait a bit for the logger to start
    time.sleep(3)
    
    # Start the Python transmitter as a subprocess
    try:
        process = subprocess.Popen([sys.executable, PYTHON_TRANSMIT_PATH])
        
        # Wait for the process to complete or for shutdown signal
        while process.poll() is None and running:
            time.sleep(0.1)
            
        # If we're here because of shutdown signal, terminate the process
        if running == False and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                
        print(f"Supervisor: Python Transmitter on Core {core_id} terminated")
        
    except Exception as e:
        print(f"Supervisor: Error running Python transmitter: {e}")

def main():
    # Create processes
    processes = []
    
    # Run C program on core 0
    c_process = multiprocessing.Process(target=run_c_program, args=(0,))
    c_process.start()
    processes.append(c_process)
    
    # Determine available cores
    available_cores = multiprocessing.cpu_count()
    
    # Run Python logger on core 1 (or core 0 if only one core available)
    logger_core = 1 if available_cores > 1 else 0
    logger_process = multiprocessing.Process(target=run_python_logger, args=(logger_core,))
    logger_process.start()
    processes.append(logger_process)
    
    # Run Python transmitter on core 2 (or another available core)
    transmitter_core = 2 if available_cores > 2 else logger_core
    transmitter_process = multiprocessing.Process(target=run_python_transmitter, args=(transmitter_core,))
    transmitter_process.start()
    processes.append(transmitter_process)
    
    # Wait for processes to complete
    try:
        for p in processes:
            p.join()
    except KeyboardInterrupt:
        # This should be caught by the signal handler, but just in case
        global running
        running = False
        
        # Terminate any running processes
        for p in processes:
            if p.is_alive():
                p.terminate()
        
        # Give processes time to clean up
        time.sleep(2)
    
    print("Supervisor: All processes have terminated")

if __name__ == "__main__":
    main()