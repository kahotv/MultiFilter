#include "stdafx.h"



/// <summary>
/// 监听UDP端口 接收转发消息
/// </summary>
class IServerLogic
{
public:
	virtual void OnTcpAccept(uint64_t id, const sockaddr_full& src, const sockaddr_full& dst);
	virtual void OnUdpSend(uint64_t id, const sockaddr_full& src, const sockaddr_full& dst);
	virtual void OnUdpRecv(uint64_t id, const sockaddr_full& src, const sockaddr_full&dst);
};


int main()
{
	Utils::InitNetwork();
	io_context ctx;
	sm_event_info info = {};
	SharedMemory sm(HEART_SM_NAME, HEART_SM_SIZE);
	ServerEvent evt(ctx, EVEMT_PORT_BEGIN, EVENT_PORT_END);
	ServerTcp tcp(ctx, evt, PROXY_TCP_PORT_BEGIN, PROXY_TCP_PORT_END);
	ServerUdp udp(ctx, evt, PROXY_TCP_PORT_BEGIN, PROXY_TCP_PORT_END);
	do
	{
		bool b = false;

		evt.AddHandler((IEventHandler*)&tcp);
		evt.AddHandler((IEventHandler*)&udp);
		b = evt.Start();
		printf("event start(): %s\n", b ? "succ" : "fail");
		if (!b) {
			break;
		}

		b = tcp.Start();
		printf("tcp start(): %s\n", b ? "succ" : "fail");
		if (!b) {
			evt.Stop();
			break;
		}

		b = udp.Start();
		printf("udp start(): %s\n", b ? "succ" : "fail");
		if (!b) {
			evt.Stop();
			tcp.Stop();
			break;
		}

		info.event_port = evt.GetListenPort();
		info.forward_port = tcp.GetPort();

		evt.SetForwardPort(info.forward_port);

		b = sm.Write(info);
		printf("写入共享信息%s 转发端口: %d, 事件端口: %d\n", b ? "succ" : "fail", info.forward_port, info.event_port);
		ctx.run();
	} while (false);

	system("pause");
	return 0;
}
