#pragma once
class ClientEvent
{
public:
	ClientEvent::ClientEvent(io_context& ctx, uint16_t event_port);

	~ClientEvent();

	void post(shared_ptr<evt_head> req);	//�����¼�


	void do_heart_timer(bool first);		//���ڷ���������ʱ��
	void do_heart_send();					//ʱ������������������������

	void start();
	void stop();

	void SetHandler(IEventHandler* handler);

	bool is_server_active_unsafe();
private:

	
	void do_event_recv();					//�����¼�
private:
	char _buff_recv[EVENT_BUFF_SIZE];					//���յ��¼�������
	udp::endpoint _ep_remote;

	bool _run = false;
	io_context& _ctx;
	udp::socket _sock;

	IEventHandler* _handler = nullptr;
	//std::function<void(evt_tcpnew_resp*)> _on_resp;

	steady_timer _heart_timer;									//����ʱ��
	shared_ptr<evt_heart_req> _heart_request = nullptr;							//������������

	uint32_t	_heart_span_ms = HEART_INTERVAL_MS;				//���������ʱ�䣨���룩
	uint32_t	_heart_error_times = 0;							//�������
	uint32_t	_heart_error_times_max = HEART_ERROR_TIMES_MAX;	//��������������
};