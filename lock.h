#include "thread.h"

extern void monitorInit(Monitor *mon);
extern void monitorLock(Monitor *mon, Thread *self);
extern void monitorUnlock(Monitor *mon, Thread *self);
extern int monitorWait0(Monitor *mon, Thread *self, long long ms, int ns,
                        int blocked, int interruptible);
extern int monitorNotify(Monitor *mon, Thread *self);
extern int monitorNotifyAll(Monitor *mon, Thread *self);

#define monitorWait(mon, self, ms, ns) \
    monitorWait0(mon, self, ms, ns, FALSE, TRUE)

extern void objectLock(Object *ob);
extern void objectUnlock(Object *ob);
extern void objectNotify(Object *ob);
extern void objectNotifyAll(Object *ob);
extern void objectWait0(Object *ob, long long ms, int ns, int interruptible);
extern int objectLockedByCurrent(Object *ob);
extern void threadMonitorCache();

#define objectWait(ob, ms, ns) \
    objectWait0(ob, ms, ns, TRUE)
