#ifndef STATISTIC_H
#define STATISTIC_H

class Mode;

class Statistic {
public:
    Statistic();

    void probe_instr_exec(int opcode);
    void report();

    static Statistic* instance();
private:
    static Statistic* m_instance;
    long long m_certain_instr_count;
};

#endif // STATISTIC_H
