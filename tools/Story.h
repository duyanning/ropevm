#ifndef STORY_H
#define STORY_H

// 第一次出现story的概念
// story.txt中的存在形式
struct Story {
    intmax_t no;                // story的编号
    vector<intmax_t> ops;       // story中包含的操作
};

// 从以story为节点的net中产生的map中的节点
// 以label的形式存在
struct StoryEntry {
    intmax_t no;                // story的编号
};


inline
std::ostream&
operator<<(std::ostream& strm, const Story& story)
{
    strm << "story " << story.no << ":" << endl;
    strm << "op count: " << story.ops.size() << endl;
    for (auto op : story.ops) {
        strm << op << " ";
    }
    strm << endl;
    
    return strm;
}


#endif // STORY_H
