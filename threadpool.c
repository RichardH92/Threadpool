#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "list.h"
#include "threadpool.h"

#define checkResults(string, val) {		 \
  if(val) {					 \
    printf("Failed with %d at %s", val, string); \
    exit(1);					 \
  }						 \
}

static void *threadCreateHelper(void *temp);

struct thread_pool {
  struct list futureList;
  bool shutDown;
  pthread_cond_t monitor;
  pthread_mutex_t mutex;
};

struct future {
  thread_pool_callable_func_t callable;
  void *callable_data;
  void *result;
  sem_t semaphore;
};

/*
 * Initializes a new thread_pool of n threads
 */
struct thread_pool * thread_pool_new(int nthreads) {
  //Dynamically allocate memory for a new thread_pool,
  //and initialize the variables
  struct thread_pool *pool = malloc(sizeof(struct thread_pool));
  list_init(&pool->futureList);
  pthread_cond_init(&pool->monitor, NULL);
  pthread_mutex_init(&pool->mutex, NULL);
  pool->shutDown = false;
  
  //Initialize the threads
  pthread_t threads[nthreads];
  int x = 0;
  int rc = 0;
  for(; x < nthreads; x++) {
    rc = pthread_create(&threads[x], NULL, threadCreateHelper, NULL);
    checkResults("pthread_create()\n", rc);
  }
  
  return pool;
}

/*
 * Helper function for creating threads
 * pthread_create() require the function it takes as a param to have the 
 * form void * (void *). We only are going to pass in a 
 * struct thread_pool *, so we must typecast.
 * 
 * This function calls pthread_cond_wait() and essentially waits the thread
 * until there is a future in futureList for it to execute
 */
static void *threadCreateHelper(void *temp) {
  //Typecast 
  struct thread_pool *pool = (struct thread_pool *) temp;
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);

  //Wait until a future is added
  rc = pthread_cond_wait(&pool->monitor, &pool->mutex);
  if(rc) {
    perror("pthread_cond_wait() failed\n");
    pthread_mutex_unlock(&pool->mutex);
    exit(1);
  }
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc);
  
  return NULL;
}
   
void thread_pool_shutdown(struct thread_pool * pool) {
  
}

struct future * thread_pool_submit(struct thread_pool * pool,
				   thread_pool_callable_func_t callable,
				   void * callable_data) {
  const int INIT_VAL = 5;
  
  struct future *fut = malloc(sizeof(struct future));
  fut->callable = callable;
  fut->callable_data = callable_data;
  sem_init(&fut->semaphore, 0, INIT_VAL);
  
  //TODO: List manipulations here
  
  return fut;
}

void * future_get(struct future * fut) {
  return 0;
}

void future_free(struct future * fut) {
  
}