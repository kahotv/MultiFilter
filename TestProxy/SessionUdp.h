#pragma once
class SessionUdp : public IUdpProcesspor, public enable_shared_from_this<SessionUdp>
{
public:
	SessionUdp(io_context &ctx, IEventPoster &evt);
	~SessionUdp();


	// ͨ�� IUdpProcesspor �̳�
	virtual void onUdpCreated(shared_ptr<conn_info> conn) override;
	virtual void onUdpClosed() override;
	virtual void onUdpSend(const uint8_t* buf, size_t size) override;
	virtual void onUdpRecv(const uint8_t* buf, size_t size) override;
	virtual void setId(uint64_t id) override;
	virtual void setOptions(uint8_t* buf, size_t size) override;
	virtual uint64_t getId() override;
	virtual void updateActive() override;
	virtual uint64_t getLastActive() override;

private:
	void doConnectRemote();
	void doRecvRemote();
	void onRecvRemote(size_t size);
private:
	io_context &_ctx;
	udp::socket _sock_remote;				//���ڷ���remote��socket

	udp::endpoint _ep_remote;					//Զ�˵�ַ

	uint64_t _id = 0;						//ΨһID
	uint64_t _last_active_time_ms = 0;		//���һ�λ�Ծʱ��
	shared_ptr<conn_info> _conn;			//������Ϣ


	size_t _buf_local_pos = 0;				//local��buffer����ʼλ�ã���Ϊ0ʱ�����������Э��ͷ
	uint8_t _buf_local[0x10000];			//local��buffer
	uint8_t _buf_remote[0x10000];			//remote��buffer

	IEventPoster &_evt;

	vector<uint8_t> _ud;					// user data
};

