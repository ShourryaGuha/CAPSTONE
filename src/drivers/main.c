#include <stdio.h>
#include <unistd.h>
#include "gps.h"

int main() {
    // Placeholder for now
    printf("I look forward to the war crimes that will commence shortly\n");

    gps_init();
    while(1) {
        spin_gps();
        usleep(10000);
        test_final_values();
    }
    terminate_connection();
    return 0;

    return 0;
}
