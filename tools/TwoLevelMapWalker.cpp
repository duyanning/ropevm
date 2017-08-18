#include "std.h"

using namespace std;

#include "TwoLevelMapWalker.h"

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
            // slot
        }
        else if (tag == "*Nodes") {
            iss >> node_count;
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
                        nodes_in_module.pop_back();
                        i--;
                        sth_in_line = true;
                    }

                    process_module(nodes_in_module, last_module_no);

                }

                //cout << module_no << endl;
                last_module_no = module_no;
                nodes_in_module.clear();
            }

                
        }
            
    }
}


