#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THINKING 2
#define HUNGRY 1
#define EATING 0
#define LEFT (ph_id + (N_PHILOSOPHERS - 1)) % N_PHILOSOPHERS
#define RIGHT (ph_id + (N_PHILOSOPHERS - 4)) % N_PHILOSOPHERS
#define LEFT_PRINT (LEFT + 1)
#define RIGHT_PRINT (RIGHT + 1)
#define ACTUAL_PHILOSOPHER_PRINT (ph_id + 1)

int N_PHILOSOPHERS;
int *state;
int *ph;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t *cond;

void println(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  putchar('\n');
}

void test(int ph_id) {
  if (state[ph_id] == HUNGRY && state[LEFT] != EATING &&
      state[RIGHT] != EATING) {
    state[ph_id] = EATING;

    usleep(200000);

    println("Filósofo %d toma los comensales %d y %d", ACTUAL_PHILOSOPHER_PRINT,
            LEFT_PRINT, RIGHT_PRINT);

    println("Filósofo %d está Comiendo", ACTUAL_PHILOSOPHER_PRINT);

    pthread_cond_signal(&cond[ph_id]);
  }
}

void take_fork(int ph_id) {
  pthread_mutex_lock(&mutex);

  state[ph_id] = HUNGRY;

  println("Filósofo %d está Hambriento", ACTUAL_PHILOSOPHER_PRINT);

  test(ph_id);

  while (state[ph_id] != EATING) {
    pthread_cond_wait(&cond[ph_id], &mutex);
  }

  pthread_mutex_unlock(&mutex);

  usleep(200000);
}

void put_fork(int ph_id) {
  pthread_mutex_lock(&mutex);

  state[ph_id] = THINKING;

  println("Filósofo %d dejó los comensales %d y %d", ACTUAL_PHILOSOPHER_PRINT,
          LEFT_PRINT, RIGHT_PRINT);

  println("Filósofo %d está pensando", ACTUAL_PHILOSOPHER_PRINT);

  test(LEFT);
  test(RIGHT);

  pthread_mutex_unlock(&mutex);

  usleep(200000);
}

void *philosopher(void *id) {
  while (1) {
    int *ph_id = id;

    take_fork(*ph_id);

    put_fork(*ph_id);
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
  cond = (pthread_cond_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_cond_t));

  int i;
  pthread_t *thread_id =
      (pthread_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_t));

  for (i = 0; i < N_PHILOSOPHERS; ++i) {
    ph[i] = i;
    pthread_cond_init(&cond[i], NULL);
    state[i] = THINKING;
    println("Filósofo %d está pensando", i + 1);
  }

  for (i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_create(&thread_id[i], NULL, philosopher, &ph[i]);

  for (i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_join(thread_id[i], NULL);

  pthread_mutex_destroy(&mutex);
  for (i = 0; i < N_PHILOSOPHERS; ++i)
    pthread_cond_destroy(&cond[i]);

  free(state);
  free(ph);
  free(cond);
  free(thread_id);

  return EXIT_SUCCESS;
}
