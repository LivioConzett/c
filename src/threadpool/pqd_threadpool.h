/*
* A thread pool useing pthreads.
* Inspired by Lowlevel (https://www.youtube.com/@LowLevelTV)
*
*
*/

#ifndef PQD_THREADPOOL_H
#define PQD_THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
 * \param done condition to signal when all tasks are done
 * \param threads array of threads
 * \param task_queue array of tasks to do
 * \param idle_threads amount of threads waiting
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
  pthread_cond_t done;
  pthread_t *threads;
  tp_task_t *task_queue;
  uint16_t idle_threads;
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
 * \brief Destroy a threadpool. Will stop all the threads and free all the memory
 * \param pool threadpool to destroy
 * \return 0 on success
 */
int8_t tp_destroy(tp_pool_t* pool);

/**
 * \brief Adds a task to the queue
 * \param pool thread pool to add the task too
 * \param function function to add to the task
 * \param arg argument for the function
 * \return 0 on success
 */
int8_t tp_add_task(tp_pool_t *pool, void (*function)(void*), void *arg);

/**
 * \brief waits for all tasks to be done
 * \param pool pool to wait for
 * \return 0 on success
 */
int8_t tp_wait_for_tasks_done(tp_pool_t *pool);


// -----------------------------------------------------------------------------------
// function definition
// -----------------------------------------------------------------------------------

#ifdef PQD_THREADPOOL_IMPLEMENTATION

/**
 * \brief handles a single thread. Waits for the notify condition, before grabbing a task
 * \param pool threadpool to get the tasks from
 */
void* tp_thread_handler(void* threadpool) {
    tp_pool_t* pool = (tp_pool_t*)threadpool;

    while (1) {
        // lock the pool in order to access the task queue
        pthread_mutex_lock(&(pool->lock));

        // increment the idle threads
        pool->idle_threads++;

        // all threads are idle and no more task are queued
        if(pool->idle_threads == pool->thread_amount && pool->queued == 0){
            // signal that the threads are idle (all tasks done)
            pthread_cond_signal(&(pool->done));
        }

        // while there is no task to do, wait for one
        while (pool->queued == 0 && !pool->stop) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // if the thread should stop
        if (pool->stop) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // get a new task
        tp_task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->task_amount;
        pool->queued--;

        // decrement the idle threads since this one has something to do now
        pool->idle_threads--;

        // unlock the mutex, so that other threads can get a task
        pthread_mutex_unlock(&(pool->lock));
        
        // execute the task
        (*(task.function))(task.arg);
    }

    return NULL;
}


/**
 * See function declaration
 */
int8_t tp_init(uint16_t thread_amount, uint16_t task_amount, tp_pool_t *pool){
    
    // check the parameters
    if(thread_amount <= 0 || task_amount <= 0) {
        fprintf(stderr, "ERROR: thread_amount and task_amount need to be greater than 0!\n");
        return 1;
    }
    if(pool == NULL){
        fprintf(stderr, "ERROR: pool in NULL!\n");
        return 2;
    }

    pool->idle_threads = 0;
    pool->thread_amount = thread_amount;
    pool->task_amount = task_amount;
    pool->queued = 0;
    pool->queue_front = 0;
    pool->queue_back = 0;
    pool->stop = 0;

    // initialize the mutex
    pthread_mutex_init(&(pool->lock), NULL);

    // initialize the condition
    pthread_cond_init(&(pool->notify), NULL);

    // initialize the condition
    pthread_cond_init(&(pool->done), NULL);

    // allocate the memory for the threads
    pool->threads = (pthread_t *)malloc(thread_amount * sizeof(pthread_t));

    // allocate the memory for the task queue
    pool->task_queue = (tp_task_t *)malloc(task_amount * sizeof(tp_task_t));

    // start the threads
    for(uint16_t i = 0; i < thread_amount; i++){
        pthread_create(&(pool->threads[i]), NULL, tp_thread_handler, pool);
    }

    return 0;
}

/**
 * See function declaration
 */
int8_t tp_destroy(tp_pool_t* pool) {

    // check the input
    if(pool == NULL) return 1;

    // get the mutex for the pool
    pthread_mutex_lock(&(pool->lock));

    // set the sopt bit. The threads will stop when
    // this is set.
    pool->stop = 1;
    
    // broadcast the notify to awaken all the waiting threads
    pthread_cond_broadcast(&(pool->notify));

    // unlock the pool again.
    pthread_mutex_unlock(&(pool->lock));

    // wait for all the threads to join
    for (uint16_t i = 0; i < pool->thread_amount; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // destroy the mutex for the pool
    pthread_mutex_destroy(&(pool->lock));

    // destroy the notify condition
    pthread_cond_destroy(&(pool->notify));

    // destroy the notify condition
    pthread_cond_destroy(&(pool->done));

    // free the allocated memory
    //free(pool->task_queue);
    //pool->task_queue = NULL;
    free(pool->threads);
    pool->threads = NULL;

    return 0;
}

/**
 * See function declaration
 */
int8_t tp_add_task(tp_pool_t *pool, void (*function)(void*), void *arg){
    
    if(pool == NULL){
        fprintf(stderr, "ERROR: param pool is NULL\n");
        return 1;
    }
    if(function == NULL){
        fprintf(stderr, "ERROR: param function is NULL\n");
        return 2;
    }

    pthread_mutex_lock(&(pool->lock));

    int next_rear = (pool->queue_back + 1) % pool->task_amount;
    if(pool->queued < pool->task_amount){

        pool->task_queue[pool->queue_back].function = function;
        pool->task_queue[pool->queue_back].arg = arg;
        pool->queue_back = next_rear;
        pool->queued++;

        pthread_cond_signal(&(pool->notify));
        
    } else {
        pthread_mutex_unlock(&(pool->lock));

        fprintf(stderr, "ERROR: task queue full\n");
        return 6;
    }

    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

/**
 * See function declaration
 */
int8_t tp_wait_for_tasks_done(tp_pool_t *pool){

    // check if the pool is NULL
    if(pool == NULL){
        fprintf(stderr, "ERROR: pool is NULL\n");
        return 1;
    }

    // get the pool mutex
    pthread_mutex_lock(&(pool->lock));

    // if all the threads are idle, and no task queued, all task must be done
    if(pool->idle_threads == pool->thread_amount && pool->queued == 0){
        // unlock the mutex
        pthread_mutex_unlock(&(pool->lock));
        return 0;
    }

    // wait for the done cond
    pthread_cond_wait(&(pool->done), &(pool->lock));

    // unlock the mutex
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}


#endif // PQD_THREADPOOL_IMPLEMENTATION
#endif // PQD_THREADPOOL_H 
