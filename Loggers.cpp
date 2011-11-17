#include "std.h"
#include "Loggers.h"
#include "rope.h"

using namespace std;

// 所有的invoke、return、get、put、aload、astore，无论是否跨线程，都被
// 记录。
MiniLogger order_logger(cout, false);
MiniLogger top_method_logger(cout, false);
MiniLogger inter_spmt_thread_logger(cout, false);

// 确定消息相关
MiniLogger certain_msg_logger(cout, true);

// 推测消息相关
MiniLogger spec_msg_logger(cout, true);

// 记录确定控制转移
MiniLogger control_transfer_logger(cout, true);

MiniLogger owner_change_logger(cout, false);
MiniLogger user_change_logger(cout, false);
MiniLogger c_user_change_logger(cout, true, &user_change_logger);
MiniLogger s_user_change_logger(cout, true, &user_change_logger);
MiniLogger r_user_change_logger(cout, true, &user_change_logger);

MiniLogger c_logger(cout, false);
MiniLogger c_new_logger(cout, true, &c_logger);
MiniLogger c_new_main_logger(cout, true, &c_new_logger);
MiniLogger c_new_sub_logger(cout, false, &c_new_logger);
MiniLogger c_destroy_frame_logger(cout, true, &c_logger);

MiniLogger delete_frame_logger(cout, false);

MiniLogger invoke_return_logger(cout, false);

MiniLogger s_logger(cout, true);
MiniLogger s_new_logger(cout, false, &s_logger);
MiniLogger s_new_main_logger(cout, false, &s_new_logger);
MiniLogger s_new_sub_logger(cout, false, &s_new_logger);
MiniLogger s_destroy_frame_logger(cout, true, &s_logger);

MiniLogger r_logger(cout, true);
MiniLogger r_new_logger(cout, true, &r_logger);
MiniLogger r_frame_logger(cout, true, &r_logger);
MiniLogger r_destroy_frame_logger(cout, true, &r_logger);

MiniLogger step_loop_in_out_logger(cout, false);

MiniLogger snapshot_logger(cout, true);
MiniLogger snapshot_detail_logger(cout, false, &snapshot_logger);

MiniLogger task_load_logger(cout, true);

MiniLogger commit_logger(cout, false);
MiniLogger commit_detail_logger(cout, true, &commit_logger);

MiniLogger verify_logger(cout, true);
MiniLogger verify_detail_logger(cout, false, &verify_logger);

MiniLogger when_leave_certain_logger(cout, false);
MiniLogger when_enter_certain_logger(cout, false);
MiniLogger when_leave_rvp_logger(cout, false);

MiniLogger c_exception_logger(cout, false);
MiniLogger s_exception_logger(cout, false);
MiniLogger r_exception_logger(cout, true);

MiniLogger c_invoke_return_logger(cout, false);
MiniLogger s_invoke_return_logger(cout, false);
MiniLogger r_invoke_return_logger(cout, false);

MiniLogger cache_logger(cout, false);
MiniLogger free_frames_logger(cout, false);

//MiniLogger assert_logger(cout, true);

//-------------------------------------------------------------------
void initialiseLoggers(InitArgs *args)
{
    MiniLogger::disable_all_loggers = not args->do_log;
}
