#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"  // usingcpp



struct OpEntry {
    intmax_t op_no;
    //string from_class_name;
    string from_object_addr;
    //string to_class_name;
    string to_object_addr;
};

class EventMapWalker : public TwoLevelMapWalker<OpEntry> {
    ofstream ofs_story;
public:
    EventMapWalker(const string& map_file_name, const string& story_file_name);
    void process(istringstream& iss, OpEntry& op) override;
    void process_module(vector<OpEntry>& nodes_in_module, int last_module_no) override;
};

EventMapWalker::EventMapWalker(const string& map_file_name, const string& story_file_name)
:
    TwoLevelMapWalker(map_file_name),
    ofs_story(story_file_name)
{
}

void
EventMapWalker::process(istringstream& iss, OpEntry& op)
{
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
    op.op_no = op_no;
    op.from_object_addr = from_object_addr;
    op.to_object_addr = to_object_addr;

}

void
EventMapWalker::process_module(vector<OpEntry>& nodes_in_module, int last_module_no)
{

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
}

int main()
{
    EventMapWalker mapWalker("event.map", "story2.txt");
    mapWalker.go();

    return 0;
}
