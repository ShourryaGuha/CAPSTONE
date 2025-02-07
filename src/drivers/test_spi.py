import spidev
import time

# Initialize SPI
spi = spidev.SpiDev()
spi.open(0, 0)  # Bus 0, Device 0
spi.max_speed_hz = 1000000  # 1MHz
spi.mode = 0

# Simple counter pattern to make it obvious if we're receiving data
counter = 0

try:
    while True:
        # Send counter value
        spi.xfer([counter])
        print(f"Sent: {counter}")
        
        # Increment counter, wrapping at 255
        counter = (counter + 1) % 256
        
        # Delay to make it easily visible
        time.sleep(0.5)

except KeyboardInterrupt:
    print("\nStopping SPI output...")
finally:
    spi.close()