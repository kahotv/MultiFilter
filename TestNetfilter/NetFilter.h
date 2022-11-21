#pragma once
class NetFilter : public NF_EventHandler, public IEventHandler
{
public:
	NetFilter(io_context &ctx, uint16_t forward_port, uint16_t event_port);
	~NetFilter();

	// Í¨¹ý NF_EventHandler ¼Ì³Ð
	virtual void threadStart() override;
	virtual void threadEnd() override;
	virtual void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo) override;
	virtual void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo) override;
	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo) override;
	virtual void tcpReceive(ENDPOINT_ID id, const char* buf, int len) override;
	virtual void tcpSend(ENDPOINT_ID id, const char* buf, int len) override;
	virtual void tcpCanReceive(ENDPOINT_ID id) override;
	virtual void tcpCanSend(ENDPOINT_ID id) override;
	virtual void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo) override;
	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq) override;
	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo) override;
	virtual void udpReceive(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options) override;
	virtual void udpSend(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options) override;
	virtual void udpCanReceive(ENDPOINT_ID id) override;
	virtual void udpCanSend(ENDPOINT_ID id) override;

	virtual void OnEventReplay(const evt_head *resp) override;

private:
	void onTcpConnectReplay(const evt_tcpnew_resp* resp);
	void onUdpRecvReplay(const evt_udp_recv* resp);

	void postEvent(shared_ptr<evt_head> req);

	bool GetUdpConninfo(ENDPOINT_ID id, OUT NF_UDP_CONN_INFO &info);

	shared_ptr<evt_udp_send> createUdpSendEvent(ENDPOINT_ID id, const unsigned char* remoteAddress, const char* buf, int len, PNF_UDP_OPTIONS options);
private:
	//SOCKET _sock_evt;

	//std::mutex _evtmap_lock;
	//std::map<ENDPOINT_ID, evt_new_tcp_stream*> _evtmap;
	//moodycamel::ConcurrentQueue<evt_new_tcp_stream*> _evtqueue;

	//std::thread _th;

	std::shared_mutex _udpinfo_lock;
	std::map<ENDPOINT_ID, NF_UDP_CONN_INFO> _udpinfo;

	sockaddr_in _forward_addr_v4;
	sockaddr_in6 _forward_addr_v6;

	ClientEvent _evt;
};