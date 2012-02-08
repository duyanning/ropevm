#ifndef DEBUGSCAFFOLD_H
#define DEBUGSCAFFOLD_H


extern bool enable_debug;

//#define NO_DEBUG

#ifndef NO_DEBUG
#define DEBUG_DECL(decl) decl
#define DEBUG_CODE(code) if (enable_debug) { code }
#else
#define DEBUG_DECL(decl)
#define DEBUG_CODE(code) {}
#endif

extern bool java_main_arrived;

#endif // DEBUGSCAFFOLD_H
