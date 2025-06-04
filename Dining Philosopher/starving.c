#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THINKING 2
#define HUNGRY 1
#define EATING 0

#define LEFT_FORK(ph_id) (ph_id)
#define RIGHT_FORK(ph_id) ((ph_id + 1) % N_PHILOSOPHERS)

#define LEFT_PRINT (ph_id + 1)
#define RIGHT_PRINT ((ph_id + (N_PHILOSOPHERS - 1)) % N_PHILOSOPHERS + 1)
#define ACTUAL_PHILOSOPHER_PRINT (ph_id + 1)

int N_PHILOSOPHERS;
int *state;

pthread_mutex_t *fork_mutex;

void println(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  putchar('\n');
}

void take_forks(int ph_id) {
  state[ph_id] = HUNGRY;
  println("Filósofo %d está Hambriento", ACTUAL_PHILOSOPHER_PRINT);

  pthread_mutex_lock(&fork_mutex[LEFT_FORK(ph_id)]);
  println("Filósofo %d tomó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT,
          LEFT_FORK(ph_id) + 1);

  usleep(100);

  pthread_mutex_lock(&fork_mutex[RIGHT_FORK(ph_id)]);
  println("Filósofo %d tomó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT,
          RIGHT_FORK(ph_id) + 1);

  state[ph_id] = EATING;
  println("Filósofo %d está Comiendo", ACTUAL_PHILOSOPHER_PRINT);

  usleep(200000);
}

void put_forks(int ph_id) {
  pthread_mutex_unlock(&fork_mutex[RIGHT_FORK(ph_id)]);
  println("Filósofo %d dejó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT,
          RIGHT_FORK(ph_id) + 1);

  pthread_mutex_unlock(&fork_mutex[LEFT_FORK(ph_id)]);
  println("Filósofo %d dejó el tenedor %d", ACTUAL_PHILOSOPHER_PRINT,
          LEFT_FORK(ph_id) + 1);

  state[ph_id] = THINKING;
  println("Filósofo %d está pensando", ACTUAL_PHILOSOPHER_PRINT);

  usleep(200000);
}

void *philosopher(void *id) {
  int ph_id = *(int *)id;

  while (1) {
    usleep(100000);

    take_forks(ph_id);

    put_forks(ph_id);
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
  int *ph = (int *)malloc(N_PHILOSOPHERS * sizeof(int));
  fork_mutex =
      (pthread_mutex_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_mutex_t));

  pthread_t *thread_id =
      (pthread_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_t));

  for (int i = 0; i < N_PHILOSOPHERS; ++i) {
    ph[i] = i;
    pthread_mutex_init(&fork_mutex[i], NULL);
    state[i] = THINKING;
    println("Filósofo %d está pensando", i + 1);
  }

  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_create(&thread_id[i], NULL, philosopher, &ph[i]);

  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_join(thread_id[i], NULL);

  for (int i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_mutex_destroy(&fork_mutex[i]);

  free(state);
  free(ph);
  free(fork_mutex);
  free(thread_id);

  return EXIT_SUCCESS;
}
