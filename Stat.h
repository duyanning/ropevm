#ifndef STAT_H
#define STAT_H

extern bool enable_stat;

//#define NO_STAT

#ifndef NO_STAT
#define STAT_DECL(decl) decl
#define STAT_CODE(code) if (enable_stat) { code }
#else
#define STAT_DECL(decl)
#define STAT_CODE(code) {}
#endif


#endif // STAT_H
