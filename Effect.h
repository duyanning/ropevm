#ifndef EFFECT_H
#define EFFECT_H

class Message;
class Snapshot;

class Effect {
public:
    std::vector<Message*> m_msgs_sent; // 期间都给谁发送了什么消息，put消息的话可能有多个，invoke消息的话就只有一个。确认这些消息的顺序很重要。
	std::vector<Frame*> m_C; // set of 推测模式下产生但尚未释放的栈桢 // effect被否定时释放，详见[[file:frame-management.org]]
    std::vector<Frame*> m_R; // set of 推测模式下return了但未释放的栈桢 // effect被确认时释放，详见[[file:frame-management.org]]

	Snapshot* snapshot; // 如果尚未产生快照，则设为0

    void add_spec_msg(Message* msg);
    void set_snapshot(Snapshot* snapshot);

};

#endif  // EFFECT_H
