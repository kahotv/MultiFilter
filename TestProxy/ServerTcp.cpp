#include "stdafx.h"
#include "ServerTcp.h"


ServerTcp::ServerTcp(io_context& ctx, IEventPoster &evt, uint16_t port_begin, uint16_t port_end)
	: _ctx(ctx)
	, _evt(evt)
	, _port_begin(port_begin)
	, _port_end(port_end)
	, _socket_forward_tmp_v4(ctx)
	, _socket_forward_tmp_v6(ctx)
{
	printf("[ServerTcp] malloc %p\n", this);
}
ServerTcp::~ServerTcp()
{
	printf("[ServerTcp] free %p\n", this);
}


bool ServerTcp::Start()
{
	_run = true;

	for (uint16_t port = _port_end; port >= _port_begin; port--)
	{
		shared_ptr<tcp::acceptor> ptr_v4 = nullptr;
		shared_ptr<tcp::acceptor> ptr_v6 = nullptr;

		try
		{
			ptr_v4 = make_shared<tcp::acceptor>(_ctx, tcp::endpoint(address_v4::loopback(), port));
			ptr_v6 = make_shared<tcp::acceptor>(_ctx, tcp::endpoint(address_v6::loopback(), port));
			doAccept(ptr_v4);
			doAccept(ptr_v6);
		}
		catch (const asio::error_code& ec)
		{
			printf("[ServerTcp]监听失败1 端口: %d, 错误信息: %s\n", port, ec.message().c_str());
			ptr_v4 = nullptr;
			ptr_v6 = nullptr;
			Sleep(10);
			continue;
		}
		catch (const exception& ex)
		{
			printf("[ServerTcp]监听失败2 端口: %d, 错误信息: %s\n", port, ex.what());
			ptr_v4 = nullptr;
			ptr_v6 = nullptr;
			Sleep(10);
			continue;
		}

		printf("[ServerTcp]监听成功 端口: %d\n", port);

		_acceptor_v4_ptr = ptr_v4;
		_acceptor_v6_ptr = ptr_v6;
		_port = port;
		break;
	}

	if (_acceptor_v4_ptr == nullptr || _acceptor_v6_ptr == nullptr)
	{
		printf("[ServerTcp]监听全部失败 端口范围: %d-%d\n", _port_begin, _port_end);

		_acceptor_v4_ptr = nullptr;
		_acceptor_v6_ptr = nullptr;
		_run = false;
	}

	return _run;
}

void ServerTcp::Stop()
{
	if (_run)
	{
		_run = false;
		_ctx.post([this]()
		{
			_acceptor_v4_ptr->close();
			_acceptor_v6_ptr->close();

			_acceptor_v4_ptr = nullptr;
			_acceptor_v6_ptr = nullptr;
		});
	}

	_ctx.post([this]() {_ctx.stop(); });
}

uint16_t ServerTcp::GetPort() const
{
	return _port;
}

void ServerTcp::OnEventRequest(const evt_head *req)
{
	//解析
	if (req->type == evt_type::tcp_conn_request)
	{
		evt_tcpnew_req *req_tcp = (evt_tcpnew_req*)req;

		//储存
		uint16_t port = ntohs(req_tcp->family == AF_INET ? req_tcp->src.si4.sin_port : req_tcp->src.si6.sin6_port);
		shared_ptr<evt_tcpnew_req> req_tcp_auto((evt_tcpnew_req*)new char[req->totallen]);
		memcpy(req_tcp_auto.get(), req, req->totallen);

		//开始连接远端
		doConnectRemote(req_tcp_auto);
	}
}

void ServerTcp::onAccept(shared_ptr<tcp::acceptor> ptr, tcp::socket sock_tmp, asio::error_code ec)
{
	if (!ec)
	{
		const auto &local = sock_tmp.local_endpoint();
		const auto &remote = sock_tmp.remote_endpoint();

		printf("[ServerTcp] 新连接: %s:%d --> %s:%d\n",
			remote.address().to_string().c_str(), remote.port(),
			local.address().to_string().c_str(), local.port());

		uint16_t port = remote.port();

		auto it_sock_remote = _sessions_socket.find(port);
		auto it_conninfo = _sessions_conninfo.find(port);

		if (it_sock_remote != _sessions_socket.end() && it_conninfo != _sessions_conninfo.end())
		{
			shared_ptr<tcp::socket> sock_local = make_shared<tcp::socket>(move(sock_tmp));

			createSession(it_conninfo->second, it_sock_remote->second, sock_local);

			_sessions_socket.erase(it_sock_remote);
			_sessions_conninfo.erase(it_conninfo);
		}
		else 
		{
			assert(false);
			printf("[ServerTcp] accept异常 不可能的情况\n");
		}
	}
	else
	{
		printf("[ServerTcp] accept异常: %s\n", ec.message().c_str());
	}

	if (_run)
	{
		doAccept(ptr);
	}
}

void ServerTcp::doAccept(shared_ptr<tcp::acceptor> ptr)
{
	if (ptr->local_endpoint().address().is_v4())
	{
		ptr->async_accept(_socket_forward_tmp_v4,
			[this, ptr](asio::error_code ec)
		{
			onAccept(ptr, move(_socket_forward_tmp_v4), ec);
		});
	}
	else
	{
		ptr->async_accept(_socket_forward_tmp_v6,
			[this, ptr](asio::error_code ec)
		{
			onAccept(ptr, move(_socket_forward_tmp_v6), ec);
		});
	}

}

uint64_t ServerTcp::getUniqueId()
{
	static uint64_t static_index = 0;
	uint64_t index = 0;

	do
	{
		index = InterlockedIncrement(&static_index);

	} while (index == 0 /*|| index == ICMPProxyID || index == ICMPLocalID*/);

	return index;
}

void ServerTcp::doConnectRemote(shared_ptr<evt_tcpnew_req> req_tcp)
{
	sockaddr_full &remote_addr = req_tcp->dst;


	//连接远端
	std::shared_ptr<tcp::endpoint> remote = nullptr;
	if (req_tcp->family == AF_INET)
	{
		remote = make_shared<tcp::endpoint>(make_address_v4(ntohl(remote_addr.si4.sin_addr.S_un.S_addr)), ntohs(remote_addr.si4.sin_port));
	}
	else
	{
		address_v6::bytes_type buf;
		memcpy(buf.data(), &remote_addr.si6.sin6_addr, sizeof(remote_addr.si6.sin6_addr));
		remote = make_shared<tcp::endpoint>(make_address_v6(buf, remote_addr.si6.sin6_scope_id), ntohs(remote_addr.si6.sin6_port));
	}

	std::shared_ptr<tcp::socket> socket_remote = make_shared<tcp::socket>(_ctx);
	socket_remote->async_connect(*remote,
		[this, req_tcp, remote, socket_remote](asio::error_code ec)
	{
		if (!ec)
		{
			printf("[SessionTcp] 连接远端[%s:%d]成功\n",
				remote->address().to_string().c_str(), remote->port());

			uint16_t port = ntohs(req_tcp->family == AF_INET ? req_tcp->src.si4.sin_port : req_tcp->src.si6.sin6_port);

			_sessions_socket[port] = socket_remote;
			_sessions_conninfo[port] = req_tcp;
		}
		else
		{
			//closeLocal();
			printf("[SessionTcp] 连接远端[%s:%d]失败: %s\n",
				remote->address().to_string().c_str(), remote->port(), ec.message().c_str());
		}

		//连接远端成功，通知劫持那边放行和转向
		//TODO resp可以做成单例
		uint16_t totallen = sizeof(evt_tcpnew_resp) + req_tcp->udlen;
		shared_ptr<evt_tcpnew_resp> resp((evt_tcpnew_resp*)new char[totallen]);

		resp->evt_id = req_tcp->evt_id;
		resp->type = evt_type::tcp_conn_replay;
		resp->source = req_tcp->source;
		resp->totallen = totallen;
		resp->family = req_tcp->family;
		resp->pid = req_tcp->pid;
		resp->src = req_tcp->src;
		resp->dst = req_tcp->dst;
		resp->udlen = req_tcp->udlen;
		memcpy(resp->ud, req_tcp->ud, req_tcp->udlen);

		//设置转向 连接远端失败就不转向
		resp->redirect = !ec;

		//回复
		_evt.SendToFilter(resp);
	});
}

void ServerTcp::createSession(shared_ptr<evt_tcpnew_req> req_tcp, shared_ptr<tcp::socket> sock_remote, shared_ptr<tcp::socket> sock_local)
{
	uint64_t id = getUniqueId();
	auto session = make_shared<SessionTcp>(_ctx, sock_local, sock_remote, req_tcp, id);
	session->Start();
}