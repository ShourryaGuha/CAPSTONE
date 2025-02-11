# Multicore Python & C Execution on Raspberry Pi 4B

This guide explains how to:
- Run Python and C code on separate CPU cores.
- Verify which core each process is running on.
- Stop the processes safely.

---

## 🚀 **1. Setup & Compilation**

### **1.1 Compile the C Driver**
Run the following command to compile the C code after navigating to `multicore_test/` into a shared object (`.so`):
```bash
gcc -shared -o driver.so -fPIC driver.c
```

### **1.2 Run the Python Script**
Execute the Python script to launch processes on separate cores:
```bash
python3 multi_core.py
```
Each process will print messages indicating which core it is running on.

---

## 🔍 **2. Checking Process Core Affinity**

### **2.1 Check Running Python Processes**
```bash
ps aux | grep python
```
This lists all running Python processes along with their **PIDs**.

### **2.2 Check CPU Affinity of a Specific Process**
Find the **PID** from `ps aux`, then run:
```bash
taskset -p <PID>
```
Example output:
```
pid 6878's current affinity mask: 8
```

### **2.3 Monitor CPU Core Usage in Real-Time**
Run:
```bash
htop
```
- Press **F2** (Setup) → Enable **"Show CPU Usage per Core"**.
- Find the **PIDs** and check which cores they are using.

---

## 🛑 **3. Stopping the Processes**

### **3.1 Safe Way (Stops Only This Script)**
```bash
pkill -f multi_core.py
```
This **only kills this script**, leaving other Python processes running.

### **3.2 Kill a Specific Process (By PID)**
```bash
kill -9 <PID>
```
Replace `<PID>` with the actual process ID.

### **3.3 Emergency Stop (Kills All Python Scripts)**
```bash
killall python3
```
⚠️ **Warning:** This stops **ALL Python programs**, including ROS nodes or background services.

---

## 📊 **4. CPU Affinity Mask Table**
Each process is assigned a core using a **bitmask**:

| Core  | Binary Mask | Decimal Mask |
|-------|------------|--------------|
| 0     | `0001`     | `1`          |
| 1     | `0010`     | `2`          |
| 2     | `0100`     | `4`          |
| 3     | `1000`     | `8`          |
| 0,1   | `0011`     | `3`          |
| 0,2   | `0101`     | `5`          |
| 1,2   | `0110`     | `6`          |
| 2,3   | `1100`     | `12`         |

Example:
```
pid 6878's current affinity mask: 8  # Process is running on Core 3
```