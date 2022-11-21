#include "stdafx.h"
#include "ServerUdp.h"



ServerUdp::ServerUdp(io_context& ctx, IEventPoster &evt, uint16_t port_begin, uint16_t port_end)
	:_ctx(ctx)
	, _evt(evt)
	, _listener4(ctx)
	, _listener6(ctx)
	, _port_begin(port_begin)
	, _port_end(port_end)
{
	printf("[ServerUdp] malloc %p\n", this);

}
ServerUdp::~ServerUdp()
{
	printf("[ServerUdp] free %p\n", this);
}

bool ServerUdp::Start()
{
	_run = true;
	/*if (!_run)
	{
		for (uint16_t port = _port_end; port >= _port_begin; port--)
		{
			asio::error_code ec4, ec6;
			udp::socket listen4(_ctx), listen6(_ctx);
			udp::socket sender4(_ctx), sender6(_ctx);

			listen4.open(udp::v4(), ec4);
			listen6.open(udp::v6(), ec6);
			listen4.bind(udp::endpoint(address_v4::loopback(), port), ec4);
			listen6.bind(udp::endpoint(address_v6::loopback(), port), ec6);

			if (!ec4 && !ec6)
			{
				_run = true;
				_listener4 = move(listen4);
				_listener6 = move(listen6);
				break;
			}

			printf("[ServerUdp] bind udp v4: %s\n", ec4.message().c_str());
			printf("[ServerUdp] bind udp v6: %s\n", ec6.message().c_str());
		}
	}

	if (_run)
	{
		doRecvLocal4();
		doRecvLocal6();
	}*/

	return _run;
}
void ServerUdp::Stop()
{
	_run = false;

	//if (_run)
	//{
	//	_listener4.close();
	//	_listener6.close();
	//}
}
uint16_t ServerUdp::GetPort() const
{
	return _port;
}

void ServerUdp::OnEventRequest(const evt_head* req)
{
	if (req->type != evt_type::udp_send)
		return;

	onUdpSend((evt_udp_send*)req);
}

void ServerUdp::onUdpSend(evt_udp_send* evt)
{
	//TODO 逻辑判断
	conn_info conn = fillConnInfo(evt->family, evt->src, evt->dst);
	shared_ptr<udp_processor> processor = getUdpProcessor(conn);

	if (processor == nullptr)
	{
		//创建UDP处理实例
		processor = createUdpProcessor();
		processor->conn = make_shared<conn_info>(conn);	//填充conn
		processor->processor->setId(getUniqueId());		//
		processor->processor->setOptions(evt->buf, evt->udlen);//填充options，用于recv
		setUdpProcessor(conn, processor);

		processor->processor->onUdpCreated(processor->conn);

		//启动监控
		doUpdateActive(processor);
	}

	//发送数据
	processor->processor->onUdpSend(evt->buf + evt->udlen, evt->sendlen);

	//刷新活跃时间
	processor->processor->updateActive();
}