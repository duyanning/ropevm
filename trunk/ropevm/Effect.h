#ifndef EFFECT_H
#define EFFECT_H

class Message;
class Snapshot;

class Effect {
public:
    std::vector<Message*> msgs_sent; // �ڼ䶼��˭������ʲô��Ϣ��put��Ϣ�Ļ������ж����invoke��Ϣ�Ļ���ֻ��һ����ȷ����Щ��Ϣ��˳�����Ҫ��
	std::vector<Frame*> C; // set of �Ʋ�ģʽ�²�������δ�ͷŵ�ջ�� // effect����ʱ�ͷţ����[[file:frame-management.org]]
    std::vector<Frame*> R; // set of �Ʋ�ģʽ��return�˵�δ�ͷŵ�ջ�� // effect��ȷ��ʱ�ͷţ����[[file:frame-management.org]]

	Snapshot* snapshot; // �����δ�������գ�����Ϊ0

};

#endif  // EFFECT_H