#pragma once

using namespace nfapi;
using namespace std;
using namespace asio;


#pragma pack(push,1)
struct evt_tcpnew_req_nf
{
	uint64_t			nfid;			//NFID
	NF_TCP_CONN_INFO	conninfo;		//��������
};

struct evt_udp_nf
{
	uint64_t			nfid;			//NFID
	uint8_t				options[0];		//postRecv��
};

#pragma pack(pop)