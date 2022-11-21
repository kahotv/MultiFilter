#include "stdafx.h"
#include "ClientEvent.h"

ClientEvent::ClientEvent::ClientEvent(io_context& ctx, uint16_t event_port)
	:_ctx(ctx)
	, _ep_remote(address_v4::from_string("127.0.0.1"), event_port)
	, _sock(ctx)
	, _heart_timer(ctx)
{
}
ClientEvent::~ClientEvent()
{

}


void ClientEvent::post(shared_ptr<evt_head> req)
{
	if (!_run)
	{
		return;
	}

	static atomic_uint16_t evt_counter = 0;
	req->evt_id = evt_counter.fetch_add(1);

	//TODO 这里是多线程安全吗？需不需要ctx.post？
	_sock.async_send(buffer(req.get(), req->totallen),
		[req](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			//printf("[client_evt] 发送Event size: %d, evt_id: %d\n", size, req->evt_id);
		}
		else 
		{
			printf("[client_evt] 发送Event size: %d, evt_id: %d, msg: %s\n", size, req->evt_id, ec.message().c_str());
		}
	});
}


void ClientEvent::do_heart_timer(bool first)
{
	if (!_run)
	{
		return;
	}

	if (first)
	{
		_heart_timer.expires_from_now(std::chrono::milliseconds(_heart_span_ms));
	}
	else 
	{
		_heart_timer.expires_at(_heart_timer.expiry() + std::chrono::milliseconds(_heart_span_ms));
	}
	_heart_timer.async_wait(
		[this](asio::error_code ec)
	{
		if (!ec)
		{
			do_heart_send();
			do_heart_timer(false);
		}
	});
}

void ClientEvent::do_heart_send()
{
	if (!_run)
	{
		return;
	}

	if (_heart_request == nullptr)
	{
		_heart_request = make_shared<evt_heart_req>();
		_heart_request->type = evt_type::heart;
		_heart_request->totallen = sizeof(evt_heart_req);
	}

	_heart_request->time = Utils::GetNowTime();

	this->post(_heart_request);
	_heart_error_times++;
}

void ClientEvent::start()
{
	if (!_run)
	{
		_run = true;
		_sock.connect(_ep_remote);
		do_heart_timer(true);		//开启心跳时钟
		do_event_recv();			//开启接收回复（心跳和事件）
	}
}
void ClientEvent::stop()
{
	if (_run)
	{
		_run = false;
		_sock.close();
	}
}

void ClientEvent::SetHandler(IEventHandler* handler)
{
	_handler = handler;
}
bool ClientEvent::is_server_active_unsafe()
{
	if (!_run)
	{
		return false;
	}

	return _heart_error_times <= _heart_error_times_max;
}

void ClientEvent::do_event_recv()
{
	_sock.async_receive(buffer(_buff_recv,sizeof(_buff_recv)),
		[this](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			evt_head* head = (evt_head*)_buff_recv;

			//printf("[client_evt] 收到event size: %d | evt_id: %d\n", size, head->evt_id);

			if (head->type == evt_type::heart)
			{
				bool succ = false;
				if (size == sizeof(evt_heart_resp))
				{
					evt_heart_resp* resp = (evt_heart_resp*)head;
					if (_heart_request->evt_id == resp->evt_id)
					{
						//_heart_error_times--;
						//收到心跳就直接清零
						_heart_error_times = 0;
						succ = true;
					}

				}

				if (!succ)
				{
					printf("[client_evt] heart recv error: %s\n", "心跳数据错误");
				}
			}
			else
			{
				if (_handler != nullptr)
				{
					_handler->OnEventReplay(head);
				}
			}

		}
		else
		{
			printf("[client_evt] heart recv error: %s\n", ec.message().c_str());
		}

		if (_run)
		{
			do_event_recv();
		}
	});
}
