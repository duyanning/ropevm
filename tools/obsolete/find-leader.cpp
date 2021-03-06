#include "std.h" // precompile std.h

using namespace std;

struct TimelineEntry {
    string class_name;
    string object_addr;
};

int main()
{
    ifstream ifs_timeline("timeline.txt");
    vector<TimelineEntry> timeline;
    while (ifs_timeline) {
        string line;
        getline(ifs_timeline, line);
        if (line.empty()) break;
        istringstream iss(line);
        TimelineEntry entry;
        iss >> entry.class_name >> entry.object_addr;
        timeline.push_back(entry);
    };
    cout << "timeline count:" << timeline.size() << endl;
    //cout << "last entry: " << timeline[timeline.size() - 1].class_name << endl;

    ifstream ifs_map("graph.map");
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
            cout << module_count << endl;
        }
        else if (tag == "*Nodes") {
            iss >> node_count;
            cout << node_count << endl;
            int last_module_no = 1;
            vector<TimelineEntry> nodes_in_module;
            for (int i = 0; i < node_count; ++i) {
                if (not sth_in_line)
                    getline(ifs_map, line);
                sth_in_line = false;
                istringstream iss(line);
                iss >> module_no;
                iss.ignore(); // :
                iss >> node_no;
                iss.ignore(2); // space and "
                string class_name;
                iss >> class_name;
                string object_addr;
                iss >> object_addr;
                object_addr.pop_back(); // "
                //cout << module_no << "-" << node_no << class_name << object_addr << endl;
                
                TimelineEntry e;
                e.class_name = class_name;
                e.object_addr = object_addr;
                nodes_in_module.push_back(e);

                // 如果新的module开始了，或者后边已经没有别的module了，就进行统计
                if (module_no != last_module_no || i == node_count - 1) {
                    if (module_no != last_module_no) {
                        nodes_in_module.pop_back();
                        i--;
                        sth_in_line = true;
                    }
                    // 根据timeline找到nodes_in_module中最老的node
                    for (int i = 0; i < timeline.size(); ++i) {
                        TimelineEntry e1 = timeline[i];
                        bool found = false;
                        for (int n = 0; n < nodes_in_module.size(); ++n) {
                            TimelineEntry e2 = nodes_in_module[n];
                            if (e1.object_addr == e2.object_addr) {
                                //cout << e
                                found = true;
                                break;
                            }
                        }
                        if (found) {
                            cout << last_module_no << " " << e1.class_name << " " << e1.object_addr << endl;
                            break;
                        }
                    }

                    //cout << module_no << endl;
                    last_module_no = module_no;
                    nodes_in_module.clear();
                }

                
            }
            
        }
    }
    return 0;
}
