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
        int ret = pthread_mutex_lock(&(pool->lock));
        if(ret != 0){
            fprintf(stderr, "THREAD ERROR: mutex lock failed with code %d\n", ret);
            pthread_exit(NULL);
        }

        // while there is no task to do, wait for one
        while (pool->queued == 0 && !pool->stop) {
            ret = pthread_cond_wait(&(pool->notify), &(pool->lock));
            if(ret != 0){
                fprintf(stderr, "THREAD ERROR: condition wait failed with code %d\n", ret);
                pthread_exit(NULL);
            }
        }

        // if the thread should stop
        if (pool->stop) {
            ret = pthread_mutex_unlock(&(pool->lock));
            if(ret != 0){
                fprintf(stderr, "THREAD ERROR: mutex unlock failed on stop with code %d\n", ret);
            }
            pthread_exit(NULL);
        }

        // get a new task
        tp_task_t task = pool->task_queue[pool->queue_front];
        pool->queue_front = (pool->queue_front + 1) % pool->task_amount;
        pool->queued--;

        // unlock the mutex, so that other threads can get a task
        ret = pthread_mutex_unlock(&(pool->lock));
        if(ret != 0){
            fprintf(stderr, "THREAD ERROR: mutex unlock failed with code %d\n", ret);
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
    if(thread_amount <= 0 || task_amount <= 0) {
        fprintf(stderr, "ERROR: thread_amount and task_amount need to be greater than 0!\n");
        return 1;
    }
    if(pool == NULL){
        fprintf(stderr, "ERROR: pool in NULL!\n");
        return 2;
    }

    pool->thread_amount = thread_amount;
    pool->task_amount = task_amount;
    pool->queued = 0;
    pool->queue_front = 0;
    pool->queue_back = 0;
    pool->stop = 0;

    // initialize the mutex
    int ret = pthread_mutex_init(&(pool->lock), NULL);
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex init failed with code %d\n", ret);
        return 3;
    }

    // initialize the condition
    ret = pthread_cond_init(&(pool->notify), NULL);
    if(ret != 0){
        pthread_mutex_destroy(&(pool->lock));
        fprintf(stderr, "ERROR: pthread cond init failed with code %d!\n", ret);
        return 4;
    }

    // allocate the memory for the threads
    pool->threads = (pthread_t *)malloc(thread_amount * sizeof(pthread_t));
    if(pool->threads == NULL){
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        fprintf(stderr, "ERROR: Allocating threads array failed!\n");
        return 5;
    }

    // allocate the memory for the task queue
    pool->task_queue = (tp_task_t *)malloc(task_amount * sizeof(tp_task_t));
    if(pool->task_queue == NULL){
        free(pool->threads);
        pool->threads = NULL;
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        fprintf(stderr, "ERROR: Allocating task queue failed!\n");
        return 6;
    }

    // start the threads
    for(uint16_t i = 0; i < thread_amount; i++){
        ret = pthread_create(&(pool->threads[i]), NULL, tp_thread_handler, pool);
        if(ret != 0){
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

            fprintf(stderr, "ERROR: pthread create failed with code %d\n", ret);
            return 7;
        }
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
    int ret = pthread_mutex_lock(&(pool->lock));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex lock failed with code %d\n", ret);
        return 2;
    }

    // set the sopt bit. The threads will stop when
    // this is set.
    pool->stop = 1;
    
    // broadcast the notify to awaken all the waiting threads
    ret = pthread_cond_broadcast(&(pool->notify));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread cond broadcast failed with code %d\n", ret);
        return 3;
    }

    // unlock the pool again.
    ret = pthread_mutex_unlock(&(pool->lock));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex unlock failed with code %d\n", ret);
        return 4;
    }

    // wait for all the threads to join
    for (uint16_t i = 0; i < pool->thread_amount; i++) {
        ret = pthread_join(pool->threads[i], NULL);
        if(ret != 0){
            fprintf(stderr, "ERROR: pthread join failed with code %d\n", ret);
            return 5;
        }
    }

    // destroy the mutex for the pool
    ret = pthread_mutex_destroy(&(pool->lock));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex destroy failed with code %d\n", ret);
        return 6;
    }

    // destroy the notify condition
    ret = pthread_cond_destroy(&(pool->notify));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread cond destroy failed with code %d\n", ret);
        return 7;
    }

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

    int ret = pthread_mutex_lock(&(pool->lock));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex lock failed with code %d\n", ret);
        return 3;
    }

    int next_rear = (pool->queue_back + 1) % pool->task_amount;
    if(pool->queued < pool->task_amount){

        pool->task_queue[pool->queue_back].function = function;
        pool->task_queue[pool->queue_back].arg = arg;
        pool->queue_back = next_rear;
        pool->queued++;

        ret = pthread_cond_signal(&(pool->notify));
        if(ret != 0){
            fprintf(stderr, "ERROR: pthread cond signal failed with code %d\n", ret);
            // remove the task again
            pool->queue_back = (pool->queue_back - 1) % pool->task_amount;
            pool->queued--;
            ret = pthread_mutex_unlock(&(pool->lock));
            if(ret != 0){
                fprintf(stderr, "ERROR: pthread mutex unlock failed with code %d\n", ret);
                return 5;
            }
            return 4;
        }
    } else {
        fprintf(stderr, "ERROR: task queue full\n");
        return 6;
    }

    ret = pthread_mutex_unlock(&(pool->lock));
    if(ret != 0){
        fprintf(stderr, "ERROR: pthread mutex unlock failed with code %d\n", ret);
        return 7;
    }

    return 0;
}

#endif // PQD_THREADPOOL_IMPLEMENTATION
#endif // PQD_THREADPOOL_H 
