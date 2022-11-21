#pragma once

//��������Filter��UDP���ݣ�����SessionUdp



class ServerUdp : public IEventHandler
{
public:
	ServerUdp(io_context& ctx, IEventPoster &evt, uint16_t port_begin, uint16_t port_end);
	~ServerUdp();

	bool Start();
	void Stop();
	uint16_t GetPort() const;

	// ͨ�� IEventHandler �̳�
	//�¼�����ص�������ServerEvent�յ��¼������֪ͨServerTcp
	virtual void OnEventRequest(const evt_head* req) override;
	//�¼��ظ��ص�������ServerClient�յ��¼��ظ���֪ͨNetfilter
	virtual void OnEventReplay(const evt_head* resp) {};

private:

	void onUdpSend(evt_udp_send* evt);
	/*
	void doRecvLocal4()
	{
		if (_run)
		{
			_listener4.async_receive_from(buffer(_buf4, sizeof(_buf4)), _ep_filter4,
				[this](asio::error_code ec, size_t size)
			{
				if (!ec)
				{
					onRecvLocal(AF_INET, size);
					doRecvLocal4();
				}
			});
		}
	}

	void doRecvLocal6()
	{
		if (_run)
		{
			_listener6.async_receive_from(buffer(_buf6, sizeof(_buf6)), _ep_filter6,
				[this](asio::error_code ec, size_t size)
			{
				if (!ec)
				{
					onRecvLocal(AF_INET6, size);
					doRecvLocal6();
				}
			});
		}
	}
	/*
	void onRecvLocal(int family, size_t size)
	{
		const char* afv = Utils::GetFamilyString(family);
		const udp::endpoint &src = family == AF_INET ? _ep_filter4 : _ep_filter6;
		const char* buf = family == AF_INET ? _buf4 : _buf6;

		if (size < sizeof(evt_udp_send))
		{
			printf("[ServerUdp][%s] onRecvLocal size: %d | from: %s:%d | ���������\n", afv, size, src.address().to_string().c_str(), src.port());
			return;
		}
		else 
		{
			//printf("[ServerUdp][%s] onRecvLocal size: %d | from: %s:%d\n", afv, size, src.address().to_string().c_str(), src.port());
		}

		evt_udp_send* up = (evt_udp_send*)buf;

		//TODO �߼��ж�
		conn_info conn = fillConnInfo(family, src, up->dst);
		shared_ptr<udp_processor> processor = getUdpProcessor(conn);

		if (processor == nullptr)
		{
			//����UDP����ʵ��
			processor = createUdpProcessor();
			processor->conn = make_shared<conn_info>(conn);
			processor->processor->setId(getUniqueId());

			setUdpProcessor(conn, processor);

			processor->processor->onUdpCreated(processor->conn);

			//�������
			doUpdateActive(processor);
		}

		//��������
		processor->processor->onUdpSend(up->ud, up->udlen);

		//ˢ�»�Ծʱ��
		processor->processor->updateActive();
	}
	*/
private:

	conn_info fillConnInfo(int family,const sockaddr_full &src, const  sockaddr_full& dst) const
	{
		conn_info conn = {};

		if (family == AF_INET)
		{
			conn.src.si4.sin_family = AF_INET;
			conn.src.si4.sin_addr = src.si4.sin_addr;
			conn.src.si4.sin_port = src.si4.sin_port;
				
			conn.dst.si4.sin_family = AF_INET;
			conn.dst.si4.sin_port = dst.si4.sin_port;
			conn.dst.si4.sin_addr = dst.si4.sin_addr;
		}		
		else	
		{		
			conn.src.si6.sin6_family = AF_INET6;
			conn.src.si6.sin6_addr = src.si6.sin6_addr;
			conn.src.si6.sin6_port = src.si6.sin6_port;
				
			conn.dst.si6.sin6_family = AF_INET6;
			conn.dst.si6.sin6_port = dst.si6.sin6_port;
			conn.dst.si6.sin6_addr = dst.si6.sin6_addr;
		}

		return conn;
	}
	uint64_t getUniqueId()
	{
		static uint64_t static_index = 0;
		uint64_t index = 0;

		do
		{
			index = InterlockedIncrement(&static_index);

		} while (index == 0 /*|| index == ICMPProxyID || index == ICMPLocalID*/);

		return index;
	}


	shared_ptr<udp_processor> createUdpProcessor()
	{
		shared_ptr<udp_processor> obj = make_shared<udp_processor>();
		obj->processor = make_shared<SessionUdp>(_ctx, _evt);
		obj->timer = make_shared<steady_timer>(_ctx);
		return obj;
	}
	shared_ptr<udp_processor> getUdpProcessor(const conn_info &conn)
	{
		auto it = _udp_mgr.find(conn);
		if (it != _udp_mgr.end())
		{
			//����
			return it->second;
		}

		return nullptr;
	}
	void setUdpProcessor(const conn_info &conn, shared_ptr<udp_processor> processor)
	{
		_udp_mgr[conn] = processor;
	}
	void delUdpProcessor(const conn_info &conn)
	{
		_udp_mgr.erase(conn);
	}


	void doUpdateActive(shared_ptr<udp_processor> processor)
	{
		processor->timer->expires_after(asio::chrono::milliseconds(10 * 1000));
		processor->timer->async_wait([this, processor](asio::error_code ec)
		{
			uint64_t now = Utils::GetNowTime();
			uint64_t last = processor->processor->getLastActive();
			if ((now - last) >= 20 * 1000)
			{
				//UDP���ӳ�ʱ���ر�
				processor->processor->onUdpClosed();
				//�Ƴ�processor
				delUdpProcessor(*processor->conn);
			}
			else 
			{
				//�������
				doUpdateActive(processor);
			}
		});
	}

private:
	io_context &_ctx;

	udp::socket _listener4, _listener6;
	uint16_t _port_begin, _port_end;	//���ؼ����˿� ��ʼ�ͽ���
	uint16_t _port = 0;					//���ؼ����˿� ʵ��

	udp::endpoint _ep_filter4, _ep_filter6;

	char _buf4[0x10000];
	char _buf6[0x10000];

	bool _run = false;

	map<conn_info, shared_ptr<udp_processor>> _udp_mgr;

	IEventPoster &_evt;
};

