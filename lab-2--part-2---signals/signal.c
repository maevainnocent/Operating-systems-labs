#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handler(int signum) {
  printf("Hello World!\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  signal(SIGALRM, handler);  // Register handler
  alarm(5);                  // Schedule alarm for 5 seconds
  while (1);                 // Busy wait
  return 0;                  // Never reached
}
