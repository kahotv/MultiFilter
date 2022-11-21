#pragma once
class ClientEvent
{
public:
	ClientEvent::ClientEvent(io_context& ctx, uint16_t event_port);

	~ClientEvent();

	void post(shared_ptr<evt_head> req);	//发送事件


	void do_heart_timer(bool first);		//用于发送心跳的时钟
	void do_heart_send();					//时钟里调用这个函数来发送心跳

	void start();
	void stop();

	void SetHandler(IEventHandler* handler);

	bool is_server_active_unsafe();
private:

	
	void do_event_recv();					//接收事件
private:
	char _buff_recv[EVENT_BUFF_SIZE];					//接收的事件缓冲区
	udp::endpoint _ep_remote;

	bool _run = false;
	io_context& _ctx;
	udp::socket _sock;

	IEventHandler* _handler = nullptr;
	//std::function<void(evt_tcpnew_resp*)> _on_resp;

	steady_timer _heart_timer;									//心跳时钟
	shared_ptr<evt_heart_req> _heart_request = nullptr;							//心跳请求内容

	uint32_t	_heart_span_ms = HEART_INTERVAL_MS;				//心跳检测间隔时间（毫秒）
	uint32_t	_heart_error_times = 0;							//错误计数
	uint32_t	_heart_error_times_max = HEART_ERROR_TIMES_MAX;	//允许的最大错误次数
};