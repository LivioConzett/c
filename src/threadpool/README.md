# threadpool

A library using `pthreads`to create a threadpool.

## Usage

To use the library define `PQD_THREADPOOL_IMPLEMENTATION` and
then include the `pqd_threadpool.h` file.  

``` C
#define PQD_THREADPOOL_IMPLEMENTATION
#include "pqd_threadpool.h"

int main(){

    return 0;
}
```

Adding some tasks to a thread pool:

```C
#include <stdio.h>
#include <unistd.h>

#define PQD_THREADPOOL_IMPLEMENTATION
#include "pqd_threadpool.h"

#define TASK_AMOUNT 10
#define THREAD_AMOUNT 3

// array for the task to write into
int task_numbers[TASK_AMOUNT];

/**
 * Function to run as task
 */
void task_function(void* arg){
    int index = *(int*)arg;
    test_number[index] = index;
    free(arg);
}

/**
 * Main function
 */
int main(){

  // create the threadpool
  tp_pool_t pool;

  // init the threadpool
  tp_init(THREAD_AMOUNT, TASK_AMOUNT, &pool);

  // create the tasks
  for(int i = 0; i < TASK_AMOUNT; i++){

      int *num = (int *)malloc(sizeof(int));
      *num = i;

      // add the tasks to the 
      tp_add_task(&pool, test_function, (void *)num);
  }

  // wait for all the task to finish
  tp_wait_for_tasks_done(&pool);

  // destroy the thread pool
  tp_destroy(&pool);

  return 0;
}
```

### declarations

``` C
typedef struct {
  void (*function)(void* arg);
  void* arg;
} tp_task_t;

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

int8_t tp_init(uint16_t thread_amount, uint16_t task_amount, tp_pool_t *pool);
int8_t tp_destroy(tp_pool_t* pool);
int8_t tp_add_task(tp_pool_t *pool, void (*function)(void*), void *arg);
int8_t tp_wait_for_tasks_done(tp_pool_t *pool);

```

### tp_init

``` C
int8_t tp_init(
                uint16_t thread_amount,
                uint16_t task_amount,
                tp_pool_t *pool
              );
```

#### Brief

Initializes the threadpool. Mallocs the memory for the `task_queue` and the `threads`
in the `tp_pool_t` structure.

#### Params

| param         | meaning                             |
|---------------|-------------------------------------|
| thread_amount | amount of threads to create in pool |
| task_amount   | length of task array to allocate    |
| pool          | thread pool to initialize           |

#### Return

| number | meaning                                                           |
|--------|-------------------------------------------------------------------|
| 0      | success                                                           |
| 1      | param `thread_amount` or `task_amount` is 0                       |
| 2      | param `pool` is NULL                                              |
| 3      | `pthread_mutex_init()` failed while initializing the `pool.lock`  |
| 4      | `pthread_cond_init()` failed while initializing the `pool.notify` |
| 5      | `pthread_cond_init()` failed while initializing the `pool.done`   |
| 6      | `threads` malloc failed                                           |
| 7      | `task_queue` malloc failed                                        |
| 8      | `pthread_create()` failed while creating a thread                 |

### tp_destroy

```C
int8_t tp_destroy(
                   tp_pool_t* pool
                 );
```

#### Brief

Destroys a threadpool. Will stop all the threads and free all the memory.

#### Params

| param         | meaning                   |
|---------------|---------------------------|
| pool          | thread pool to destroy    |

#### Return

| number | meaning                                                                  |
|--------|--------------------------------------------------------------------------|
| 0      | success                                                                  |
| 1      | param `pool` is NULL                                                     |
| 2      | `pthread_mutex_lock()` failed while locking the `tp_pool_t.lock`         |
| 3      | `pthread_cond_broadcast()` failed while notifying the `tp_pool_t.notify` |
| 4      | `pthread_mutex_unlock()` failed while unlocking the `tp_pool_t.lock`     |
| 5      | `pthread_join()` failed while joining the threads                        |
| 6      | `pthread_mutex_destroy()` failed while destroying the `tp_pool_t.lock`   |
| 7      | `pthread_cond_destroy()` failed while destroying the `tp_pool_t.done`    |
| 8      | `pthread_cond_destroy()` failed while destroying the `tp_pool_t.notify`  |

### tp_add_task

```C
int8_t tp_add_task(
                    tp_pool_t *pool,
                    void (*function)(void*),
                    void *arg
                  );
```

#### Brief

Adds a task to the task queue.

#### Params

| param         | meaning                          |
|---------------|----------------------------------|
| pool          | thread pool to add task to       |
| function      | function the task should execute |
| arg           | arguments for the function       |

#### Return

| number | meaning                                                                                                                    |
|--------|----------------------------------------------------------------------------------------------------------------------------|
| 0      | success                                                                                                                    |
| 1      | param `pool` is NULL                                                                                                       |
| 2      | param `function` is NULL                                                                                                   |
| 3      | `pthread_mutex_lock()` failed while locking the `tp_pool_t.lock`                                                           |
| 4      | `pthread_cond_signal()` failed while notifying the `tp_pool_t.notify`                                                      |
| 5      | `pthread_mutex_unlock()` failed while unlocking `tp_pool_t.lock` while cleaning up after the `pthread_cond_signal()` error |
| 6      | `tp_pool_t.task_queue` is momentarily full                                                                                 |
| 7      | `pthread_mutex_unlock()` failed while unlocking the `tp_pool_t.lock`                                                       |

### tp_wait_for_tasks_done

```C
int8_t tp_wait_for_tasks_done(
                              tp_pool_t *pool
                             );

```

#### Brief

Blocks and waits for all the tasks currently in the `task_queue` to be done. Technically it checks if all
the threads are idle and no more tasks are queued. If that is the case, then all the tasks must be done.

#### Params

| param         | meaning                       |
|---------------|-------------------------------|
| pool          | thread pool to wait for       |

#### Return

| number | meaning                                                              |
|--------|----------------------------------------------------------------------|
| 0      | success                                                              |
| 1      | param `pool` is NULL                                                 |
| 2      | `pthread_mutex_lock()` failed while locking the `tp_pool_t.lock`     |
| 3      | `pthread_cond_wait()` failed while waiting on `tp_pool_t.done`       |
| 4      | `pthread_mutex_unlock()` failed while unlocking `tp_pool_t.lock`     |
