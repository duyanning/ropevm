#include "std.h"
#include "Statistic.h"
#include "jam.h"
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
    m_certain_instr_count(0)
{
}

void
Statistic::report()
{
    cout << "\n";
    cout << "statistic:\n";
    cout << "(certain mode)instructions: " << m_certain_instr_count << endl;
}

void
Statistic::probe_instr_exec(int opcode)
{
    m_certain_instr_count++;
}
