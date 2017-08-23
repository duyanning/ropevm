#ifndef PHASE_H
#define PHASE_H

struct Phase {
    intmax_t no;                // phase的编号
    vector<intmax_t> stories;       // phase中包含的story
};


inline
std::ostream&
operator<<(std::ostream& strm, const Phase& phase)
{
    strm << "phase " << phase.no << ":" << endl;
    strm << "story count: " << phase.stories.size() << endl;
    for (auto story : phase.stories) {
        strm << story << " ";
    }
    strm << endl;
    
    return strm;
}

#endif // PHASE_H
