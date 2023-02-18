#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

int main() {
    int i = 0;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    while (i < 100) {

        if(i==20){
            sprintf(command, "echo 'Message number %d' > /dev/baoipc0 \n echo c > /proc/sysrq-trigger", i);
            printf("Send message number %d and echo c > /proc/sysrq-trigger\n", i);
            system(command); 
        } 

        // Send message
        sprintf(command, "echo 'Message number %d' > /dev/baoipc0", i);
        printf("Send message number %d\n", i);
        system(command);

        // Read answer
        sprintf(command, "cat /dev/baoipc0");
        printf("Read message number %d\n", i);
        system(command);

        i++;
        sleep(1);
    }

    return 0;
}
