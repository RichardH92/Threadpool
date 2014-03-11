#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include "list.h"
#include "threadpool.h"

struct thread_pool {
  struct list *futureList;
  bool shutDown;
  struct future *monitor;
  pthread_mutex_t *mutex;
};

struct future {
  thread_pool_callable_func_t callable;
  void *callable_data;
  void *result;
  sem_t *semaphore;
};

struct thread_pool * thread_pool_new(int nthreads) {
  struct thread_pool *pool = malloc(sizeof(struct thread_pool));
  list_init(pool->futureList);
  pthread_mutex_init(pool->mutex, 0);
  pool->shutDown = false;
  pool->monitor = 0;
  
  int x = 0;
  for(; x < nthreads; x++) {
    
  }
  
  return pool;
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
  sem_init(fut->semaphore, 0, INIT_VAL);
  
  //TODO: List manipulations here
  
  return fut;
}

void * future_get(struct future * fut) {
  return 0;
}

void future_free(struct future * fut) {
  
}