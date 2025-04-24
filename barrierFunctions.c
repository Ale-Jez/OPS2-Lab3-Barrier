#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>    
#include <sys/wait.h>    
#include <pthread.h>


// Shared memory for barrier
pthread_barrier_t *barrier = mmap(NULL, sizeof(pthread_barrier_t),
    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

if (barrier == MAP_FAILED)
{
perror("mmap");
exit(EXIT_FAILURE);
}


// attribute for sharing
pthread_barrierattr_t attr;
if (pthread_barrierattr_init(&attr) != 0) {
    perror("pthread_barrierattr_init");
    exit(EXIT_FAILURE);
}
if (pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
    perror("pthread_barrierattr_setpshared");
    exit(EXIT_FAILURE);
}


// initialise +1 for parent proc    
if (pthread_barrier_init(barrier, &attr, NUM_CHILDREN + 1) != 0) {
    perror("pthread_barrier_init");
    exit(EXIT_FAILURE);
}


// usage
int ret = pthread_barrier_wait(barrier);
if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
{
    // work
}


// cleanup
pthread_barrier_destroy(barrier);
pthread_barrierattr_destroy(&attr);
munmap(barrier, sizeof(pthread_barrier_t));