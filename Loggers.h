#ifndef LOGGERS_H
#define LOGGERS_H

extern MiniLogger owner_change_logger;
extern MiniLogger user_change_logger;
extern MiniLogger c_user_change_logger;
extern MiniLogger s_user_change_logger;
extern MiniLogger r_user_change_logger;
extern MiniLogger c_logger;
extern MiniLogger s_logger;
extern MiniLogger r_logger;
extern MiniLogger c_new_logger;
extern MiniLogger c_new_main_logger;
extern MiniLogger c_new_sub_logger;
extern MiniLogger s_new_logger;
extern MiniLogger s_new_main_logger;
extern MiniLogger s_new_sub_logger;
extern MiniLogger r_new_logger;
extern MiniLogger step_loop_in_out_logger;
extern MiniLogger c_destroy_frame_logger;
extern MiniLogger s_destroy_frame_logger;
extern MiniLogger r_destroy_frame_logger;
extern MiniLogger r_frame_logger;
extern MiniLogger snapshot_detail_logger;
extern MiniLogger task_load_logger;
extern MiniLogger commit_logger;
extern MiniLogger commit_detail_logger;
extern MiniLogger verify_logger;
extern MiniLogger verify_detail_logger;
extern MiniLogger snapshot_logger;
extern MiniLogger when_leave_certain_logger;
extern MiniLogger when_enter_certain_logger;
extern MiniLogger when_leave_rvp_logger;
extern MiniLogger c_exception_logger;
extern MiniLogger s_exception_logger;
extern MiniLogger r_exception_logger;
extern MiniLogger c_invoke_return_logger;
extern MiniLogger s_invoke_return_logger;
extern MiniLogger r_invoke_return_logger;
extern MiniLogger cache_logger;
extern MiniLogger free_frames_logger;
extern MiniLogger mark_frame_certain_logger;

#endif // LOGGERS_H