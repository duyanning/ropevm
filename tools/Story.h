#ifndef STORY_H
#define STORY_H

// 以完整形式存在
struct Story {
    intmax_t no;                // story的编号
    vector<intmax_t> ops;       // story中包含的操作
};

// 以infomap节点标签的形式存在于.net或.map文件中
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


inline
std::istream&
operator>>(std::istream& strm, StoryEntry& story)
{
    strm >> story.no;
    return strm;
}

#endif // STORY_H
