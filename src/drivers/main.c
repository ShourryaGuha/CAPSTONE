#include <stdio.h>
#include <unistd.h>
#include "gps.h"

int main() {
    // Placeholder for now
    printf("I look forward to the war crimes that will commence shortly\n");

    gps_init();
    while(1) {
        test_read();
        usleep(10000);
    }
    terminate_connection();
    return 0;

    return 0;
}
