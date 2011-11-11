#include "std.h"
#include "rope.h"
#include "Helper.h"

static int VM_initing = TRUE;
extern void initialisePlatform();


void
use_env_args(InitArgs *args)
{
    args->do_log = get_bool(getenv("log"), "1");
    //args->do_spec = get_bool(getenv("spec"), "1");
    args->model = get_int(getenv("model"), "3");
}

/* Setup default values for command line args */
void setDefaultInitArgs(InitArgs *args) {
    args->do_log = FALSE;
    //args->do_spec = FALSE;
    args->model = 3;

    args->noasyncgc = FALSE;

    args->verbosegc    = FALSE;
    args->verbosedll   = FALSE;
    args->verboseclass = FALSE;

    args->compact_specified = FALSE;

    args->classpath = NULL;
    args->bootpath  = NULL;

    args->java_stack = DEFAULT_STACK;
    args->min_heap   = DEFAULT_MIN_HEAP;
    args->max_heap   = DEFAULT_MAX_HEAP;

    args->props_count = 0;

    args->vfprintf = vfprintf;
    args->abort    = abort;
    args->exit     = exit;

#ifdef INLINING
    args->replication = 10;
    args->codemem = args->max_heap / 4;
#endif

    use_env_args(args);
}

int VMInitialising() {
    return VM_initing;
}

void initVM(InitArgs *args) {
    initialiseLoggers(args);
    initialiseJvm(args);
    /* Perform platform dependent initialisation */
    initialisePlatform();

    /* Initialise the VM modules -- ordering is important! */
    initialiseHooks(args);
    initialiseProperties(args);
    initialiseAlloc(args);
    initialiseDll(args);
    initialiseUtf8();
    initialiseThreadStage1(args);
    initialiseSymbol();
    initialiseClass(args);
    initialiseMonitor();
    initialiseString();
    initialiseException();
    initialiseNatives();
    initialiseJNI();
    initialiseInterpreter(args);
    initialiseThreadStage2(args);
    initialiseGC(args);

    VM_initing = FALSE;
}

unsigned long parseMemValue(char *str) {
    char *end;
    unsigned long n = strtol(str, &end, 0);

    switch(end[0]) {
        case '\0':
            break;

        case 'M':
        case 'm':
            n *= MB;
            break;

        case 'K':
        case 'k':
            n *= KB;
            break;

        default:
             n = 0;
    }

    return n;
}
