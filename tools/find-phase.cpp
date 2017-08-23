#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"
#include "Story.h"
#include "Phase.h"


class StoryMapWalker : public TwoLevelMapWalker<StoryEntry> {
    void on_module_count(intmax_t module_count) override;
    void read_node(istringstream& iss, NodeType& story) override;
    void process_module(ModuleType& nodes_in_module, int& module_no) override;
public:
    StoryMapWalker(const string& map_file_name);
};
 

StoryMapWalker::StoryMapWalker(const string& map_file_name)
:
    TwoLevelMapWalker(map_file_name)
{
}


void
StoryMapWalker::on_module_count(intmax_t module_count)
{
    cout << "phase count: " << module_count << endl;
}


void
StoryMapWalker::read_node(istringstream& iss, NodeType& story)
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
StoryMapWalker::process_module(ModuleType& module, int& module_no)
{

    sort(module.begin(), module.end(), [] (const OpEntry& op1, const OpEntry& op2) {
            return op1.op_no < op2.op_no;
        });
    //cout << i << endl;
    cout << "story " << module_no << ": " << module.front().op_no << "-" << module.back().op_no << endl;
                    
    // ofs_story << "story " << module_no << ":" << endl;
    // ofs_story << "op count: " << module.size() << endl;
    // for (auto op : module) {
    //     ofs_story << op.op_no << " ";
    // }
    // ofs_story << endl;
}


int main()
{
    StoryMapWalker mapWalker("story.map");
    mapWalker.go();

    return 0;
}
