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
        if(pthread_mutex_lock(&(pool->lock)) != 0){
            fprintf(stderr, "ERROR in thread: mutex lock failed!\n");
            pthread_exit(NULL);
        }

        // while there is no task to do, wait for one
        while (pool->queued == 0 && !pool->stop) {
            if(pthread_cond_wait(&(pool->notify), &(pool->lock)) != 0){
                fprintf(stderr, "ERROR in thread: condition wait failed!\n");
                pthread_exit(NULL);
            }
        }

        // if the thread should stop
        if (pool->stop) {
            if(pthread_mutex_unlock(&(pool->lock)) != 0){
                fprintf(stderr, "ERROR in thread: mutex unlock failed on stop!\n");
            }
            pthread_exit(NULL);
        }

        // get a new task
        tp_task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->task_amount;
        pool->queued--;

        // unlock the mutex, so that other threads can get a task
        if(pthread_mutex_unlock(&(pool->lock)) != 0){
            fprintf(stderr, "ERROR in thread: mutex unlock failed!\n");
            pthread_exit(NULL);
        }
        
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
        if(pthread_create(&(pool->threads[i]), NULL, tp_thread_handler, pool) != 0){

            // terminate all the threads that have been started if the creation 
            // of one of them fails
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

    // get the mutex for the pool
    if(pthread_mutex_lock(&(pool->lock)) != 0) return 1;

    // set the sopt bit. The threads will stop when
    // this is set.
    pool->stop = 1;
    
    // broadcast the notify to awaken all the waiting threads
    if(pthread_cond_broadcast(&(pool->notify)) != 0) return 2;
    // unlock the pool again.
    if(pthread_mutex_unlock(&(pool->lock)) != 0) return 3;

    // wait for all the threads to join
    for (uint16_t i = 0; i < pool->thread_amount; i++) {
        if(pthread_join(pool->threads[i], NULL) != 0) return 4;
    }

    // destroy the mutex for the pool
    if(pthread_mutex_destroy(&(pool->lock)) != 0) return 5;
    // destroy the notify condition
    if(pthread_cond_destroy(&(pool->notify)) != 0) return 6;

    // free the allocated memory
    free(pool->task_queue);
    pool->task_queue = NULL;
    free(pool->threads);
    pool->threads = NULL;

    pool = NULL;

    return 0;
}

/**
 * See function declaration
 */
int8_t tp_add_task(tp_pool_t *pool, void (*function)(void*), void *arg){
    
    if(pthread_mutex_lock(&(pool->lock)) != 0) return 1;

    int next_rear = (pool->queue_back + 1) % pool->task_amount;
    if(pool->queued < pool->task_amount){
        pool->task_queue[pool->queue_back].function = function;
        pool->task_queue[pool->queue_back].arg = arg;
        pool->queue_back = next_rear;
        pool->queued++;
        if(pthread_cond_signal(&(pool->notify)) != 0){
            // remove the task again
            pool->queue_back = (pool->queue_back - 1) % pool->task_amount;
            pool->queued--;
            if(pthread_mutex_unlock(&(pool->lock)) != 0) return 3;
            return 2;
        }
    } else {
        return 4;
    }

    if(pthread_mutex_unlock(&(pool->lock)) != 0) return 5;
}

#endif // PQD_THREADPOOL_IMPLEMENTATION
#endif // PQD_THREADPOOL_H 
