#pragma once

//TCP Session 每个连接一个session
//背靠背代理，当一方close后，需要等close方的数据send完后才close另一方
class SessionTcp : public enable_shared_from_this<SessionTcp>
{
public:
	SessionTcp(io_context& ctx, sockettcp_auto sock_local, sockettcp_auto sock_remote, shared_ptr<evt_tcpnew_req> conninfo, uint64_t id);
	~SessionTcp();

	void Start();

private:
	void doConnectRemote();

	void doReadLocal();		//local -> proxy
	void doWriteRemote();	//proxy ->remote

	void doReadRemote();	//remote -> proxy
	void doWriteLocal();	//proxy -> local

	void closeLocal();
	void closeRemote();
private:

	io_context& _ctx;

	sockettcp_auto _socket_local = nullptr;
	sockettcp_auto _socket_remote = nullptr;

	char _buf_local[SERVER_TCP_BUFFER];
	char _buf_remote[SERVER_TCP_BUFFER];

	size_t _buf_local_pos = 0;
	size_t _buf_local_size = 0;
	size_t _buf_remote_pos = 0;
	size_t _buf_remote_size = 0;

	uint64_t _id = 0;

	shared_ptr<evt_tcpnew_req> _conninfo = nullptr;

	bool _run = false;
};