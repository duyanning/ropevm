#include "std.h"
#include "rope.h"

#include "thread.h"
#include "SpmtThread.h"


/*
在一个java线程内部，一个对象只属于一个spmt线程。
但是一个对象有可能被多个java线程访问。
在每个java线程中，该对象都属于其中一个spmt线程。
这样，一个对象就属于多个spmt线程。
对象中记录的spmt线程，是创建对象的java线程中。
其他的spmt线程，记录在各个java线程中。
 */
SpmtThread*
Object::get_st()
{

    assert(m_st); // 对象一产生，创建它的java线程便为其分配了spmt线程

    /*
      if 向对象get_st的java线程就是创建对象的java线程
          return 对象记录的spmt线程
    */
    if (m_st->get_thread() == g_get_current_thread()) {
        return m_st;
    }

    /*
      如果向对象get_st的java线程并不是创建对象的java线程，询问此java线程，对象由哪个spmt线程负责
     */
    Thread* thread = g_get_current_thread();
    SpmtThread* st = thread->spmt_thread_of(this);
    if (st)
        return st;

    // 如果没有对应的spmt线程，说明这是本java线程第一次访问这个对象，为其安排spmt线程
    //st = thread->assign_spmt_thread_for(this);
    st = thread->assign_spmt_thread_for(this);
    // assign_spmt_thread_for后要么跟set_st，要么跟join_st_in_other_threads
    // 现在的问题，哪些设置在assign_spmt_thread_for中进行？哪些在之后进行？
    // 设置 st 的java线程，设置与对象的关联，设置leader。
    join_st_in_other_threads(st);

    return st;
}


void
Object::set_st(SpmtThread* st)
{
    assert(m_st == 0);
    m_st = st;

    // spmt线程不记录它拥有哪些对象，因为它拥有的对象实在是太多了。
    //st->push_back(this);
}


void
Object::join_st_in_other_threads(SpmtThread* st)
{
    assert(m_st);

    Thread* thread = threadSelf();
    thread->register_object_spmt_thread(this, st);
}
