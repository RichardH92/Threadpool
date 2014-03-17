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
static void futureHelper(struct thread_pool *pool);

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
  printf("Test thread_pool_new\n");
  
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
  printf("Test threadCreateHelper\n");
  
  struct thread_pool *pool = (struct thread_pool *) temp;
  assert(pool != NULL);

  while(!pool->shutDown) {
    int rc = pthread_mutex_lock(&pool->mutex);
    checkResults("pthread_mutex_lock()\n", rc);
    
    //Wait until a future is added
    waitHelper(pool);
    
    //While there are futures waiting to execute
    while(pool != NULL && !list_empty(&pool->futureList)) {
      //Execute the future
      futureHelper(pool);
    }
    
    rc = pthread_mutex_unlock(&pool->mutex);
    checkResults("pthread_mutex_unlock()\n", rc);
  }
  
  return NULL;
}

/*
 * Helper function: calls pthread_cond_wait() and checks it
 * 
 * pthread_cond_wait() will unlock the mutex until data is available,
 * and then it will relock it
 */
static void waitHelper(struct thread_pool *pool) {
  printf("Test waitHelper\n");
  
  int rc = pthread_cond_wait(&pool->monitor, &pool->mutex);
  if(rc) {
    printf("pthread_cond_wait() failed\n");
    pthread_mutex_unlock(&pool->mutex);
    exit(1);
  }
}

/*
 * Helper function to execute a future, and then store its
 * result
 */
static void futureHelper(struct thread_pool *pool) {
  printf("Test futureHelper\n");
  
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);
  
  //Get another future
  struct list_elem *e = list_pop_front(&pool->futureList);
  struct future *tempFuture = list_entry(e, struct future, elem);
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc);
  
  //printf("Waiting\n");
  //Execute that future and store its result
  //sem_wait(&tempFuture->semaphore);
  printf("Function called\n");
  tempFuture->result = tempFuture->callable(tempFuture->callable_data);
  printf("Function returned\n");
  sem_post(&tempFuture->semaphore);     
}
   
void thread_pool_shutdown(struct thread_pool * pool) {
  printf("Test thread_pool_shutdown\n");
  
  int rc = pthread_mutex_lock(&pool->mutex);
  checkResults("pthread_mutex_lock()\n", rc);
  
  pool->shutDown = true;
  
  rc = pthread_mutex_unlock(&pool->mutex);
  checkResults("pthread_mutex_unlock()\n", rc);
  
  /*struct list_elem *e;
  struct future *tempFuture
  
  while(!list_empty(&pool->futureList)) {
      e = list_pop_front(&pool->futureList);
      tempFuture = list_entry(e, struct future, elem);
      future_free(tempFuture);
  }*/
  
  assert(list_empty(&pool->futureList));
  
  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->monitor);
  free(pool);
  
  exit(1);
}

struct future * thread_pool_submit(struct thread_pool * pool,
				   thread_pool_callable_func_t callable,
				   void * callable_data) {
  printf("Test thread_pool_submit\n");
  
  const int INIT_VAL = 1;
  
  struct future *newFuture = malloc(sizeof(struct future));
  newFuture->callable = callable;
  newFuture->callable_data = callable_data;
  newFuture->result = NULL;
  sem_init(&newFuture->semaphore, 0, INIT_VAL);
  
  list_push_back(&pool->futureList, &newFuture->elem);
  pthread_cond_signal(&pool->monitor);    
  
  return newFuture;
}

void * future_get(struct future * future) {
  printf("Test future_get\n");
  
  assert(future != NULL);
  void *temp = NULL;
  
  //while(temp == NULL) {
    //printf("temp == NULL\n");
    //sem_wait(&future->semaphore);
    //printf("Done waiting in future_get\n");
    //temp = future->result;
    //sem_post(&future->semaphore);
  //}
    
    while(temp == NULL) {
      sem_wait(&future->semaphore);
      temp = future->result;
    }
  
  return temp;
}

void future_free(struct future * future) {
  printf("Test future_free\n");
  
  int val = -1;
  while(val != 0) {
    sem_wait(&future->semaphore);
    sem_getvalue(&future->semaphore, &val);
    sem_post(&future->semaphore);
    val++;
  }
  
  sem_destroy(&future->semaphore);
  free(future);
}