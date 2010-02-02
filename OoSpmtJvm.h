#ifndef OOSPMTJVM_H
#define OOSPMTJVM_H

#include "Core.h"

class Object;

class OoSpmtJvm
{
 public:
    //    ~OoSpmtJvm();
    Core* alloc_core();
    Core* alloc_core_for_object(Object* object);
    Core* core_for_object(Object* object);
    static OoSpmtJvm* instance();
    static bool do_spec;

 protected:
    OoSpmtJvm();
    int get_next_core_id() { return m_next_core_id++; }

 private:
	std::vector<Core*> m_cores;
    Core* m_currentCore;
    pthread_mutex_t m_lock;

    static OoSpmtJvm* m_instance;
    static int m_next_core_id;
};

#endif
