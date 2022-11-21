#include "stdafx.h"
#include "ServerEvent.h"
//Event收发类 负责监听UDP端口，接收流量劫持模块的TCP转发请求

ServerEvent::ServerEvent(io_context& ctx, uint16_t port_begin, uint16_t port_end) :
	_ctx(ctx)
	, _port_begin(port_begin)
	, _port_end(port_end)
{
	printf("[ServerEvent] malloc %p\n", this);
	memset(_buf_req, 0, sizeof(_buf_req));
}
ServerEvent::~ServerEvent()
{
	printf("[ServerEvent] malloc %p\n", this);
}

void ServerEvent::doRecv()
{
	_listener_ptr->async_receive_from(buffer(_buf_req, sizeof(_buf_req)), _sender_ep,
		[this](asio::error_code ec, size_t size)
	{
		//printf("[ServerEvent] do_recv size: %d, err: %s\n", size, ec.message().c_str());
		if (!ec)
		{
			const auto &local = _listener_ptr->local_endpoint();
			const auto &remote = _sender_ep;

			/*printf("[ServerEvent]收到event: %s:%d --> %s:%d | size: %d\n",
				remote.address().to_string().c_str(), remote.port(),
				local.address().to_string().c_str(), local.port(),
				size);*/

			onEventRequest((evt_head*)_buf_req);
		}

		if (_run)
		{
			doRecv();
		}
	});


}
void ServerEvent::onEventRequest(const evt_head* req)
{
	if (req->type == evt_type::heart)
	{
		doReplayHeart((const evt_heart_req*)req);
		
	}else
	{
		for (const auto& handler : _evt_handlers)
		{
			handler->OnEventRequest(req);
		}
	}
}
void ServerEvent::doReplayHeart(const evt_heart_req* req)
{
	if (_heart_resp == nullptr)
	{
		_heart_resp = make_shared<evt_heart_resp>();
		_heart_resp->type = evt_type::heart;
		_heart_resp->totallen = sizeof(evt_heart_resp);
	}

	_heart_resp->evt_id = req->evt_id;
	_heart_resp->time = Utils::GetNowTime();

	this->Replay(_heart_resp);
}

void ServerEvent::SendToFilter(const shared_ptr<evt_head> evt)
{
	this->Replay(evt);
}

void ServerEvent::AddHandler(IEventHandler *evt_handler)
{
	if (_evt_handlers.find(evt_handler) == _evt_handlers.end())
	{
		_evt_handlers.insert(evt_handler);
	}
}

bool ServerEvent::Start()
{
	_run = true;

	for (uint16_t port = _port_end; port >= _port_begin; port--)
	{

		try
		{
			_listener_ptr = make_shared<udp::socket>(_ctx, udp::endpoint(address_v4::loopback(), port));
			doRecv();
		}
		catch (const asio::error_code& ec)
		{
			printf("[ServerEvent]监听失败1 端口: %d, 错误信息: %s\n", port, ec.message().c_str());
			_listener_ptr = nullptr;
			Sleep(10);
			continue;
		}
		catch (const exception& ex)
		{
			printf("[ServerEvent]监听失败2 端口: %d, 错误信息: %s\n", port, ex.what());
			_listener_ptr = nullptr;
			Sleep(10);
			continue;
		}

		printf("[ServerEvent]监听成功 端口: %d\n", port);

		_port = port;
		break;
	}

	if (_listener_ptr == nullptr)
	{
		printf("监听全部失败 端口范围: %d-%d\n", _port_begin, _port_end);

		_listener_ptr = nullptr;
		_run = false;
	}

	return _run;
}
void ServerEvent::Stop()
{
	if (_run)
	{
		_run = false;
		_ctx.post([this]() {_listener_ptr->close(); });
	}

	_ctx.post([this]() {_ctx.stop(); });
}
void ServerEvent::Replay(shared_ptr<evt_head> resp)
{
	if (_run && _listener_ptr != nullptr)
	{
		_listener_ptr->async_send_to(buffer(resp.get(), resp->totallen), 
			_sender_ep,
			[this, resp](asio::error_code ec,size_t size)
		{
			//printf("[ServerEvent] Replay 回复Event evt_id: %d\n", resp->evt_id);
		});
	}
}
uint16_t ServerEvent::GetListenPort() const
{
	return _port;
}
void ServerEvent::SetForwardPort(uint16_t port)
{
	_forward_v4 = {};
	_forward_v6 = {};

	_forward_v4.si4.sin_family = AF_INET;
	_forward_v4.si4.sin_addr = in4addr_loopback;
	_forward_v4.si4.sin_port = htons(port);

	_forward_v6.si6.sin6_family = AF_INET6;
	_forward_v6.si6.sin6_addr = in6addr_loopback;
	_forward_v6.si6.sin6_port = htons(port);
}
