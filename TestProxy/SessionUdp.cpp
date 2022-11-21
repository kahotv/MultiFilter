#include "stdafx.h"
#include "SessionUdp.h"



SessionUdp::SessionUdp(io_context &ctx, IEventPoster &evt)
	: _ctx(ctx)
	, _evt(evt)
	, _sock_remote(ctx)
{
	printf("[SessionUdp] malloc %p\n", this);
}


SessionUdp::~SessionUdp()
{
	printf("[SessionUdp] free %p\n", this);
}

void SessionUdp::onUdpCreated(shared_ptr<conn_info> conn)
{
	printf("[SessionUdp] onUdpCreated %s -> %s\n", 
		Utils::IPGetEndpoint((sockaddr*)&conn->src).c_str(),
		Utils::IPGetEndpoint((sockaddr*)&conn->dst).c_str());

	_conn = conn;
	doConnectRemote();
}

void SessionUdp::onUdpClosed()
{
	printf("[SessionUdp] onUdpClosed %s -> %s\n",
		Utils::IPGetEndpoint((sockaddr*)&_conn->src).c_str(),
		Utils::IPGetEndpoint((sockaddr*)&_conn->dst).c_str());

	_sock_remote.close();
}

void SessionUdp::onUdpSend(const uint8_t* buf, size_t size)
{
	memcpy(_buf_local + _buf_local_pos, buf, size);

	//TODO 填充协议头

	auto self = shared_from_this();
	_sock_remote.async_send(buffer(buf, size + _buf_local_pos),
		[this, self](asio::error_code ec,size_t size)
	{

	});
}

void SessionUdp::onUdpRecv(const uint8_t* buf, size_t size)
{
	uint16_t udlen = (uint16_t)_ud.size();		
	uint16_t totallen = (uint16_t)(sizeof(evt_udp_recv) + udlen + size);
	shared_ptr<evt_udp_recv> evt((evt_udp_recv*)new char[totallen]);
	evt->source = evt_source::unknown;
	evt->type = evt_type::udp_recv;
	evt->family = _conn->src.family;
	evt->src = _conn->src;
	evt->dst = _conn->dst;
	evt->totallen = totallen;
	evt->udlen = udlen;
	evt->recvlen = (uint16_t)size;

	//复制ud和recvdata
	memcpy(evt->buf, _ud.data(), evt->udlen);
	memcpy(evt->buf + evt->udlen, buf, size);

	_evt.SendToFilter(evt);
}

void SessionUdp::setId(uint64_t id)
{
	if (_id == 0)
	{
		_id = id;
	}
}

uint64_t SessionUdp::getId()
{
	return _id;
}

void SessionUdp::setOptions(uint8_t* buf, size_t size)
{
	if (size != 0)
	{
		_ud.resize(size);
		memcpy(_ud.data(), buf, size);
	}
}

void SessionUdp::updateActive()
{
	_last_active_time_ms = Utils::GetNowTime();
}
uint64_t SessionUdp::getLastActive()
{
	return _last_active_time_ms;
}

void SessionUdp::doConnectRemote()
{
	auto &src = _conn->src;
	auto &dst = _conn->dst;

	auto self = shared_from_this();

	if (dst.family == AF_INET)
	{
		_ep_remote = udp::endpoint(address_v4(ntohl(dst.si4.sin_addr.S_un.S_addr)), ntohs(dst.si4.sin_port));
		_sock_remote.open(udp::v4());
	}
	else 
	{
		address_v6::bytes_type buf;
		memcpy(buf.data(), &dst.si6.sin6_addr, sizeof(dst.si6.sin6_addr));
		
		_ep_remote = udp::endpoint(make_address_v6(buf, dst.si6.sin6_scope_id), ntohs(dst.si6.sin6_port));
		_sock_remote.open(udp::v6());
	}
	_sock_remote.connect(_ep_remote);
	doRecvRemote();
	/*
	_sock_remote.async_connect(_ep_remote,
		[this, self](asio::error_code ec) 
	{
		if (!ec)
		{
			doRecvRemote();
		}
		else 
		{
			printf("[SessionUdp] doConnectRemote fail: %s\n", ec.message().c_str());
		}
	});
	*/
}

void SessionUdp::doRecvRemote()
{
	auto self = shared_from_this();

	_sock_remote.async_receive(buffer(_buf_remote, sizeof(_buf_remote)),
		[this, self](asio::error_code ec,size_t size)
	{
		if (!ec)
		{
			onRecvRemote(size);
		}
		else 
		{
			printf("[SessionUdp] doRecvRemote fail: %s\n", ec.message().c_str());
		}
	});
}

void SessionUdp::onRecvRemote(size_t size)
{
	onUdpRecv(_buf_remote, size);
	doRecvRemote();
}