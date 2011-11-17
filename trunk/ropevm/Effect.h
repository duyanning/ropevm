#ifndef EFFECT_H
#define EFFECT_H

class Message;
class Snapshot;
class SpmtThread;


class Effect {
public:
    Effect();

    void add_to_C(Frame* frame);
    void remove_from_C(Frame* frame);

    void add_to_R(Frame* frame);

	std::list<Frame*> C; // set of 推测模式下产生但尚未释放的栈桢 // effect被否定时释放，详见[[file:frame-management.org]]
    std::vector<Frame*> R; // set of 推测模式下return了但未释放的栈桢 // effect被确认时释放，详见[[file:frame-management.org]]

    Message* msg_sent;
	Snapshot* snapshot; // 如果尚未产生快照，则设为0
};

#endif  // EFFECT_H
