#!/usr/bin/env python3

import os
import sys
import multiprocessing
import subprocess
import signal
import time

# Configure paths
C_PROGRAM_PATH = "build/main"
PYTHON_LOGGER_PATH = "src/drivers/data_logger.py"

# Flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    global running
    print("\nShutting down all processes...")
    running = False

# Register signal handlers
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

def run_c_program(core_id):
    """Function to run the C program on a specific core"""
    pid = os.getpid()
    
    # Bind process to specified CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"C Program running on Core {core_id}, PID: {pid}")
    
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
                
        print(f"C Program on Core {core_id} terminated")
        raise RuntimeError(f"C Program on Core {core_id} terminated unexpectedly.")
        
    except Exception as e:
        print(f"Error running C program: {e}")
    
def run_python_logger(core_id):
    """Function to run the Python logger on a specific core"""
    pid = os.getpid()
    
    # Bind process to specified CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"Python Logger running on Core {core_id}, PID: {pid}")
    
    # Wait for the C program to start and set up shared memory
    time.sleep(1)
    
    # Import and run the logger script
    try:
        sys.path.append(os.path.dirname(PYTHON_LOGGER_PATH))
        logger_name = os.path.basename(PYTHON_LOGGER_PATH).replace('.py', '')
        
        # First try to run as a module
        try:
            logger_module = __import__(logger_name)
            if hasattr(logger_module, 'main'):
                print(f"Running logger as a module")
                
                # Set up a flag for the logger to check
                global running
                logger_module.running = running
                
                # Run the logger
                logger_module.main()
            else:
                raise ImportError("No main function in module")
                
        except ImportError:
            # If that fails, run it as a subprocess
            print(f"Running logger as a subprocess")
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
                    
        print(f"Python Logger on Core {core_id} terminated")
        raise RuntimeError(f"Python Logger on Core {core_id} terminated unexpectedly.")
        
    except Exception as e:
        print(f"Error running Python logger: {e}")

def main():
    # Check if the executables exist
    if not os.path.exists(C_PROGRAM_PATH):
        print(f"Error: C program not found at {C_PROGRAM_PATH}")
        return
        
    if not os.path.exists(PYTHON_LOGGER_PATH):
        print(f"Error: Python logger not found at {PYTHON_LOGGER_PATH}")
        return
    
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
        print("Warning: Only one CPU core available. Python logger will run on core 0 as well.")
        logger_process = multiprocessing.Process(target=run_python_logger, args=(0,))
        logger_process.start()
        processes.append(logger_process)
    
    # Wait for processes to complete
    try:
        for p in processes:
            p.join()
    except RuntimeError as e:
        print(f"Error detected: {e}")
        global running
        running = False  # Stop running flag
        print("Shutting down all processes...")

        # Terminate any running processes
        for p in processes:
            p.terminate()
        
        for p in processes:
            p.join()
    except KeyboardInterrupt:
        # This should be caught by the signal handler, but just in case
        running = False
        print("Waiting for processes to terminate...")
        time.sleep(2)  # Give processes time to clean up
    
    print("All processes have terminated")

if __name__ == "__main__":
    main()