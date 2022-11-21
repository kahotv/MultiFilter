#pragma once
//Event收发类 负责监听UDP端口，接收流量劫持模块的TCP转发请求
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

	// 通过 IEventPoster 继承
	virtual void SendToFilter(const shared_ptr<evt_head> evt) override;

private:
	void doRecv();

	void onEventRequest(const evt_head *head);
	void doReplayHeart(const evt_heart_req *req);

private:
	io_context &_ctx;
	shared_ptr<udp::socket> _listener_ptr = nullptr;
	udp::endpoint _sender_ep;
	uint16_t _port_begin = 0, _port_end = 0;	//本地监听端口 开始和结束
	uint16_t _port = 0;							//本地监听端口 实际
	sockaddr_full _forward_v4 = {};				//本地转向地址
	sockaddr_full _forward_v6 = {};				//本地转向地址
	bool _run = false;

	char _buf_req[EVENT_BUFF_SIZE];				//请求buffer
	char _buf_resp[EVENT_BUFF_SIZE];			//回复buffer

	shared_ptr<evt_heart_resp> _heart_resp = nullptr;

	set<IEventHandler*> _evt_handlers;
};
