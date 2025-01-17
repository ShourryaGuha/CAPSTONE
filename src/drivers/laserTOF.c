#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define UART_DEVICE "/dev/ttyS0"

int uart_init(const char *device) {
    int uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd == -1) {
        perror("Unable to open UART");
        return -1;
    }

    struct termios options;
    tcgetattr(uart_fd, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &options);

    return uart_fd;
}

int uart_send_data(int uart_fd, uint8_t *data, size_t length) {
    if (write(uart_fd, data, length) < 0) {
        perror("UART write failed");
        return -1;
    }
    return 0;
}

int main() {
    int uart_fd = uart_init(UART_DEVICE);
    if (uart_fd == -1) {
        return -1;
    }

    uint8_t data_frame[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    if (uart_send_data(uart_fd, data_frame, sizeof(data_frame)) == -1) {
        close(uart_fd);
        return -1;
    }

    close(uart_fd);
    return 0;
}