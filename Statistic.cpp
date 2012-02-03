#include "std.h"
#include "Statistic.h"
#include "rope.h"
#include "Mode.h"

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
