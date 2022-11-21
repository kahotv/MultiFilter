#pragma once
//TCP转发类
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

	uint16_t _port_begin, _port_end;	//本地监听端口 开始和结束
	uint16_t _port = 0;					//本地监听端口 实际

	bool _run = false;

	//key=本地端口(主机序)
	//以下两个容器用来解决accept和event先后顺序问题，用完后要从容器移除，变为SessionTcp持有
	//小概率 先收到accept后收到event
	//大概率 先收到event后收到accpet
	//map<uint16_t, shared_ptr<evt_tcpnew_req>> _requests_event;		//储存event数据
	map<uint16_t, shared_ptr<evt_tcpnew_req>> _sessions_conninfo;			//储存临时的远端socket，等待accept时使用
	map<uint16_t, shared_ptr<tcp::socket>> _sessions_socket;			//储存临时的远端socket，等待accept时使用
	//map<uint16_t, shared_ptr<tcp::socket>>	_requests_socket;		//储存accept socket数据
};
