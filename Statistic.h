#ifndef STATISTIC_H
#define STATISTIC_H

class Mode;

class Statistic {
public:
    Statistic();

    void probe_instr_exec(int opcode);
    void report_stat(std::ostream& os);

    void turn_on_statistic();
    void turn_off_statistic();


    static Statistic* instance();
private:
    bool m_enabled;
    static Statistic* m_instance;
    long long m_certain_instr_count;

};

#endif // STATISTIC_H
