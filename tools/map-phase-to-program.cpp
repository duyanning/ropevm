#include "std.h"

using namespace std;

#include "Story.h"
#include "Phase.h"

// linklib boost_system
// linklib boost_serialization

using namespace std;

int main()
{
    ifstream ifs_phase("phase2.txt");
    boost::archive::text_iarchive ia_phase(ifs_phase);

    ifstream ifs_story("story2.txt");
    boost::archive::text_iarchive ia_story(ifs_story);

    intmax_t phase_count;
    ia_phase >> phase_count;

    intmax_t story_count;
    ia_story >> story_count;

    cout << phase_count << endl;
    cout << story_count << endl;
    return 0;
}
