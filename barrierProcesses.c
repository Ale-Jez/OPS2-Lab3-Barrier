/*
each runner is a child process
and a jury is another child process managing the barrier
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR(source)                                                            \
  do {                                                                         \
    fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                            \
    perror(source);                                                            \
    kill(0, SIGKILL);                                                          \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define NUM_RUNNERS 5
#define NUM_STAGES 3

typedef struct sharedData {
  pthread_barrier_t *barrier_jury;
  pthread_barrier_t *barrier_runners;
} sharedData_t;

// Runner thread function
void runner_work(int runner_id, pthread_barrier_t *barrier_jury,
                 pthread_barrier_t *barrier_runners) {
  srand(getpid());
  for (int stage = 0; stage < NUM_STAGES; stage++) {
    // Simulate runner finishing the stage by sleeping for a random time
    int time_to_complete =
        rand() % 5 + 1; // Random time between 1 and 5 seconds
    printf("Runner %d starting stage %d, will take %d seconds.\n", runner_id,
           stage + 1, time_to_complete);
    sleep(time_to_complete);

    // After finishing the stage, synchronize with others at the first barrier
    printf("Runner %d finished stage %d.\n", runner_id, stage + 1);

    // Wait at the barrier for all runners (including the jury)
    pthread_barrier_wait(barrier_runners);

    // After the first barrier, wait for the jury to allow the next stage
    pthread_barrier_wait(barrier_jury);
  }
}

// Jury thread function
void jury_work(pthread_barrier_t *barrier_jury,
               pthread_barrier_t *barrier_runners) {
  for (int stage = 0; stage < NUM_STAGES; stage++) {
    printf("Jury waiting for all runners to finish stage %d.\n", stage + 1);

    // Jury waits for all runners to reach the barrier for the current stage
    pthread_barrier_wait(barrier_runners);

    if (stage + 2 <= NUM_STAGES)
      // Once all runners have finished the stage, the jury can start the next
      // stage
      printf("Jury starting stage %d.\n", stage + 2);

    // After the jury has started the stage, allow the runners to proceed to the
    // next stage
    pthread_barrier_wait(barrier_jury);
  }
}

void createChildren(sharedData_t *sharedData) {
  int ret;
  for (int i = 0; i < NUM_RUNNERS; i++) {
    if (-1 == (ret = fork()))
      ERR("fork");

    if (ret == 0) // in a child
    {
      runner_work(i, sharedData->barrier_jury, sharedData->barrier_runners);
      exit(EXIT_SUCCESS);
    }
  }

  if (-1 == (ret = fork()))
    ERR("fork");
  if (ret == 0) // jury
  {
    jury_work(sharedData->barrier_jury, sharedData->barrier_runners);
    exit(EXIT_SUCCESS);
  }
}

int main() {
  srand(getpid());

  pthread_barrier_t *barrier_jury =
      mmap(NULL, sizeof(pthread_barrier_t), PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (barrier_jury == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  pthread_barrier_t *barrier_runners =
      mmap(NULL, sizeof(pthread_barrier_t), PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if (barrier_runners == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  pthread_barrierattr_t attr;
  if (pthread_barrierattr_init(&attr) != 0) {
    perror("pthread_barrierattr_init");
    exit(EXIT_FAILURE);
  }
  if (pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0) {
    perror("pthread_barrierattr_setpshared");
    exit(EXIT_FAILURE);
  }

  if (pthread_barrier_init(barrier_jury, &attr, NUM_RUNNERS + 1) != 0) {
    perror("pthread_barrier_init");
    exit(EXIT_FAILURE);
  }
  if (pthread_barrier_init(barrier_runners, &attr, NUM_RUNNERS + 1) != 0) {
    perror("pthread_barrier_init");
    exit(EXIT_FAILURE);
  }
  pthread_barrierattr_destroy(&attr);

  sharedData_t *sharedData =
      mmap(NULL, sizeof(sharedData_t), PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sharedData->barrier_jury = barrier_jury;
  sharedData->barrier_runners = barrier_runners;
  createChildren(sharedData);

  while (wait(NULL) > 0)
    ;

  // cleanup
  pthread_barrier_destroy(barrier_jury);
  munmap(barrier_jury, sizeof(pthread_barrier_t));

  pthread_barrier_destroy(barrier_runners);
  munmap(barrier_runners, sizeof(pthread_barrier_t));
  return 0;
}
