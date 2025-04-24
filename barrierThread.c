/*
each runner is a child process
and a jury is another child process managing the barrier
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_RUNNERS 5
#define NUM_STAGES 3

pthread_barrier_t barrier_runners;  // Barrier for runners after each stage
pthread_barrier_t barrier_jury;     // Barrier to synchronize jury and runners before the next stage

// Runner thread function
void *runner(void *arg) {
    int runner_id = *((int *)arg);
    free(arg); // Free memory for runner_id

    for (int stage = 0; stage < NUM_STAGES; stage++) {
        // Simulate runner finishing the stage by sleeping for a random time
        int time_to_complete = rand() % 5 + 1;  // Random time between 1 and 5 seconds
        printf("Runner %d starting stage %d, will take %d seconds.\n", runner_id, stage + 1, time_to_complete);
        sleep(time_to_complete);

        // After finishing the stage, synchronize with others at the first barrier
        printf("Runner %d finished stage %d.\n", runner_id, stage + 1);
        
        // Wait at the barrier for all runners (including the jury)
        pthread_barrier_wait(&barrier_runners);

        // After the first barrier, wait for the jury to allow the next stage
        pthread_barrier_wait(&barrier_jury);
    }

    return NULL;
}



// Jury thread function
void *jury(void *arg) {
    for (int stage = 0; stage < NUM_STAGES; stage++) {
        printf("Jury waiting for all runners to finish stage %d.\n", stage + 1);

        // Jury waits for all runners to reach the barrier for the current stage
        pthread_barrier_wait(&barrier_runners);

        if (stage+2 <= NUM_STAGES)
            // Once all runners have finished the stage, the jury can start the next stage
            printf("Jury starting stage %d.\n", stage + 2);

        // After the jury has started the stage, allow the runners to proceed to the next stage
        pthread_barrier_wait(&barrier_jury);
    }

    return NULL;
}

int main() {
    srand(time(NULL)); // Seed for random number generation

    // Initialize the barriers:
    // - barrier_runners is used to synchronize the runners and jury at the end of each stage
    // - barrier_jury is used to synchronize the runners before starting the next stage
    pthread_barrier_init(&barrier_runners, NULL, NUM_RUNNERS + 1);
    pthread_barrier_init(&barrier_jury, NULL, NUM_RUNNERS + 1);

    pthread_t jury_thread;
    pthread_t runner_threads[NUM_RUNNERS];

    // Create the jury thread
    pthread_create(&jury_thread, NULL, jury, NULL);

    // Create the runner threads
    for (int i = 0; i < NUM_RUNNERS; i++) {
        int *runner_id = malloc(sizeof(int));  // Allocate memory for runner ID
        *runner_id = i + 1;
        pthread_create(&runner_threads[i], NULL, runner, runner_id);
    }

    // Wait for all runner threads to finish
    for (int i = 0; i < NUM_RUNNERS; i++) {
        pthread_join(runner_threads[i], NULL);
    }

    // Wait for the jury thread to finish
    pthread_join(jury_thread, NULL);

    // Destroy the barriers
    pthread_barrier_destroy(&barrier_runners);
    pthread_barrier_destroy(&barrier_jury);

    return 0;
}
