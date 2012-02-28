#include "std.h"
#include "Statistic.h"
#include "rope.h"
#include "Mode.h"
#include "Helper.h"

using namespace std;

Statistic* Statistic::m_instance = 0;

Statistic*
Statistic::instance()
{
    if (m_instance == 0) {
        m_instance = new Statistic;
    }

    return m_instance;
}

Statistic::Statistic()
:
    m_enabled(true)
{
}

void
Statistic::report_stat(ostream& os)
{
    os << "\n";
    os << "statistic:\n";
    //os << "cycles: " << m_certain_instr_count << endl;
}

void
Statistic::probe_instr_exec(int opcode)
{
    if (not m_enabled)
        return;

    //m_certain_instr_count++;
}



void
Statistic::turn_on_statistic()
{
    m_enabled = true;
}


void
Statistic::turn_off_statistic()
{
    m_enabled = false;
}


// 只统计程序本身的代码，而不统计那庞大但未经并行化的标准库代码
bool
g_should_enable_stat(MethodBlock* mb)
{
    if (not java_main_arrived)
        return false;

    // if (not is_app_obj(mb->classobj))
    //     return false;

    return true;
}
