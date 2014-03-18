#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
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
static void waitHelper(struct thread_pool *pool);

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
  struct list_elem elem;
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
    rc = pthread_create(&threads[x], NULL, threadCreateHelper, pool);
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
  struct thread_pool *pool = (struct thread_pool *) temp;
  assert(pool != NULL);
  struct list_elem *e = NULL;
  struct future *tempFuture = NULL;
  
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);
  
  while(pool != NULL && !pool->shutDown) {
    waitHelper(pool);
    
    while(pool != NULL && !list_empty(&pool->futureList) && !pool->shutDown) {
      //Get another future
      e = list_pop_front(&pool->futureList);
      assert(e != NULL);
      tempFuture = list_entry(e, struct future, elem);
      
      rc = pthread_mutex_unlock(&pool->mutex);
      checkResults("pthread_mutex_unlock()\n", rc);
    
      assert(tempFuture != NULL);
      tempFuture->result = tempFuture->callable(tempFuture->callable_data);
      sem_post(&tempFuture->semaphore);
      
      rc = pthread_mutex_lock(&pool->mutex);
      checkResults("pthread_mutex_lock()\n", rc);
    }
  }
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc);
  
  return NULL;
}

/*
 * Helper function: calls pthread_cond_wait() and checks it
 * 
 * pthread_cond_wait() will unlock the mutex until data is available,
 * and then it will relock it
 */
static void waitHelper(struct thread_pool *pool) {
  int rc = pthread_cond_wait(&pool->monitor, &pool->mutex);
  if(rc) {
    printf("pthread_cond_wait() failed\n");
    pthread_mutex_unlock(&pool->mutex);
    exit(1);
  }
}
   
void thread_pool_shutdown(struct thread_pool * pool) {
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);
  
  pool->shutDown = true;
  pthread_cond_broadcast(&pool->monitor);
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc); 
  
  free(pool);
}

struct future * thread_pool_submit(struct thread_pool * pool,
				   thread_pool_callable_func_t callable,
				   void * callable_data) {
  
  const int INIT_VAL = 0;
  
  struct future *newFuture = malloc(sizeof(struct future));
  newFuture->callable = callable;
  newFuture->callable_data = callable_data;
  newFuture->result = NULL;
  sem_init(&newFuture->semaphore, 0, INIT_VAL);
  
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);
  
  list_push_back(&pool->futureList, &newFuture->elem);
  pthread_cond_signal(&pool->monitor);  
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc);
  
  return newFuture;
}

void * future_get(struct future * future) {
  assert(future != NULL);
  void *temp = NULL;

  sem_wait(&future->semaphore);
  temp = future->result;
  
  return temp;
}

void future_free(struct future * future) {  
  sem_destroy(&future->semaphore);
  free(future);
}