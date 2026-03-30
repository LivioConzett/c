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

### declarations

``` C
typedef struct {
  void (*function)(void* arg);
  void* arg;
} tp_task_t;

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

int8_t tp_init(uint16_t thread_amount, uint16_t task_amount, tp_pool_t *threadpool);

```

### tp_init

``` C
int8_t tp_init(
                uint16_t thread_amount,
                uint16_t task_amount,
                tp_pool_t *threadpool
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
| threadpool    | thread pool to initialize           |

#### Return

| number | meaning                                                      |
|--------|--------------------------------------------------------------|
| 0      | success                                                      |
| 1      | param `thread_amount` or `task_amount` is 0                  |
| 2      | param `threadpool` is NULL                                   |
| 3      | `pthread_mutex_init()` failed while initializing the `lock`  |
| 4      | `pthread_cond_init()` failed while initializing the ``notify`|
| 5      | `threads` malloc failed                                      |
| 6      | `task_queue` malloc failed                                   |
| 7      | `pthread_create()` failed while creating a thread            |
