#ifndef LASERTOF_H
#define LASERTOF_H

#include <stdint.h>
#define QUEUE_SIZE 500
#define GPS_BAUD_RATE 9600
#define GPS_UART_PORT 1

/**
 * @brief Status codes returned by the device
 */
typedef enum {
    STATUS_OK = 0x0000,              /**< No error */
    STATUS_LOW_POWER = 0x0001,       /**< Power input too low (should be >= 2.2V) */
    STATUS_TEMP_LOW = 0x0003,        /**< Module temperature too low (< -20°C) */
    STATUS_TEMP_HIGH = 0x0004,       /**< Module temperature too high (> +40°C) */
    STATUS_OUT_OF_RANGE = 0x0005,    /**< Target out of range */
    STATUS_INVALID_RESULT = 0x0006,  /**< Invalid measure result */
    STATUS_STRONG_BACKGROUND = 0x0007, /**< Background light too strong */
    STATUS_WEAK_SIGNAL = 0x0008,     /**< Laser signal too weak */
    STATUS_STRONG_SIGNAL = 0x0009,   /**< Laser signal too strong */
    STATUS_UNSTABLE = 0x000F,        /**< Laser signal not stable */
    STATUS_INVALID_FRAME = 0x0081    /**< Invalid frame received */
} LRF_Status;

/**
 * @brief Measurement modes
 */
typedef enum {
    MODE_AUTO = 0x00,  /**< Auto mode - adjusts speed based on signal strength */
    MODE_SLOW = 0x01,  /**< Slow mode - higher accuracy, slower speed */
    MODE_FAST = 0x02   /**< Fast mode - lower accuracy, higher speed */
} LRF_Mode;

/**
 * @brief Device handle structure
 */
typedef struct {
    int handle;         /**< UART handle */
    uint8_t address;    /**< Device address */
} LRF_Device;

/**
 * @brief Initialize the laser rangefinder
 * 
 * Initializes the UART communication, configures GPIO pins,
 * and performs the power-up sequence and auto baud rate detection.
 * 
 * @param device Pointer to device handle structure
 * @return 0 on success, negative value on error:
 *         -1: pigpio initialization failed
 *         -2: UART initialization failed
 *         -3: Auto baud rate detection failed
 */
int Laser_Init(LRF_Device *device);


/**
 * @brief Perform a single distance measurement
 * 
 * @param device Pointer to device handle structure
 * @param mode Measurement mode (Auto, Slow, or Fast)
 * @param distance Pointer to store the measured distance in meters
 * @return 0 on success, negative value on error:
 *         -1: Communication timeout
 *         -2: Invalid response
 */
int Laser_Measure(LRF_Device *device, LRF_Mode mode, float *distance);

/**
 * @brief Read the device status
 * 
 * @param device Pointer to device handle structure
 * @return Status code from LRF_Status enum
 */
LRF_Status Laser_GetStatus(LRF_Device *device);



#endif /* LASERTOF_H */

