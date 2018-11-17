#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"
#include "Op.h"
#include "Story.h"

// linklib boost_system
// linklib boost_serialization

class EventMapWalker : public TwoLevelMapWalker<OpEntry> {
    ofstream ofs_story;
    ofstream ofs_story2;        // 用于boost::serialization
    boost::archive::text_oarchive oa;
    void on_module_count(intmax_t module_count) override;
    void read_node(istringstream& iss, NodeType& op) override;
    void process_module(ModuleType& module, int& module_no) override;
public:
    EventMapWalker(const string& map_file_name, const string& story_file_name);
};
 

EventMapWalker::EventMapWalker(const string& map_file_name, const string& story_file_name)
:
    TwoLevelMapWalker(map_file_name),
    ofs_story(story_file_name),
    ofs_story2("story2.txt"),
    oa(ofs_story2)
{
}


void
EventMapWalker::on_module_count(intmax_t module_count)
{
    cout << "story count: " << module_count << endl;
    ofs_story << "story count: " << module_count << endl;
    oa << module_count;
}


void
EventMapWalker::read_node(istringstream& iss, NodeType& op)
{
    iss >> op;
}


void
EventMapWalker::process_module(ModuleType& module, int& module_no)
{

    sort(module.begin(), module.end(), [] (const OpEntry& op1, const OpEntry& op2) {
            return op1.op_no < op2.op_no;
        });
    //cout << i << endl;
    cout << "story " << module_no << ": " << module.front().op_no << "-" << module.back().op_no << endl;
                    
////////////////////////
    
    Story story;
    story.no = module_no;
    for (auto op : module) {
        story.ops.push_back(op.op_no);
    }
//////////////////////////

    ofs_story << story;
    oa << story;
}


int main()
{
    EventMapWalker mapWalker("event.map", "story.txt");
    mapWalker.go();

    return 0;
}
