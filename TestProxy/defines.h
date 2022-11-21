#pragma once

#define PROXY_TCP_PORT_BEGIN	40000		//���ش���˿ڿ�ʼ
#define PROXY_TCP_PORT_END		40888		//���ش���˿ڽ���
#define	EVEMT_PORT_BEGIN		40000		//�¼��˿ڿ�ʼ
#define EVENT_PORT_END			40777		//�¼��˿ڽ���

using namespace std;
using namespace asio;
using namespace asio::ip;


typedef shared_ptr<tcp::socket> sockettcp_auto;


//UDP����ӿ�
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

	//ˢ�»�Ծʱ��
	virtual void updateActive() = 0;
	//ȡ�����Ծʱ��
	virtual uint64_t getLastActive() = 0;
};

struct udp_processor
{
	vector<uint8_t>	options;		//options
	shared_ptr<conn_info> conn;
	shared_ptr<IUdpProcesspor> processor;
	shared_ptr<steady_timer> timer;
};