#ifndef PTHREAD_H
#define PTHREAD_H

#include <sched.h>

/* CHECK */
typedef int pthread_t;

struct pthread_queue
{
	int threadId;
	struct pthread_queue* next;
};

struct pthread_mutex
{
	volatile long int _lock, _listlock;
	struct pthread_queue* list;
	int type, count;
	unsigned long owner;
};

typedef struct pthread_mutex pthread_mutex_t;

typedef int pthread_mutexattr_t;

struct pthread_attr_s
{
	int scope;
	int detachstate;
};

typedef struct pthread_attr_s pthread_attr_t;

typedef unsigned int pthread_key_t;

/* Conditional variables */

struct pthread_cond
{
	pthread_mutex_t mutex;
	int signal;
	struct pthread_queue* list;
};

typedef struct pthread_cond pthread_cond_t;

typedef unsigned int pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER	{0, 0, NULL, PTHREAD_MUTEX_NORMAL, 0, 0}
#define PTHREAD_MUTEX_NORMAL		0
#define PTHREAD_MUTEX_RECURSIVE		2

#define PTHREAD_CREATE_JOINABLE		0x1
#define PTHREAD_CREATE_DETACHED		0x2

#define PTHREAD_COND_INITIAL		0x1
#define PTHREAD_COND_SIGNAL			0x2

#define PTHREAD_COND_INITIALIZER	{0x1, NULL}
#define PTHREAD_COND_SIGNALLED		{0x2, NULL}

#define PTHREAD_STACK_MIN	8192

#define PTHREAD_SCOPE_SYSTEM	0x00000001
#define PTHREAD_SCOPE_PROCESS	0x00000002

/* Keys */

#define PTHREAD_KEYS_MAX		1024

/* Functions */

pthread_t pthread_self(void);

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

void pthread_cleanup_push(void (*routine)(void*),void* arg);
void pthread_cleanup_pop(int execute);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond,pthread_mutex_t* mutex);

/* Attributes */
int pthread_attr_init(pthread_attr_t* attr);
int pthread_attr_destroy(pthread_attr_t* attr);
int pthread_attr_setscope(pthread_attr_t* attr, int scope);
int pthread_attr_getstack(const pthread_attr_t* attr,void** stackAddr,size_t* stackSize);

static inline int pthread_equal(pthread_t thread1, pthread_t thread2)
{
	return (thread1 == thread2);
}

/* Cleanup functions */
struct _pthread_cleanup_buffer
{
	void (*routine)(void*);
	void* arg;
	int cancelType;
	struct _pthread_cleanup_buffer* prev;
};

void _pthread_cleanup_push(struct _pthread_cleanup_buffer* buffer, void (*routine)(void*), void* arg);

void _pthread_cleanup_pop(struct _pthread_cleanup_buffer* buffer, int execute);

#define pthread_cleanup_push(routine, arg) \
	{ struct _pthread_cleanup_buffer _buffer; \
		_pthread_cleanup_push(&_buffer, (routine), (arg));

#define pthread_cleanup_pop(execute) \
	_pthread_cleanup_pop(&_buffer, (execute)); }

#endif
