#include "stdafx.h"
#include "NetFilter.h"


NetFilter::NetFilter(io_context &ctx, uint16_t forward_port, uint16_t event_port) :
	_evt(ctx, event_port)
{
	_forward_addr_v4 = {};
	_forward_addr_v4.sin_family = AF_INET;
	_forward_addr_v4.sin_addr = in4addr_loopback;
	_forward_addr_v4.sin_port = htons(forward_port);

	_forward_addr_v6 = {};
	_forward_addr_v6.sin6_family = AF_INET6;
	_forward_addr_v6.sin6_addr = in6addr_loopback;
	_forward_addr_v6.sin6_port = htons(forward_port);

	//启动event模块
	_evt.SetHandler(this);
	_evt.start();

}
NetFilter::~NetFilter()
{
	_evt.stop();
}
// 通过 NF_EventHandler 继承
void NetFilter::threadStart()
{
	printf("[Netfilter] threadStart\n");
}

void NetFilter::threadEnd()
{
	printf("[Netfilter] threadEnd\n");
}

void NetFilter::tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printf("[Netfilter][%lld][%s] tcpConnectRequest\n", id, Utils::GetFamilyString(pConnInfo->ip_family));

	if (_evt.is_server_active_unsafe())
	{
		uint16_t totallen = sizeof(evt_tcpnew_req) + sizeof(evt_tcpnew_req_nf);
		shared_ptr<evt_tcpnew_req> req((evt_tcpnew_req*)new char[totallen]);
		req->totallen = totallen;
		req->type = evt_type::tcp_conn_request;
		req->family = pConnInfo->ip_family;
		req->pid = pConnInfo->processId;
		if (req->family == AF_INET)
		{
			req->src.si4 = (sockaddr_in&)pConnInfo->localAddress;
			req->dst.si4 = (sockaddr_in&)pConnInfo->remoteAddress;
		}
		else
		{
			req->src.si6 = (sockaddr_in6&)pConnInfo->localAddress;
			req->dst.si6 = (sockaddr_in6&)pConnInfo->remoteAddress;
		}

		req->udlen = sizeof(evt_tcpnew_req_nf);
		evt_tcpnew_req_nf* nfinfo = (evt_tcpnew_req_nf*)req->ud;

		nfinfo->nfid = id;
		nfinfo->conninfo = *pConnInfo;

		//转发到代理模块
		postEvent(req);
	}
	else 
	{
		nf_completeTCPConnectRequest(id, pConnInfo);
	}
}

void NetFilter::tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printf("[Netfilter][%lld][%s] tcpConnected\n", id, Utils::GetFamilyString(pConnInfo->ip_family));
	//event



	//发送数据并关闭监视
	//pConnInfo->filteringFlag = NF_FILTERING_FLAG::NF_ALLOW;
	//nf_tcpPostSend(id, (const char*)&evt, sizeof(evt));
	//nf_tcpDisableFiltering(id);
}

void NetFilter::tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printf("[Netfilter][%lld][%s] tcpClosed\n", id, Utils::GetFamilyString(pConnInfo->ip_family));
}

void NetFilter::tcpReceive(ENDPOINT_ID id, const char* buf, int len)
{
	printf("[Netfilter][%lld] tcpReceive\n", id);
	//nf_tcpPostReceive(id, buf, len);
}

void NetFilter::tcpSend(ENDPOINT_ID id, const char* buf, int len)
{
	printf("[Netfilter][%lld] tcpSend\n", id);
	//nf_tcpPostSend(id, buf, len);
}

void NetFilter::tcpCanReceive(ENDPOINT_ID id) 
{
	printf("[Netfilter][%lld] tcpCanReceive\n", id);
}

void NetFilter::tcpCanSend(ENDPOINT_ID id) 
{
	printf("[Netfilter][%lld] tcpCanSend\n", id);
}

void NetFilter::udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo) 
{
	//wchar_t buf[MAX_PATH * 2];
	//if (!nf_getProcessNameW(pConnInfo->processId, buf, sizeof(buf) / 2))
	//{
	//	buf[0] = L'\0';
	//}

	//printf("[Netfilter][%lld][%s] udpCreated %d | %ws\n"
	//	, id, Utils::GetFamilyString(pConnInfo->ip_family), pConnInfo->processId, buf);

	{
		unique_lock<shared_mutex> locker(_udpinfo_lock);
		_udpinfo[id] = *pConnInfo;
	}

}

void NetFilter::udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq) 
{
	//printf("[Netfilter][%lld][%s] udpConnectRequest\n"
	//	, id, Utils::GetFamilyString(pConnReq->ip_family));
}

void NetFilter::udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo) 
{
	//wchar_t buf[MAX_PATH * 2];
	//if (!nf_getProcessNameW(pConnInfo->processId, buf, sizeof(buf) / 2))
	//{
	//	buf[0] = L'\0';
	//}

	//printf("[Netfilter][%lld][%s] udpClosed %d | %ws\n"
	//	, id, Utils::GetFamilyString(pConnInfo->ip_family), pConnInfo->processId, buf);

	{
		unique_lock<shared_mutex> locker(_udpinfo_lock);

		auto it = _udpinfo.find(id);
		if (it != _udpinfo.end())
		{
			_udpinfo.erase(it);
		}
	}
}

void NetFilter::udpReceive(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options) 
{
	//printf("[Netfilter][%lld] udpReceive\n", id);
}

void NetFilter::udpSend(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options) 
{
	//printf("[Netfilter][%lld] udpSend\n", id);

	bool isDns = false;

	sockaddr_full* remote = (sockaddr_full*)remoteAddress;
	if (remote->family == AF_INET && remote->si4.sin_port == htons(53))
	{
		isDns = true;
	}
	else if (remote->family == AF_INET6 && remote->si6.sin6_port == htons(53))
	{
		isDns = true;
	}

	if (isDns)
	{
		nf_udpPostSend(id, remoteAddress, buf, len, options);
		return;
	}


	sockaddr_full remote_new = *remote;

	if (_evt.is_server_active_unsafe() && (remote_new.family == AF_INET || remote_new.family == AF_INET6))
	{
		//转发到中心模块

		auto req = createUdpSendEvent(id, remoteAddress, buf, len, options);

		postEvent(req);
	}
	else 
	{
		nf_udpPostSend(id, remoteAddress, buf, len, options);
	}
}

void NetFilter::udpCanReceive(ENDPOINT_ID id) 
{
	//printf("[Netfilter][%lld] udpCanReceive\n", id);
}

void NetFilter::udpCanSend(ENDPOINT_ID id) 
{
	//printf("[Netfilter][%lld] udpCanSend\n", id);
}

void NetFilter::OnEventReplay(const evt_head *resp)
{
	if (resp->type == evt_type::tcp_conn_replay)
	{
		onTcpConnectReplay((evt_tcpnew_resp*)resp);
	}
	else if (resp->type == evt_type::udp_recv)
	{
		onUdpRecvReplay((evt_udp_recv*)resp);
	}
	else 
	{
		printf("[Netfilter] 未知的event type: %d\n", resp->type);
	}
}

void NetFilter::onTcpConnectReplay(const evt_tcpnew_resp* resp)
{
	if (resp->udlen != sizeof(evt_tcpnew_req_nf))
	{
		printf("[Netfilter][%d] OnEventReplay evtid: %d | 错误的EVENT\n", -1, resp->evt_id);
		return;
	}

	evt_tcpnew_req_nf* nfinfo = (evt_tcpnew_req_nf*)resp->ud;
	printf("[Netfilter][%lld] OnEventReplay  evtid: %d | %s -> %s | 转向: %c\n",
		nfinfo->nfid, resp->evt_id,
		Utils::IPGetEndpoint((sockaddr*)nfinfo->conninfo.localAddress).c_str(),
		Utils::IPGetEndpoint((sockaddr*)nfinfo->conninfo.remoteAddress).c_str(),
		resp->redirect ? 'T' : 'F'
	);

	if (resp->redirect)
	{
		if (resp->family == AF_INET)
		{
			memcpy(nfinfo->conninfo.remoteAddress, &_forward_addr_v4, sizeof(_forward_addr_v4));

		}
		else
		{
			memcpy(nfinfo->conninfo.remoteAddress, &_forward_addr_v6, sizeof(_forward_addr_v6));
		}
	}

	nf_completeTCPConnectRequest(nfinfo->nfid, &nfinfo->conninfo);
}
void NetFilter::onUdpRecvReplay(const evt_udp_recv* resp)
{
	/*
	*	|	evt 	 |	  ud    |  buf   |
	*	|evt_udp_recv|evt_udp_nf|recvdata|
	*/

	evt_udp_nf* udpnf = (evt_udp_nf*)resp->buf;

	ENDPOINT_ID id = udpnf->nfid;
	unsigned char* remoteAddress = (unsigned char*)&resp->dst;
	const char* buf = (const char*)resp->buf + resp->udlen;
	int buflen = resp->recvlen;
	NF_UDP_OPTIONS *options = (NF_UDP_OPTIONS*)udpnf->options;



	nf_udpPostReceive(id, remoteAddress, buf, buflen, options);
}

void NetFilter::postEvent(shared_ptr<evt_head> req)
{
	if (req != nullptr)
	{
		req->source = evt_source::netfiltersdk;
		_evt.post(req);
	}

}

bool NetFilter::GetUdpConninfo(ENDPOINT_ID id, OUT NF_UDP_CONN_INFO &info)
{
	{
		shared_lock<shared_mutex> locker(_udpinfo_lock);

		auto it = _udpinfo.find(id);
		if (it != _udpinfo.end())
		{
			info = it->second;
			return true;
		}
	}

	return false;
}

shared_ptr<evt_udp_send> NetFilter::createUdpSendEvent(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options)
{

	/*
	*	|	evt 	 |	  ud    |  buf   |
	*	|evt_udp_send|evt_udp_nf|senddata|
	*/

	NF_UDP_CONN_INFO info = {};

	if (!GetUdpConninfo(id, OUT info))
		return nullptr;

	int optlen = sizeof(NF_UDP_OPTIONS) + options->optionsLength - 1;	//opt len
	int udlen = sizeof(evt_udp_nf) + optlen;							//附加长度=evtnf + opt;
	int totallen = sizeof(evt_udp_send) + udlen + len;				//总长度=evt+附加长度+send长度
	shared_ptr<evt_udp_send> req((evt_udp_send*)new char[totallen]);

	req->type = evt_type::udp_send;
	req->family = ((sockaddr*)remoteAddress)->sa_family;
	req->pid = info.processId;
	req->totallen = totallen;											//总长度
	req->udlen = udlen;													//附加长度
	req->sendlen = len;													//send长度

	if (req->family == AF_INET)
	{
		req->src.si4 = *(sockaddr_in*)info.localAddress;
		req->dst.si4 = *(sockaddr_in*)remoteAddress;
	}
	else
	{
		req->src.si6 = *(sockaddr_in6*)info.localAddress;
		req->dst.si6 = *(sockaddr_in6*)remoteAddress;
	}

	//复制send data
	memcpy(req->buf + req->udlen, buf, req->sendlen);

	//复制options
	evt_udp_nf* evt_nf = (evt_udp_nf*)req->buf;
	evt_nf->nfid = id;
	memcpy(evt_nf->options, options, optlen);

	return req;
}

