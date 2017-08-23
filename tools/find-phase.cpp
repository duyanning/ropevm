#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"


struct StoryEntry {
    intmax_t op_no;
    //string from_class_name;
    string from_object_addr;
    //string to_class_name;
    string to_object_addr;
};


class StoryMapWalker : public TwoLevelMapWalker<StoryEntry> {
    void on_module_count(intmax_t module_count) override;
    void read_node(istringstream& iss, StoryEntry& story) override;
    void process_module(vector<StoryEntry>& nodes_in_module, int& module_no) override;
public:
    StoryMapWalker(const string& map_file_name);
};
 

StroyMapWalker::StroyMapWalker(const string& map_file_name)
:
    TwoLevelMapWalker(map_file_name)
{
}


void
StroyMapWalker::on_module_count(intmax_t module_count)
{
    cout << "phase count: " << module_count << endl;
}


void
StroyMapWalker::read_node(istringstream& iss, StoryEntry& story)
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
    //cout << op_no << endl;


    op.op_no = op_no;
    op.from_object_addr = from_object_addr;
    op.to_object_addr = to_object_addr;

}


void
StroyMapWalker::process_module(vector<OpEntry>& nodes_in_module, int& module_no)
{

    sort(nodes_in_module.begin(), nodes_in_module.end(), [] (const OpEntry& op1, const OpEntry& op2) {
            return op1.op_no < op2.op_no;
        });
    //cout << i << endl;
    cout << "story " << module_no << ": " << nodes_in_module.front().op_no << "-" << nodes_in_module.back().op_no << endl;
                    
    // ofs_story << "story " << module_no << ":" << endl;
    // ofs_story << "op count: " << nodes_in_module.size() << endl;
    // for (auto op : nodes_in_module) {
    //     ofs_story << op.op_no << " ";
    // }
    // ofs_story << endl;
}


int main()
{
    StroyMapWalker mapWalker("story.map");
    mapWalker.go();

    return 0;
}
