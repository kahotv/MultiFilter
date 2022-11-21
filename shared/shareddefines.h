#pragma once

#define SERVER_TCP_BUFFER		0x1000	//���ֽڣ�SessonTcp��buffer�ߴ磨˫��

#define EVENT_BUFF_SIZE			0x10000	//���ֽڣ��¼�����������
#define HEART_INTERVAL_MS		5000;	//�����룩�������ʱ��
#define HEART_ERROR_TIMES_MAX	3		//���Σ������������������������proxy���ѹرգ�ֹͣת��


#define HEART_SM_NAME			L"sm_heart_evt_testproxy"	//���������ڴ�����
#define HEART_SM_SIZE			0x1000						//���������ڴ�ߴ�

#pragma pack(push,1)
struct sm_event_info
{
	uint16_t forward_port;	//ת���˿�
	uint16_t event_port;	//�¼��˿� 
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

//�¼�����
enum class evt_type : uint8_t
{
	unknown = 0x00,
	heart,
	tcp_conn_request,
	tcp_conn_replay,
	udp_send,
	udp_recv,
};

//����Դ
enum class evt_source : uint8_t
{
	unknown = 0x00,
	lsp,
	netfiltersdk,
	wintun,
};

struct evt_head
{
	uint16_t	totallen;		//�ܳ���
	uint16_t	evt_id;			//�¼�ID
	evt_type	type;			//�¼�����
	evt_source	source;			//����Դ
};

struct evt_heart_req : evt_head
{
	uint64_t time;				//����ʱ��
};
struct evt_heart_resp :evt_head
{
	uint64_t time;				//�ظ�ʱ��
};

//��TCP�¼�����ת���߸�֪����ģ���Լ�����Ϣ(����ʽ)
struct evt_tcpnew_req : evt_head
{
	uint16_t		family;		//AF_INET or AF_INET6
	uint32_t		pid;		//����ID
	sockaddr_full	src;		//�����ַ
	sockaddr_full	dst;		//Ŀ���ַ
	uint16_t		udlen;		//�û����ݳ���
	uint8_t			ud[0];		//�û�����
};

struct evt_tcpnew_resp : evt_head
{
	uint16_t		family;		//AF_INET or AF_INET6
	uint32_t		pid;		//����ID
	sockaddr_full	src;		//�����ַ
	sockaddr_full	dst;		//Ŀ���ַ
	bool			redirect;	//�Ƿ�ת��
	uint16_t		udlen;		//�û����ݳ���
	uint8_t			ud[0];		//�û�����
};

struct evt_udp_send :evt_head
{
	uint16_t		family;
	uint32_t		pid;
	sockaddr_full	src;		//�����ַ
	sockaddr_full	dst;		//Ŀ���ַ
	uint16_t		udlen;		//�û����ݳ���
	uint16_t		sendlen;	//send���ݳ���
	uint8_t			buf[0];		//ud+send
};

struct evt_udp_recv :evt_head
{
	uint16_t		family;
	sockaddr_full	src;		//�����ַ
	sockaddr_full	dst;		//Ŀ���ַ
	uint16_t		udlen;		//�û����ݳ���
	uint16_t		recvlen;	//recv���ݳ���
	uint8_t			buf[0];		//ud+recv
};

#pragma pack(pop)

//TCP��UDPʹ
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
	//�¼�����ص�������ServerEvent�յ��¼������֪ͨServerTcp
	virtual void OnEventRequest(const evt_head* req) {};
	//�¼��ظ��ص�������ServerClient�յ��¼��ظ���֪ͨFilter
	virtual void OnEventReplay(const evt_head* resp) {};
};


class IEventPoster
{
public:
	virtual void SendToFilter(const std::shared_ptr<evt_head> evt) = 0;
};
