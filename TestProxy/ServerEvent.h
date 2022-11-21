#pragma once
//Event�շ��� �������UDP�˿ڣ����������ٳ�ģ���TCPת������
class ServerEvent : public IEventPoster
{
public:
	ServerEvent(io_context& ctx, uint16_t port_begin, uint16_t port_end);
	~ServerEvent();

	void AddHandler(IEventHandler *evt_handler);
	bool Start();
	void Stop();

	void Replay(shared_ptr<evt_head> resp);

	uint16_t GetListenPort() const;
	void SetForwardPort(uint16_t port);

	// ͨ�� IEventPoster �̳�
	virtual void SendToFilter(const shared_ptr<evt_head> evt) override;

private:
	void doRecv();

	void onEventRequest(const evt_head *head);
	void doReplayHeart(const evt_heart_req *req);

private:
	io_context &_ctx;
	shared_ptr<udp::socket> _listener_ptr = nullptr;
	udp::endpoint _sender_ep;
	uint16_t _port_begin = 0, _port_end = 0;	//���ؼ����˿� ��ʼ�ͽ���
	uint16_t _port = 0;							//���ؼ����˿� ʵ��
	sockaddr_full _forward_v4 = {};				//����ת���ַ
	sockaddr_full _forward_v6 = {};				//����ת���ַ
	bool _run = false;

	char _buf_req[EVENT_BUFF_SIZE];				//����buffer
	char _buf_resp[EVENT_BUFF_SIZE];			//�ظ�buffer

	shared_ptr<evt_heart_resp> _heart_resp = nullptr;

	set<IEventHandler*> _evt_handlers;
};
