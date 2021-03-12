#include <stdio.h>
#include <unistd.h>


int main(int argc, char **argv) {
    printf("Argument count: %d\n", argc);
    printf("My arguments:%s\n",*argv);
    sleep(5);
    printf("Hello, World!\n");
}