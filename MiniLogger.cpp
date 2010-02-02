#include "std.h"
#include "MiniLogger.h"

using namespace std;

MiniLogger MiniLogger::default_logger(cout, true);
bool MiniLogger::disable_all_loggers = true;
