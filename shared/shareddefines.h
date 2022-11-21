#pragma once

#define SERVER_TCP_BUFFER		0x1000	//（字节）SessonTcp的buffer尺寸（双向）

#define EVENT_BUFF_SIZE			0x10000	//（字节）事件缓冲区长度
#define HEART_INTERVAL_MS		5000;	//（毫秒）心跳间隔时间
#define HEART_ERROR_TIMES_MAX	3		//（次）心跳最大错误计数，超过则当做proxy端已关闭，停止转发


#define HEART_SM_NAME			L"sm_heart_evt_testproxy"	//心跳共享内存名称
#define HEART_SM_SIZE			0x1000						//心跳共享内存尺寸

#pragma pack(push,1)
struct sm_event_info
{
	uint16_t forward_port;	//转发端口
	uint16_t event_port;	//事件端口 
};

struct sockaddr_full
{
	union
	{
		uint16_t family;
		sockaddr_in si4;
		sockaddr_in6 si6;
	};

	bool operator <(const sockaddr_full& c2) const
	{
		const auto& c1 = *this;
		if (c1.family != c2.family)
			return c1.family < c2.family;

		//c1.family == c2.family

		if (c1.family == AF_INET)
		{
			if (c1.si4.sin_addr.S_un.S_addr != c2.si4.sin_addr.S_un.S_addr)
				return c1.si4.sin_addr.S_un.S_addr < c2.si4.sin_addr.S_un.S_addr;

			return c1.si4.sin_port < c2.si4.sin_port;
		}
		else if (c1.family == AF_INET6)
		{
			uint32_t* p1 = (uint32_t*)c1.si6.sin6_addr.u.Byte;
			uint32_t* p2 = (uint32_t*)c2.si6.sin6_addr.u.Byte;

			if (p1[0] < p2[0] ||
				p1[1] < p2[1] ||
				p1[2] < p2[2] ||
				p1[3] < p2[3])
			{
				return true;
			}

			return c1.si6.sin6_port < c2.si6.sin6_port;
		}

		return false;
	}
	bool operator !=(const sockaddr_full& c2) const
	{
		const auto& c1 = *this;

		if (c1.family != c2.family)
			return true;

		if (c1.si4.sin_addr.S_un.S_addr != c2.si4.sin_addr.S_un.S_addr)
			return true;

		return c1.si4.sin_port != c2.si4.sin_port;
	}
};

//事件类型
enum class evt_type : uint8_t
{
	unknown = 0x00,
	heart,
	tcp_conn_request,
	tcp_conn_replay,
	udp_send,
	udp_recv,
};

//数据源
enum class evt_source : uint8_t
{
	unknown = 0x00,
	lsp,
	netfiltersdk,
	wintun,
};

struct evt_head
{
	uint16_t	totallen;		//总长度
	uint16_t	evt_id;			//事件ID
	evt_type	type;			//事件类型
	evt_source	source;			//数据源
};

struct evt_heart_req : evt_head
{
	uint64_t time;				//发起时间
};
struct evt_heart_resp :evt_head
{
	uint64_t time;				//回复时间
};

//新TCP事件，由转发者告知中心模块自己的信息(流格式)
struct evt_tcpnew_req : evt_head
{
	uint16_t		family;		//AF_INET or AF_INET6
	uint32_t		pid;		//进程ID
	sockaddr_full	src;		//发起地址
	sockaddr_full	dst;		//目标地址
	uint16_t		udlen;		//用户数据长度
	uint8_t			ud[0];		//用户数据
};

struct evt_tcpnew_resp : evt_head
{
	uint16_t		family;		//AF_INET or AF_INET6
	uint32_t		pid;		//进程ID
	sockaddr_full	src;		//发起地址
	sockaddr_full	dst;		//目标地址
	bool			redirect;	//是否转向
	uint16_t		udlen;		//用户数据长度
	uint8_t			ud[0];		//用户数据
};

struct evt_udp_send :evt_head
{
	uint16_t		family;
	uint32_t		pid;
	sockaddr_full	src;		//发起地址
	sockaddr_full	dst;		//目标地址
	uint16_t		udlen;		//用户数据长度
	uint16_t		sendlen;	//send数据长度
	uint8_t			buf[0];		//ud+send
};

struct evt_udp_recv :evt_head
{
	uint16_t		family;
	sockaddr_full	src;		//发起地址
	sockaddr_full	dst;		//目标地址
	uint16_t		udlen;		//用户数据长度
	uint16_t		recvlen;	//recv数据长度
	uint8_t			buf[0];		//ud+recv
};

#pragma pack(pop)

//TCP或UDP使
struct conn_info
{
	sockaddr_full src;
	sockaddr_full dst;
	bool operator <(const conn_info& c2) const
	{
		const auto& c1 = *this;
		if (c1.src != c2.src)
			return c1.src < c2.src;
		return c1.dst < c2.dst;
	}
};

class IEventHandler
{
public:
	//事件请求回调，用于ServerEvent收到事件请求后通知ServerTcp
	virtual void OnEventRequest(const evt_head* req) {};
	//事件回复回调，用于ServerClient收到事件回复后通知Filter
	virtual void OnEventReplay(const evt_head* resp) {};
};


class IEventPoster
{
public:
	virtual void SendToFilter(const std::shared_ptr<evt_head> evt) = 0;
};
