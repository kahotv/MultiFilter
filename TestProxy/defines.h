#pragma once

#define PROXY_TCP_PORT_BEGIN	40000		//本地代理端口开始
#define PROXY_TCP_PORT_END		40888		//本地代理端口结束
#define	EVEMT_PORT_BEGIN		40000		//事件端口开始
#define EVENT_PORT_END			40777		//事件端口结束

using namespace std;
using namespace asio;
using namespace asio::ip;


typedef shared_ptr<tcp::socket> sockettcp_auto;


//UDP处理接口
class IUdpProcesspor
{
public:
	virtual void onUdpCreated(shared_ptr<conn_info> conn) = 0;
	virtual void onUdpClosed() = 0;
	virtual void onUdpSend(const uint8_t* buf, size_t size) = 0;
	virtual void onUdpRecv(const uint8_t* buf, size_t size) = 0;

	virtual void setId(uint64_t id) = 0;
	virtual uint64_t getId() = 0;

	virtual void setOptions(uint8_t* buf, size_t size) = 0;

	//刷新活跃时间
	virtual void updateActive() = 0;
	//取最近活跃时间
	virtual uint64_t getLastActive() = 0;
};

struct udp_processor
{
	vector<uint8_t>	options;		//options
	shared_ptr<conn_info> conn;
	shared_ptr<IUdpProcesspor> processor;
	shared_ptr<steady_timer> timer;
};