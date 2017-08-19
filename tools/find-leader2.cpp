#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"


struct TimelineEntry {
    string class_name;
    string object_addr;
};

vector<TimelineEntry> timeline;

class ObjMapWalker : public TwoLevelMapWalker<TimelineEntry> {
    void on_module_count(intmax_t module_count) override;
    void on_node_count(intmax_t node_count) override;
    void process(istringstream& iss, TimelineEntry& op) override;
    void process_module(vector<TimelineEntry>& nodes_in_module, int& module_no) override;
public:
    ObjMapWalker(const string& map_file_name);
};


 

ObjMapWalker::ObjMapWalker(const string& map_file_name)
:
    TwoLevelMapWalker(map_file_name)
{
}


void
ObjMapWalker::on_module_count(intmax_t module_count)
{
    cout << module_count << endl;
}


void
ObjMapWalker::on_node_count(intmax_t node_count)
{
    cout << node_count << endl;
}


void
ObjMapWalker::process(istringstream& iss, TimelineEntry& e)
{
    string class_name;
    iss >> class_name;
    string object_addr;
    iss >> object_addr;
    object_addr.pop_back(); // "
    //cout << module_no << "-" << node_no << class_name << object_addr << endl;
                
    e.class_name = class_name;
    e.object_addr = object_addr;


}


void
ObjMapWalker::process_module(vector<TimelineEntry>& nodes_in_module, int& module_no)
{
    // 根据timeline找到nodes_in_module中最老的node
    for (size_t i = 0; i < timeline.size(); ++i) {
        TimelineEntry e1 = timeline[i];
        bool found = false;
        for (size_t n = 0; n < nodes_in_module.size(); ++n) {
            TimelineEntry e2 = nodes_in_module[n];
            if (e1.object_addr == e2.object_addr) {
                //cout << e
                found = true;
                break;
            }
        }
        if (found) {
            cout << module_no << " " << e1.class_name << " " << e1.object_addr << endl;
            break;
        }
    }

}


int main()
{
    ifstream ifs_timeline("timeline.txt");

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
 
    ObjMapWalker mapWalker("graph.map");
    mapWalker.go();

    return 0;
}

