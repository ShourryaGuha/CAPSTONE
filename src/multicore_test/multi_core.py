import os
import multiprocessing
import ctypes

# Load compiled C driver
c_driver = ctypes.CDLL("./driver.so")
c_driver.run_task.argtypes = [ctypes.c_int]  # Define function signature

def worker(core_id):
    pid = os.getpid()
    
    # Bind process to a specific CPU core
    os.sched_setaffinity(pid, {core_id})
    
    print(f"Python Process {pid} assigned to Core {core_id}")

    # Infinite loop printing messages
    while True:
        print(f"Python Task running on core {core_id}, PID: {pid}")
        c_driver.run_task(core_id)  # Call C function (which also loops)
        
if __name__ == "__main__":
    processes = []
    
    for i in range(multiprocessing.cpu_count()):
        p = multiprocessing.Process(target=worker, args=(i,))
        p.start()
        processes.append(p)

    for p in processes:
        p.join()
