#include "std.h"
#include "rope.h"
#include "Helper.h"

static int VM_initing = TRUE;
extern void initialisePlatform();


// 根据环境变量调整默认参数值
void
use_env_args(InitArgs *args)
{
    args->do_log = get_bool(getenv("log"), "1");
    args->do_probe = get_bool(getenv("probe"), "1");
    args->model = get_int(getenv("model"), "3");
    args->do_graph = get_bool(getenv("graph"), "1");
    args->do_event = get_bool(getenv("event"), "1");
    args->support_invoker_execute = get_bool(getenv("support_invoker_execute"), "1");
    args->support_irrevocable = get_bool(getenv("support_irrevocable"), "1");
    args->support_spec_safe_native = get_bool(getenv("support_spec_safe_native"), "1");
    args->support_spec_barrier = get_bool(getenv("support_spec_barrier"), "1");
    args->support_self_read = get_bool(getenv("support_self_read"), "1");
}

/* Setup default values for command line args */
// 设置默认的参数值，并根据环境变量，调整这些参数值
// 注意：这些参数值后边还会受命令行选项控制，当然，不在这里，而在其他函数中
void setDefaultInitArgs(InitArgs *args) {
    args->do_log = FALSE;
    args->do_probe = TRUE;
    args->model = 3;
    args->do_graph = FALSE;
    args->do_event= FALSE;
    args->support_invoker_execute = true;
    args->support_irrevocable = true;
    args->support_spec_safe_native = true;
    args->support_spec_barrier = true;
    args->support_self_read = true;

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
    //initialiseLoggers(args);
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
