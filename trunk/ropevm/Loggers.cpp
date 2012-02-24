#include "std.h"
#include "Loggers.h"
#include "rope.h"
#include "RopeVM.h"

using namespace std;

bool p = true;           // 控制是否输出指针值，在对比日志的时候输出指针的话，两次运行，其指针值必然不等。此时就应当关闭。其他时候应当打开，对于分析日志非常有用。

// 所有的invoke、return、get、put、aload、astore，无论是否跨线程，都被
// 记录。
MiniLogger order_logger(cout, false);
MiniLogger top_method_logger(cout, false);
MiniLogger inter_st_logger(cout, false);

// 确定消息相关
MiniLogger certain_msg_logger(cout, true);

// 推测消息相关
MiniLogger spec_msg_logger(cout, true);

// 记录确定控制转移
MiniLogger control_transfer_logger(cout, true);

// 记录停机与唤醒
MiniLogger wakeup_halt_logger(cout, false);

MiniLogger switch_mode_logger(cout, false);

// unwind栈桢
MiniLogger unwind_frame_logger(cout, false);

MiniLogger c_logger(cout, false);
MiniLogger c_new_logger(cout, true, &c_logger);
MiniLogger c_pop_frame_logger(cout, true, &c_logger);


MiniLogger s_logger(cout, true);
MiniLogger s_new_logger(cout, false, &s_logger);
MiniLogger s_push_frame_logger(cout, false, &s_logger);
MiniLogger s_pop_frame_logger(cout, true, &s_logger);

MiniLogger r_logger(cout, false);
MiniLogger r_new_logger(cout, true, &r_logger);
MiniLogger r_frame_logger(cout, true, &r_logger);
MiniLogger r_pop_frame_logger(cout, true, &r_logger);

MiniLogger step_loop_in_out_logger(cout, true);

MiniLogger snapshot_logger(cout, true); // 只用来控制下级logger，不用来输出
MiniLogger snapshot_take_logger(cout, true, &snapshot_logger);
MiniLogger snapshot_take_pin_logger(cout, true, &snapshot_take_logger);
MiniLogger snapshot_commit_logger(cout, true, &snapshot_logger);
MiniLogger snapshot_discard_logger(cout, true, &snapshot_logger);


MiniLogger snapshot_detail_logger(cout, false, &snapshot_logger);

MiniLogger task_load_logger(cout, true);

MiniLogger spec_barrier_logger(cout, true); // 推测路障logger

// 记录验证时的情况
MiniLogger verify_logger(cout, true);
MiniLogger verify_detail_logger(cout, false, &verify_logger);

MiniLogger rvp_logger(cout, true);

MiniLogger when_leave_rvp_logger(cout, false);

MiniLogger c_exception_logger(cout, false);
MiniLogger s_exception_logger(cout, false);
MiniLogger r_exception_logger(cout, true);

// 释放栈桢所占内存时
MiniLogger free_frames_logger(cout, true);

// 记录leader入主spmt线程
MiniLogger leader_logger(cout, true);

MiniLogger break_logger(cout, true);

//-------------------------------------------------------------------
void initialiseLoggers(InitArgs *args)
{
    if (args->do_log) {
        RopeVM::instance()->turn_on_log();
    }
    else {
        RopeVM::instance()->turn_off_log();
    }
}
