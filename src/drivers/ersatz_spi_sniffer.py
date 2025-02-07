import pigpio
import time

MOSI_PIN = 10
MISO_PIN = 9  
SCLK_PIN = 11

pi = pigpio.pi()
if not pi.connected:
    print("Failed to connect to pigpio daemon.")
    exit(1)

# Set up pins
for pin in (MOSI_PIN, MISO_PIN, SCLK_PIN):
    pi.set_mode(pin, pigpio.INPUT)

# Track previous SCLK state
last_sclk = 0
byte_buffer = []
bit_count = 0

try:
    while True:
        sclk = pi.read(SCLK_PIN)
        
        # Detect rising edge
        if sclk == 1 and last_sclk == 0:
            mosi = pi.read(MOSI_PIN)
            miso = pi.read(MISO_PIN)
            byte_buffer.append((mosi, miso))
            bit_count += 1
            
            # Print complete bytes
            if bit_count == 8:
                mosi_byte = int(''.join(str(b[0]) for b in byte_buffer), 2)
                miso_byte = int(''.join(str(b[1]) for b in byte_buffer), 2)
                print(f"MOSI: 0x{mosi_byte:02X}, MISO: 0x{miso_byte:02X}")
                byte_buffer = []
                bit_count = 0
                
        last_sclk = sclk
        
except KeyboardInterrupt:
    print("\nStopping SPI sniffer...")
finally:
    pi.stop()