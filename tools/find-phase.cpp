// 输出story.map中的社区信息(即阶段)
// 此前，infomap会对故事网络(即从story.txt而来的story.net)进行社区发现，得到阶段(story.map)

#include "std.h" // precompile std.h

using namespace std;

#include "TwoLevelMapWalker.h"
#include "Story.h"
#include "Phase.h"

// linklib boost_system
// linklib boost_serialization

class StoryMapWalker : public TwoLevelMapWalker<StoryEntry> {
    ofstream ofs_phase;
    ofstream ofs_phase2;        // 用于boost::serialization
    boost::archive::text_oarchive oa;
    void on_module_count(intmax_t module_count) override;
    void read_node(istringstream& iss, NodeType& story) override;
    void process_module(ModuleType& nodes_in_module, int& module_no) override;
public:
    StoryMapWalker(const string& map_file_name, const string& phase_file_name);
};
 

StoryMapWalker::StoryMapWalker(const string& map_file_name, const string& phase_file_name)
:
    TwoLevelMapWalker(map_file_name),
    ofs_phase(phase_file_name),
    ofs_phase2("phase2.txt"),
    oa(ofs_phase2)
{
}


void
StoryMapWalker::on_module_count(intmax_t module_count)
{
    cout << "phase count: " << module_count << endl;
    ofs_phase << "phase count: " << module_count << endl;
    oa << module_count;
}


void
StoryMapWalker::read_node(istringstream& iss, NodeType& story)
{
    iss >> story;
}


void
StoryMapWalker::process_module(ModuleType& module, int& module_no)
{

    sort(module.begin(), module.end(), [] (const StoryEntry& op1, const StoryEntry& op2) {
            return op1.no < op2.no;
        });

    //cout << "phase " << module_no << ": " << endl;

    Phase phase;
    phase.no = module_no;
    for (auto story : module) {
        phase.stories.push_back(story.no);
    }

    ofs_phase << phase;
    oa << phase;
}


int main()
{
    StoryMapWalker mapWalker("story.map", "phase.txt");
    mapWalker.go();

    return 0;
}
