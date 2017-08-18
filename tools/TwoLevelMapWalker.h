#ifndef TWOLEVELMAPWALKER_H
#define TWOLEVELMAPWALKER_H

template <class ModuleEntry>
class TwoLevelMapWalker {
    string m_map_file_name;
    virtual void process(istringstream& iss, ModuleEntry& entry) = 0;
    virtual void process_module(vector<ModuleEntry>& nodes_in_module, int last_module_no) = 0;
public:
    TwoLevelMapWalker(const string& map_file_name);
    void go();
};




#endif // TWOLEVELMAPWALKER_H
