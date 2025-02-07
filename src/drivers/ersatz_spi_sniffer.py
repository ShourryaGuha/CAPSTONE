import RPi.GPIO as GPIO
import time

# Pin definitions (using BCM numbering)
MOSI_PIN = 10
MISO_PIN = 9
SCLK_PIN = 11

# Setup GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(MOSI_PIN, GPIO.IN)
GPIO.setup(MISO_PIN, GPIO.IN)
GPIO.setup(SCLK_PIN, GPIO.IN)

# Initialize variables
last_sclk = 0
byte_buffer = []
bit_count = 0
last_debug_time = time.time()

print("Starting SPI monitoring...")
print(f"Monitoring pins - MOSI: {MOSI_PIN}, MISO: {MISO_PIN}, SCLK: {SCLK_PIN}")

try:
    while True:
        sclk = GPIO.input(SCLK_PIN)
        mosi = GPIO.input(MOSI_PIN)
        miso = GPIO.input(MISO_PIN)
        
        # Print debug info every second
        current_time = time.time()
        if current_time - last_debug_time >= 1.0:
            print(f"Current pin states - SCLK: {sclk}, MOSI: {mosi}, MISO: {miso}")
            last_debug_time = current_time
        
        # Detect rising edge
        if sclk == 1 and last_sclk == 0:
            print(f"Rising edge detected! MOSI: {mosi}, MISO: {miso}")
            byte_buffer.append((mosi, miso))
            bit_count += 1
            
            # Print complete bytes
            if bit_count == 8:
                mosi_byte = int(''.join(str(b[0]) for b in byte_buffer), 2)
                miso_byte = int(''.join(str(b[1]) for b in byte_buffer), 2)
                print(f"Complete byte - MOSI: 0x{mosi_byte:02X}, MISO: 0x{miso_byte:02X}")
                byte_buffer = []
                bit_count = 0
        
        last_sclk = sclk
        # Small delay to prevent CPU overload
        time.sleep(0.00001)
        
except KeyboardInterrupt:
    print("\nStopping SPI monitor...")
finally:
    GPIO.cleanup()