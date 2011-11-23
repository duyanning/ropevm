#ifndef LOGGERS_H
#define LOGGERS_H

extern MiniLogger wakeup_halt_logger;
extern MiniLogger order_logger;
extern MiniLogger top_method_logger;
extern MiniLogger inter_spmt_thread_logger;
extern MiniLogger certain_msg_logger;
extern MiniLogger spec_msg_logger;
extern MiniLogger control_transfer_logger;
extern MiniLogger c_logger;
extern MiniLogger s_logger;
extern MiniLogger r_logger;
extern MiniLogger c_new_logger;
extern MiniLogger s_new_logger;
extern MiniLogger r_new_logger;
extern MiniLogger step_loop_in_out_logger;
extern MiniLogger c_destroy_frame_logger;
extern MiniLogger s_create_frame_logger;
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
extern MiniLogger snapshot_take_logger;
extern MiniLogger snapshot_take_pin_logger;
extern MiniLogger snapshot_commit_logger;
extern MiniLogger snapshot_discard_logger;
extern MiniLogger when_leave_rvp_logger;
extern MiniLogger c_exception_logger;
extern MiniLogger s_exception_logger;
extern MiniLogger r_exception_logger;
extern MiniLogger free_frames_logger;
extern MiniLogger invoke_return_logger;

extern bool p;

#endif // LOGGERS_H
