#include <pthread.h>
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
    println("Filósofo %d toma los tenedores %d y %d", ACTUAL_PHILOSOPHER_PRINT,
            LEFT_PRINT, RIGHT_PRINT);
    println("Filósofo %d está Comiendo", ACTUAL_PHILOSOPHER_PRINT);
  }
}

void take_fork(int ph_id) {
  state[ph_id] = HUNGRY;
  println("Filósofo %d está Hambriento", ACTUAL_PHILOSOPHER_PRINT);

  while (1) {
    if (state[LEFT] != EATING && state[RIGHT] != EATING) {
      state[ph_id] = EATING;
      usleep(200000);
      println("Filósofo %d toma los tenedores %d y %d",
              ACTUAL_PHILOSOPHER_PRINT, LEFT_PRINT, RIGHT_PRINT);
      println("Filósofo %d está Comiendo", ACTUAL_PHILOSOPHER_PRINT);
      break;
    }
    usleep(1);
  }
}

void put_fork(int ph_id) {
  state[ph_id] = THINKING;
  println("Filósofo %d dejó los tenedores %d y %d", ACTUAL_PHILOSOPHER_PRINT,
          LEFT_PRINT, RIGHT_PRINT);
  println("Filósofo %d está pensando", ACTUAL_PHILOSOPHER_PRINT);
  usleep(200000);
}

void *philosopher(void *id) {
  int *ph_id = id;
  while (1) {
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
  pthread_t *thread_id =
      (pthread_t *)malloc(N_PHILOSOPHERS * sizeof(pthread_t));

  for (int i = 0; i < N_PHILOSOPHERS; ++i) {
    ph[i] = i;
    state[i] = THINKING;
    println("Filósofo %d está pensando", i + 1);
  }

  for (int i = 0; i < N_PHILOSOPHERS; ++i) {
    pthread_create(&thread_id[i], NULL, philosopher, &ph[i]);
  }

  for (int i = 0; i < N_PHILOSOPHERS; ++i) {
    pthread_join(thread_id[i], NULL);
  }

  free(state);
  free(ph);
  free(thread_id);

  return EXIT_SUCCESS;
}
