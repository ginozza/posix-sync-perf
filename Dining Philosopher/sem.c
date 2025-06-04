#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THINKING 2
#define HUNGRY 1
#define EATING 0

#define LEFT (ph_id + (N_PHILOSOPHERS - 1)) % N_PHILOSOPHERS
#define RIGHT (ph_id + 1) % N_PHILOSOPHERS
#define LEFT_PRINT (LEFT + 1)
#define RIGHT_PRINT (RIGHT + 1)
#define ACTUAL_PHILOSOPHER_PRINT (ph_id + 1)

int N_PHILOSOPHERS;
int *state;
int *ph;

sem_t *fork_sem;
sem_t room_sem;

void println(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  putchar('\n');
}

void take_fork(int ph_id) {
  sem_wait(&room_sem);

  state[ph_id] = HUNGRY;
  println("Filósofo %d está Hambriento", ACTUAL_PHILOSOPHER_PRINT);

  sem_wait(&fork_sem[LEFT]);
  println("Filósofo %d tomó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT, LEFT_PRINT);

  sem_wait(&fork_sem[RIGHT]);
  println("Filósofo %d tomó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT, RIGHT_PRINT);

  state[ph_id] = EATING;
  println("Filósofo %d está Comiendo", ACTUAL_PHILOSOPHER_PRINT);
  usleep(100000);
}

void put_fork(int ph_id) {
  sem_post(&fork_sem[RIGHT]);
  println("Filósofo %d dejó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT, RIGHT_PRINT);

  sem_post(&fork_sem[LEFT]);
  println("Filósofo %d dejó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT, LEFT_PRINT);

  state[ph_id] = THINKING;
  println("Filósofo %d está pensando", ACTUAL_PHILOSOPHER_PRINT);
  usleep(100000);

  sem_post(&room_sem);
}

void *philosopher(void *id) {
  int ph_id = *(int *)id;
  while (1) {
    take_fork(ph_id);
    put_fork(ph_id);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Uso: %s <numero_de_filosofos>\n", argv[0]);
    return EXIT_FAILURE;
  }

  N_PHILOSOPHERS = atoi(argv[1]);
  if (N_PHILOSOPHERS <= 1) {
    printf("El número de filósofos debe ser mayor que 1\n");
    return EXIT_FAILURE;
  }

  state = (int *)malloc(N_PHILOSOPHERS * sizeof(int));
  ph = (int *)malloc(N_PHILOSOPHERS * sizeof(int));
  fork_sem = (sem_t *)malloc(N_PHILOSOPHERS * sizeof(sem_t));
  pthread_t *thread_id = (pthread_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_t));

  sem_init(&room_sem, 0, N_PHILOSOPHERS - 1);

  for (int i = 0; i < N_PHILOSOPHERS; ++i) {
    ph[i] = i;
    sem_init(&fork_sem[i], 0, 1);
    state[i] = THINKING;
    println("Filósofo %d está pensando", i + 1);
  }

  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_create(&thread_id[i], NULL, philosopher, &ph[i]);

  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_join(thread_id[i], NULL);

  sem_destroy(&room_sem);
  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    sem_destroy(&fork_sem[i]);

  free(state);
  free(ph);
  free(fork_sem);
  free(thread_id);

  return EXIT_SUCCESS;
}
