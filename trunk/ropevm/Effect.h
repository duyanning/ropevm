#ifndef EFFECT_H
#define EFFECT_H

class Message;
class Snapshot;
class SpmtThread;


class Effect {
public:
    Message* msg_sent;
    //SpmtThread* msg_target_spmt_thread; // 这个或可放入消息中

	std::vector<Frame*> C; // set of 推测模式下产生但尚未释放的栈桢 // effect被否定时释放，详见[[file:frame-management.org]]
    std::vector<Frame*> R; // set of 推测模式下return了但未释放的栈桢 // effect被确认时释放，详见[[file:frame-management.org]]

	Snapshot* snapshot; // 如果尚未产生快照，则设为0

    void commit();
    void discard();

};

#endif  // EFFECT_H
