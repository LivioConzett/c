/*
* A thread pool useing pthreads.
* Heavily inspired by Lowlevel (https://www.youtube.com/@LowLevelTV)
*
*
*/

#ifndef PQD_THREADPOOL_H
#define PQD_THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

// -----------------------------------------------------------------------------------
// Structure declarations
// -----------------------------------------------------------------------------------

/**
 * \brief structure for a task
 * \param function function to run
 * \param arg pointer to the args to pass to the function
 */
typedef struct {
  void (*function)(void* arg);
  void* arg;
} tp_task_t;

/**
 * \brief thread pool structure
 * \param lock mutex to lock the threadpool struct
 * \param notify condition to signal thread that new task has been added
 * \param threads array of threads
 * \param task_queue array of tasks to do
 * \param thread_amount amount of threads in the pool
 * \param task_amount length of task_queue array
 * \param queued amount of task needed to be done in the task_queue
 * \param queue_front first task in the queue
 * \param queue_back last task in the queue
 * \param stop stop bit to tell threads to end
 */
typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads;
  tp_task_t *task_queue;
  uint16_t thread_amount;
  uint16_t task_amount;
  uint16_t queued;
  uint16_t queue_front;
  uint16_t queue_back;
  uint8_t stop;
} tp_pool_t;

// -----------------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------------


/**
 * \brief Create and initialize the threadpool
 * \param thread_amount amount of threads to create
 * \param task_amount length of task queue to create
 * \param pool pointer to threadpool to initialize
 * \return 0 if success
 */
int8_t tp_init(uint16_t thread_amount, uint16_t task_amount, tp_pool_t *pool);

/**
 * \brief destroy a threadpool. Will stop all the threads and free all the memory
 * \param pool threadpool to destroy
 * \return 0 on success
 */
int8_t tp_destroy(tp_pool_t* pool);
// void threadpool_add_task(threadpool_t* pool, void (*function)(void*), void* arg);



// #ifdef PQD_THREADPOOL_IMPLEMENTATION
// -----------------------------------------------------------------------------------
// function definition
// -----------------------------------------------------------------------------------


/**
 * See function declaration
 */
int8_t tp_init(uint16_t thread_amount, uint16_t task_amount, tp_pool_t *pool){
    
    // check the parameters
    if(thread_amount <= 0 || task_amount <= 0) return 1;
    if(pool == NULL) return 2;

    pool->thread_amount = thread_amount;
    pool->task_amount = task_amount;
    pool->queued = 0;
    pool->queue_front = 0;
    pool->queue_back = 0;
    pool->stop = 0;

    // initialize the mutex
    if(pthread_mutex_init(&(pool->lock), NULL) != 0) return 3;

    // initialize the condition
    if(pthread_cond_init(&(pool->notify), NULL) != 0){
        pthread_mutex_destroy(&(pool->lock));
        return 4;
    }

    // allocate the memory for the threads
    pool->threads = (pthread_t *) malloc(thread_amount * sizeof(pthread_t));
    if(pool->threads == NULL){
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        return 5;
    }

    // allocate the memory for the task queue
    pool->task_queue = (tp_task_t *) malloc(task_amount * sizeof(tp_task_t));
    if(pool->task_queue = NULL){
        free(pool->threads);
        pool->threads = NULL;
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        return 6;
    }

    // start the threads
    for(uint16_t i = 0; i < thread_amount; i++){
        if(pthread_create(&(pool->threads[i]), NULL, thread_function, pool) != 0){

            // terminate all the threads that have been started if the creation 
            // if them fails
            pthread_mutex_lock(&(pool->lock));
            pool->stop = 1;
            pthread_cond_broadcast(&(pool->notify));
            pthread_mutex_unlock(&(pool->lock));

            for (uint16_t ii = 0; ii < i; ii++) {
                pthread_join(pool->threads[ii], NULL);
            }

            pthread_mutex_destroy(&(pool->lock));
            pthread_cond_destroy(&(pool->notify));

            free(pool->task_queue);
            pool->task_queue = NULL;
            
            free(pool->threads);
            pool->threads = NULL;

            return 7;
        }
    }

    return 0;
}

/**
 * See function declaration
 */
int8_t tp_destroy(tp_pool_t* pool) {

    pthread_mutex_lock(&(pool->lock));
    pool->stop = 1;
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for (uint16_t i = 0; i < pool->thread_amount; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));

    free(pool->task_queue);
    pool->task_queue = NULL;

    free(pool->threads);
    pool->threads = NULL;

    pool = NULL;

    return 0;
}



void* thread_function(void* threadpool) {
    threadpool_t* pool = (threadpool_t*)threadpool;
    printf("starting thread\n");

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        while (pool->queued == 0 && !pool->stop) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->stop) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % QUEUE_SIZE;
        pool->queued--;

        pthread_mutex_unlock(&(pool->lock));
        
        (*(task.fn))(task.arg);
    }

    return NULL;
}








void threadpool_add_task(threadpool_t *pool, void (*function)(void*), void *arg){
    pthread_mutex_lock(&(pool->lock));

    int next_rear = (pool->queue_back + 1) % QUEUE_SIZE;
    if(pool->queued < QUEUE_SIZE){
        pool->task_queue[pool->queue_back].fn = function;
        pool->task_queue[pool->queue_back].arg = arg;
        pool->queue_back = next_rear;
        pool->queued++;
        pthread_cond_signal(&(pool->notify));
    } else {
        printf("Task queue is full! Cannot add more tasks.\n");
    }

    pthread_mutex_unlock(&(pool->lock));
}
#endif // PQD_THREADPOOL_IMPLEMENTATION

#endif // PQD_THREADPOOL_H 