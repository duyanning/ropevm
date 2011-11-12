#ifndef CREATING
#include <pthread.h>
#include <setjmp.h>
#include <stdlib.h>

/* Thread states */

#define CREATING      0
#define STARTED       1
#define RUNNING       2
#define WAITING       3
#define TIMED_WAITING 4
#define BLOCKED       5
#define SUSPENDED     6

/* thread priorities */

#define MIN_PRIORITY   1
#define NORM_PRIORITY  5
#define MAX_PRIORITY  10

/* Enable/Disable suspend modes */

#define SUSP_BLOCKING 1
#define SUSP_CRITICAL 2

class Thread;

typedef struct monitor {
    pthread_mutex_t lock;
    Thread *owner;
    Object *obj;
    int count;
    int in_wait;
    uintptr_t entering;
    int wait_count;
    Thread *wait_set;
    struct monitor *next;
} Monitor;

class SpmtThread;


// 代表一个java线程
class Thread {
    friend class SpmtThread;
public:
    Object* thread;
    int id;
    pthread_t tid;
    char state;
    char suspend;
    char blocking;
    char interrupted;
    char interrupting;
    //ExecEnv *ee;
    Object* exception;

    void *stack_top;
    void *stack_base;
    Monitor *wait_mon;
    Monitor *blocked_mon;
    Thread *wait_prev;
    Thread *wait_next;
    pthread_cond_t wait_cv;
    long long blocked_count;
    long long waited_count;
    Thread *prev, *next;
    unsigned int wait_id;
    unsigned int notify_id;

    // 一个java线程的背后是若干spmt线程。
    // java线程仅仅是一个概念上的东西，每个spmt线程对应一个os线程。
    // 但目前，简单起见，java线程对应一个os线程，众spmt线程由该os线程驱动。
    std::vector<SpmtThread*> m_spmt_threads;

    SpmtThread* m_initial_spmt_thread; // 每个java线程一产生，就有一个spmt线程。

    // 因为多个java线程可能都会访问同一个对象，所以一个对象不再属于一个
    // spmt线程，而是属于几个spmt线程，而这些spmt线程分属于不同的java线
    // 程。对象中记录的spmt线程是创建对象的java线程中的spmt线程，其他
    // spmt线程，记录在各自的java线程中。此处即为此从对象到spmt线程的映
    // 射表。
    typedef std::map<Object*, SpmtThread*> Object2SpmtThreadMap;
    Object2SpmtThreadMap m_object_to_spmt_thread;
    void register_object_spmt_thread(Object* object, SpmtThread* spmt_thread);
    SpmtThread* spmt_thread_of(Object* obj);

    SpmtThread* assign_spmt_thread_for(Object* obj); // 为对象分配spmt线程
    Thread();
    ~Thread();
    bool create();


    SpmtThread* get_initial_spmt_thread();
    SpmtThread* get_current_spmt_thread();

    // 垃圾回收（尚未实现）
    void scan();
    void scan_spmt_threads();

private:
    SpmtThread* m_current_spmt_thread;
};

Thread* g_get_current_thread();

class VMThread : public Thread {
public:
    virtual void beforeBegin();
    virtual void afterEnd();
};

extern Thread *threadSelf();
extern Thread *threadSelf0(Object *jThread);

extern void *getStackTop(Thread *thread);
extern void *getStackBase(Thread *thread);

extern int getThreadsCount();
extern int getPeakThreadsCount();
extern void resetPeakThreadsCount();
extern long long getTotalStartedThreadsCount();

extern void threadInterrupt(Thread *thread);
extern void threadSleep(Thread *thread, long long ms, int ns);
extern void threadYield(Thread *thread);

extern int threadIsAlive(Thread *thread);
extern int threadInterrupted(Thread *thread);
extern int threadIsInterrupted(Thread *thread);
extern int systemIdle(Thread *self);

extern void suspendAllThreads(Thread *thread);
extern void resumeAllThreads(Thread *thread);

extern void createVMThread(const char *name, void (*start)(Thread*));

extern void disableSuspend0(Thread *thread, void *stack_top);
extern void enableSuspend(Thread *thread);
extern void fastEnableSuspend(Thread *thread);

extern Thread *attachJNIThread(char *name, char is_daemon, Object *group);
extern void detachJNIThread(Thread *thread);

extern const char* getThreadStateString(Thread *thread);

extern Thread *findThreadById(long long id);
extern void suspendThread(Thread *thread);
extern void resumeThread(Thread *thread);

#define disableSuspend(thread)          \
{                                       \
    sigjmp_buf *env;                    \
    env = (sigjmp_buf*)alloca(sizeof(sigjmp_buf)); \
    sigsetjmp(*env, FALSE);             \
    disableSuspend0(thread, (void*)env);\
}

#define fastDisableSuspend(thread)      \
{                                       \
    thread->blocking = SUSP_CRITICAL;   \
    MBARRIER();                         \
}

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cv;
} VMWaitLock;

typedef pthread_mutex_t VMLock;

#define initVMLock(lock) pthread_mutex_init(&lock, NULL)
#define initVMWaitLock(wait_lock) {            \
    pthread_mutex_init(&wait_lock.lock, NULL); \
    pthread_cond_init(&wait_lock.cv, NULL);    \
}

#define lockVMLock(lock, self) { \
    self->state = BLOCKED;       \
    pthread_mutex_lock(&lock);   \
    self->state = RUNNING;       \
}

#define tryLockVMLock(lock, self) \
    (pthread_mutex_trylock(&lock) == 0)

#define unlockVMLock(lock, self) if(self) pthread_mutex_unlock(&lock)

#define lockVMWaitLock(wait_lock, self) lockVMLock(wait_lock.lock, self)
#define unlockVMWaitLock(wait_lock, self) unlockVMLock(wait_lock.lock, self)

#define waitVMWaitLock(wait_lock, self) {                        \
    self->state = WAITING;                                       \
    pthread_cond_wait(&wait_lock.cv, &wait_lock.lock);           \
    self->state = RUNNING;                                       \
}

#define timedWaitVMWaitLock(wait_lock, self, ms) {               \
    struct timeval tv;                                           \
    struct timespec ts;                                          \
    gettimeofday(&tv, 0);                                        \
    ts.tv_sec = tv.tv_sec + ms/1000;                             \
    ts.tv_nsec = (tv.tv_usec + ((ms%1000)*1000))*1000;           \
    if(ts.tv_nsec > 999999999L) {                                \
        ts.tv_sec++;                                             \
        ts.tv_nsec -= 1000000000L;                               \
    }                                                            \
    self->state = TIMED_WAITING;                                 \
    pthread_cond_timedwait(&wait_lock.cv, &wait_lock.lock, &ts); \
    self->state = RUNNING;                                       \
}

#define notifyVMWaitLock(wait_lock, self) pthread_cond_signal(&wait_lock.cv)
#define notifyAllVMWaitLock(wait_lock, self) pthread_cond_broadcast(&wait_lock.cv)
#endif

