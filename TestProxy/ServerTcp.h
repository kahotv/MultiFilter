#pragma once
//TCPת����
class ServerTcp : public IEventHandler
{
public:
	ServerTcp(io_context& ctx, IEventPoster &evt, uint16_t port_begin, uint16_t port_end);
	~ServerTcp();
	bool Start();
	void Stop();
	uint16_t GetPort() const;

	virtual void OnEventRequest(const evt_head *req) override;

private:
	void onAccept(shared_ptr<tcp::acceptor> ptr, tcp::socket sock_tmp, asio::error_code ec);
	void doAccept(shared_ptr<tcp::acceptor> ptr);

	uint64_t getUniqueId();

	void doConnectRemote(shared_ptr<evt_tcpnew_req> req_tcp);

	void createSession(shared_ptr<evt_tcpnew_req> req_tcp, shared_ptr<tcp::socket> sock_remote, shared_ptr<tcp::socket> sock_local);
private:
	io_context &_ctx;
	IEventPoster &_evt;

	tcp::socket _socket_forward_tmp_v4;
	tcp::socket _socket_forward_tmp_v6;
	shared_ptr<tcp::acceptor> _acceptor_v4_ptr = nullptr;
	shared_ptr<tcp::acceptor> _acceptor_v6_ptr = nullptr;

	uint16_t _port_begin, _port_end;	//���ؼ����˿� ��ʼ�ͽ���
	uint16_t _port = 0;					//���ؼ����˿� ʵ��

	bool _run = false;

	//key=���ض˿�(������)
	//�������������������accept��event�Ⱥ�˳�����⣬�����Ҫ�������Ƴ�����ΪSessionTcp����
	//С���� ���յ�accept���յ�event
	//����� ���յ�event���յ�accpet
	//map<uint16_t, shared_ptr<evt_tcpnew_req>> _requests_event;		//����event����
	map<uint16_t, shared_ptr<evt_tcpnew_req>> _sessions_conninfo;			//������ʱ��Զ��socket���ȴ�acceptʱʹ��
	map<uint16_t, shared_ptr<tcp::socket>> _sessions_socket;			//������ʱ��Զ��socket���ȴ�acceptʱʹ��
	//map<uint16_t, shared_ptr<tcp::socket>>	_requests_socket;		//����accept socket����
};
