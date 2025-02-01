# CAPSTONE 2025 - Raspberry Pi Setup and Development Guide

## Raspberry Pi SSH Access

### Login Details
- **Hostname:** `bombsite.local` (may not always work)
- **IP Address:** `192.168.91.50` (consistent)
- **Username:** `shourryag`
- **Password:** `mte482@caps`

### SSH Access via VS Code
1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open VS Code and install the [Remote Development Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack).
3. Open the command palette (`Ctrl+Shift+P`) and select **Remote-SSH: Connect to Host...**.
4. Click **Add New SSH Host...** and enter:
   ```sh
   shourryag@bombsite.local
   ```
5. If prompted, enter the password: `mte482@caps`.
6. Once connected, open a terminal in VS Code to start working.

### SSH Access via Terminal
**Prerequisite:** Ensure you are on the same network or use a VPN.

1. Open a terminal (Git Bash recommended on Windows).
2. Connect using one of the following commands:
   ```sh
   ssh shourryag@192.168.91.50
   ```
   or
   ```sh
   ssh shourryag@bombsite.local
   ```
3. Accept the SSH fingerprint by typing **yes** when prompted.
4. Enter the password when prompted.

#### Setting Up SSH Key Authentication
To avoid entering a password every time:
1. Navigate to your SSH key directory (`~/.ssh/` or equivalent).
2. Copy your SSH key to the Raspberry Pi:
   ```sh
   ssh-copy-id -i id_ed25519.pub shourryag@192.168.91.50
   ```

---

## File Transfer

### Transfer Files to Raspberry Pi (Remote Host)
```sh
scp -r CAPSTONE/ shourryag@192.168.91.50:~/Documents/FILE_TRANSFER_ONLY/
```

### Transfer Files from Raspberry Pi to Local Machine
```sh
scp -r shourryag@192.168.91.50:~/Documents/FILE_TRANSFER_ONLY/ CAPSTONE/
```

**Note:** Use the GitHub repository for version-controlled changes instead of manual file transfers.

---

## Building and Running C Files

### **Building & Running (Current Method)**
1. **Clean (optional):**
   ```sh
   make clean
   ```
   *(Only if you want to rebuild everything from scratch.)*
2. **Compile:**
   ```sh
   make
   ```
3. **Run:**
   ```sh
   sudo ./main
   ```
   *(Located in the build folder.)*

### **Deprecated Method (If using CMake)**
If modifying the `Makefile` (e.g., adding a new driver like `gps.c`), update it as follows:
```makefile
add_library(drivers gps.c)
```
Then, run:
```sh
make clean   # Only if a full rebuild is needed
cmake .      # Only if Makefile was modified
make
sudo ./main
```

---

## Notes
- Ensure you are on the same network as the Raspberry Pi.
- Port forwarding may be required for external SSH access but may not be possible on a mobile hotspot.
- Use GitHub for proper version control instead of manual file transfers where possible.

