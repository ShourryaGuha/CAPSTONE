#include <stdio.h>
#include <unistd.h>

void run_task(int core) {
    while (1) {
        printf("C Task running on core %d, PID: %d\n", core, getpid());
        sleep(1);  // Slow down output for readability
    }
}
