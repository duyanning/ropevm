#ifndef PHASE_H
#define PHASE_H

struct Phase {
    intmax_t no;                // phase的编号
    vector<intmax_t> stories;       // phase中包含的story
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & no;
        ar & stories;
    }
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
