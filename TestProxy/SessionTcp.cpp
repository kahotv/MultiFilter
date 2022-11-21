#include "stdafx.h"
#include "SessionTcp.h"

#pragma once
//TCP Session 每个连接一个session

SessionTcp::SessionTcp(io_context& ctx, sockettcp_auto sock_local, sockettcp_auto sock_remote, shared_ptr<evt_tcpnew_req> conninfo, uint64_t id)
	: _ctx(ctx)
	, _socket_local(sock_local)
	, _socket_remote(sock_remote)
	, _conninfo(conninfo)
	, _id(id)
{
	printf("[SessionTcp] malloc %p\n", this);
}

SessionTcp::~SessionTcp()
{
	printf("[SessionTcp] free %p\n", this);
}

void SessionTcp::Start()
{
	_run = true;

	//初始化连接
	_socket_remote->set_option(asio::socket_base::send_buffer_size(12 * 1024 * 1024));
	_socket_remote->set_option(asio::socket_base::receive_buffer_size(12 * 1024 * 1024));
	_socket_remote->set_option(asio::ip::tcp::no_delay(true));
	_socket_local->set_option(asio::socket_base::send_buffer_size(12 * 1024 * 1024));
	_socket_local->set_option(asio::socket_base::receive_buffer_size(12 * 1024 * 1024));
	_socket_local->set_option(asio::ip::tcp::no_delay(true));
	doReadLocal();
	doReadRemote();
}

void SessionTcp::doConnectRemote()
{
}

void SessionTcp::doReadLocal()
{
	auto self = shared_from_this();
	_socket_local->async_receive(
		buffer(_buf_local, sizeof(_buf_local)),
		[this, self](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			//printf("[SessionTcp][%lld] doReadLocal() size: %d\n", _id, size);
			//doReadLocal();
			_buf_local_size += size;
			doWriteRemote();
		}
		else
		{
			closeLocal();
			printf("[SessionTcp][%lld] doReadLocal() error: %s\n", _id, ec.message().c_str());
		}
	}
	);
}
void SessionTcp::doWriteRemote()
{
	auto self = shared_from_this();
	_socket_remote->async_send(buffer(_buf_local + _buf_local_pos, _buf_local_size),
		[this, self](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			//printf("[SessionTcp][%lld] doWriteRemote() size: %d\n", _id, size);

			if (_buf_local_size != size)
			{
				//一般不会到这里
				_buf_local_pos += size;
				_buf_local_size -= size;
				//继续发送
				doWriteRemote();
			}
			else
			{

				_buf_local_pos = 0;
				_buf_local_size = 0;
				//发送完了，继续读local
				doReadLocal();
			}
		}
		else
		{
			closeRemote();
			printf("[SessionTcp][%lld] doReadRemote() error: %s\n", _id, ec.message().c_str());
		}
	});
}

void SessionTcp::doReadRemote()
{
	auto self = shared_from_this();
	_socket_remote->async_receive(
		buffer(_buf_remote, sizeof(_buf_remote)),
		[this, self](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			//printf("[SessionTcp][%lld] doReadRemote() size: %d\n", _id, size);
			_buf_remote_size += size;
			doWriteLocal();
		}
		else
		{
			closeRemote();
			printf("[SessionTcp][%lld] doReadRemote() error: %s\n", _id, ec.message().c_str());
		}
	}
	);
}

void SessionTcp::doWriteLocal()
{
	auto self = shared_from_this();
	_socket_local->async_send(buffer(_buf_remote + _buf_remote_pos, _buf_remote_size),
		[this, self](asio::error_code ec, size_t size)
	{
		if (!ec)
		{
			//printf("[SessionTcp][%lld] doWriteLocal() size: %d\n", _id, size);

			if (_buf_remote_size != size)
			{
				//一般不会到这里
				_buf_remote_pos += size;
				_buf_remote_size -= size;
				//继续发送
				doWriteRemote();
			}
			else
			{
				_buf_remote_pos = 0;
				_buf_remote_size = 0;
				//发送完了，继续读local
				doReadRemote();
			}
		}
		else
		{
			printf("[SessionTcp][%lld] doReadRemote() error: %s\n", _id, ec.message().c_str());
			closeLocal();
		}
	});
}


void SessionTcp::closeLocal()
{
	_socket_local->close();
	printf("[SessionTcp][%lld] 关闭local\n", _id);
}
void SessionTcp::closeRemote()
{
	_socket_remote->close();
	printf("[SessionTcp][%lld] 关闭remote\n", _id);
}



