#include "std.h"

using namespace std;

#include "Story.h"
#include "Phase.h"

// linklib boost_system
// linklib boost_serialization

using namespace std;

intmax_t begin_op_of_story(const Story& story)
{
    return story.ops[0];
}

intmax_t end_op_of_story(const Story& story)
{
    intmax_t size = story.ops.size();
    return story.ops[size - 1];
}

int main()
{
    ifstream ifs_phase("phase2.txt");
    boost::archive::text_iarchive ia_phase(ifs_phase);

    ifstream ifs_story("story2.txt");
    boost::archive::text_iarchive ia_story(ifs_story);

    intmax_t phase_count;
    ia_phase >> phase_count;

    vector<Phase> phases;
    for (int i = 0; i < phase_count; ++i) {
        Phase p;
        ia_phase >> p;
        phases.push_back(p);
    }

    intmax_t story_count;
    ia_story >> story_count;

    vector<Story> stories;
    for (int i = 0; i < story_count; ++i) {
        Story s;
        ia_story >> s;
        stories.push_back(s);
    }

    cout << "story count: " << story_count << endl;
    cout << "phase count: " << phase_count << endl;

    // 找出每个阶段的范围(最小的事件编号、最大的事件编号)事件编号可能
    // 还不行，因为编号是动态序列中的编号，在源码中大编号并不一定位于
    // 小编号的后边。先做出来再看吧。光知道操作编号，却对应不到源码行
    // 数，意义不大。

    // 注意：infomap给出的故事和阶段编号都是从1开始的
    for (intmax_t i = 0; i < phase_count; i++) { // 遍历所有阶段
        cout << "phase " << i + 1 << ":" << endl;
        Phase& p = phases[i];
        
        intmax_t begin;
        begin = begin_op_of_story(stories[p.stories[0] - 1]);


        intmax_t end;
        end = end_op_of_story(stories[p.stories[0] - 1]);



        for (auto s : p.stories) { // 遍历每个阶段中的所有故事
            intmax_t b;
            //cout << "haha " << s << endl;
            b = begin_op_of_story(stories[s - 1]);
            //cout << "a" << endl;
            intmax_t e;
            e = end_op_of_story(stories[s - 1]);

            if (b < begin)
                begin = b;

            if (e < end)
                end = e;
            
        }

        cout << "[" << begin << ", " << end << "]" << endl;
    }

    // 至此，每个阶段的起止操作的编号已经知道了。
    // 现在拿着操作编号去哪里查对应的源码位置？
    // 从event.txt中查吧
    // 可以在event.txt中补充什么信息呢？
    // 栈帧对应方法的名字，操作对应方法名字(还可以加上操作的pc位置)
    
    return 0;
}
