#include "std.h" // precompile std.h

using namespace std;

// struct TimelineEntry {
//     string class_name;
//     string object_addr;
// };

struct OpEntry {
    intmax_t op_no;
    //string from_class_name;
    string from_object_addr;
    //string to_class_name;
    string to_object_addr;
};

int main()
{
    // ifstream ifs_timeline("timeline.txt");
    // vector<TimelineEntry> timeline;
    // while (ifs_timeline) {
    //     string line;
    //     getline(ifs_timeline, line);
    //     if (line.empty()) break;
    //     istringstream iss(line);
    //     TimelineEntry entry;
    //     iss >> entry.class_name >> entry.object_addr;
    //     timeline.push_back(entry);
    // };
    // cout << "timeline count:" << timeline.size() << endl;

    ofstream ofs_story("story.txt");

    ifstream ifs_map("event.map");
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
            cout << "storys count: " << module_count << endl;
            ofs_story << "storys count: " << module_count << endl;
        }
        else if (tag == "*Nodes") {
            iss >> node_count;
            //cout << node_count << endl;
            int last_module_no = 1;
            //vector<TimelineEntry> nodes_in_module;
            vector<OpEntry> nodes_in_module;
            for (int i = 0; i < node_count; ++i) {
                if (not sth_in_line)
                    getline(ifs_map, line);
                sth_in_line = false;
                istringstream iss(line);
                iss >> module_no;
                iss.ignore(); // :
                iss >> node_no;
                iss.ignore(2); // space and "
                intmax_t op_no;
                iss >> op_no;
                string from_object_addr;
                iss >> from_object_addr;
                string arrow;
                iss >> arrow;
                string to_object_addr;
                iss >> to_object_addr;
                to_object_addr.pop_back(); // "
                //cout << module_no << "-" << op_no << endl;
                
                OpEntry op;
                op.op_no = op_no;
                op.from_object_addr = from_object_addr;
                op.to_object_addr = to_object_addr;
                nodes_in_module.push_back(op);

                // 如果新的module开始了，或者后边已经没有别的module了，就进行统计
                if (module_no != last_module_no || i == node_count - 1) {
                    if (module_no != last_module_no) {
                        nodes_in_module.pop_back();
                        i--;
                        sth_in_line = true;
                    }

                    sort(nodes_in_module.begin(), nodes_in_module.end(), [] (const OpEntry& op1, const OpEntry& op2) {
                            return op1.op_no < op2.op_no;
                        });
                    //cout << i << endl;
                    cout << "story " << last_module_no << ": " << nodes_in_module.front().op_no << "-" << nodes_in_module.back().op_no << endl;
                    
                    ofs_story << "story " << last_module_no << ":" << endl;
                    ofs_story << "op count: " << nodes_in_module.size() << endl;
                    for (auto op : nodes_in_module) {
                        ofs_story << op.op_no << " ";
                    }
                    ofs_story << endl;

                    // // 根据timeline找到nodes_in_module中最老的node
                    // for (int i = 0; i < timeline.size(); ++i) {
                    //     TimelineEntry e1 = timeline[i];
                    //     bool found = false;
                    //     for (int n = 0; n < nodes_in_module.size(); ++n) {
                    //         TimelineEntry e2 = nodes_in_module[n];
                    //         if (e1.object_addr == e2.object_addr) {
                    //             //cout << e
                    //             found = true;
                    //             break;
                    //         }
                    //     }
                    //     if (found) {
                    //         cout << last_module_no << " " << e1.class_name << " " << e1.object_addr << endl;
                    //         break;
                    //     }
                    // }

                    //cout << module_no << endl;
                    last_module_no = module_no;
                    nodes_in_module.clear();
                }

                
            }
            
        }
    }
    return 0;
}
