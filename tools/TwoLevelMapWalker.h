#ifndef TWOLEVELMAPWALKER_H
#define TWOLEVELMAPWALKER_H

template <class ModuleEntry>
class TwoLevelMapWalker {
    string m_map_file_name;
    virtual void on_module_count(intmax_t module_count);
    virtual void on_node_count(intmax_t node_count);
    virtual void process(istringstream& iss, ModuleEntry& entry) = 0;
    virtual void process_module(vector<ModuleEntry>& nodes_in_module, int& module_no) = 0;
public:
    TwoLevelMapWalker(const string& map_file_name);
    void go();
};


template <class ModuleEntry>
void
TwoLevelMapWalker<ModuleEntry>::on_module_count(intmax_t module_count)
{
    cout << "module count: " << module_count << endl;
}


template <class ModuleEntry>
void
TwoLevelMapWalker<ModuleEntry>::on_node_count(intmax_t node_count)
{
}


template <class ModuleEntry>
TwoLevelMapWalker<ModuleEntry>::TwoLevelMapWalker(const string& map_file_name)
:
    m_map_file_name(map_file_name)
{
}


template <class ModuleEntry>
void
TwoLevelMapWalker<ModuleEntry>::go()
{
    ifstream ifs_map(m_map_file_name);

    string line;
    bool sth_in_line;           // 因为某些特殊情况下，读出的东西又会被退回，这个标志为true，表示line中还有未处理（即退回）的东西
    int module_count;
    int node_count;
    int module_no;
    int node_no;
    string tag;
    sth_in_line = false;
    while (ifs_map) {
        getline(ifs_map, line);
        istringstream iss(line);
        iss >> tag;
        if (tag == "*Modules") {
            iss >> module_count;
            on_module_count(module_count);
        }
        else if (tag == "*Nodes") {
            iss >> node_count;
            on_node_count(node_count);
            int last_module_no = 1;
            vector<ModuleEntry> nodes_in_module;
            for (int i = 0; i < node_count; ++i) {
                if (not sth_in_line)
                    getline(ifs_map, line);
                sth_in_line = false;
                istringstream iss(line);
                iss >> module_no;
                iss.ignore(); // :
                iss >> node_no;
                iss.ignore(2); // space and "

                ModuleEntry e;
                process(iss, e);
                nodes_in_module.push_back(e);

                // 如果新的module开始了，或者后边已经没有别的module了，就进行统计
                if (module_no != last_module_no || i == node_count - 1) {
                    if (module_no != last_module_no) {
                        //cout << "***" << endl;
                        nodes_in_module.pop_back();
                        i--;
                        sth_in_line = true;
                    }

                    process_module(nodes_in_module, last_module_no);

                    //cout << module_no << endl;
                    last_module_no = module_no;
                    nodes_in_module.clear();
                }
            }
        }
    }
}


#endif // TWOLEVELMAPWALKER_H
