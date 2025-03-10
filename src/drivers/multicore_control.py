#!/usr/bin/env python3

import os
import sys
import multiprocessing
import subprocess
import signal
import time

# # Configure paths - adjust these to match your actual executable paths
C_PROGRAM_PATH = "build/main"
PYTHON_LOGGER_PATH = "src/drivers/data_logger.py"

# C_PROGRAM_PATH = "../../build/main"
# PYTHON_LOGGER_PATH = "data_logger.py"

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

def main():
    # Create processes
    processes = []
    
    # Run C program on core 0
    c_process = multiprocessing.Process(target=run_c_program, args=(0,))
    c_process.start()
    processes.append(c_process)
    
    # Run Python logger on core 1
    if multiprocessing.cpu_count() > 1:
        logger_process = multiprocessing.Process(target=run_python_logger, args=(1,))
        logger_process.start()
        processes.append(logger_process)
    else:
        print("Supervisor: Warning: Only one CPU core available. Python logger will run on core 0 as well.")
        logger_process = multiprocessing.Process(target=run_python_logger, args=(0,))
        logger_process.start()
        processes.append(logger_process)
    
    # Wait for processes to complete
    try:
        for p in processes:
            p.join()
    except KeyboardInterrupt:
        # This should be caught by the signal handler, but just in case
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