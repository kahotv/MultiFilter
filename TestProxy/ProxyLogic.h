#pragma once
class ProxyLogic
{
public:
	static bool IsCDN(std::shared_ptr<SessionTcp> cli, sockaddr_full& new_dst, bool &acc);
	static bool IsCN2(std::shared_ptr<SessionTcp> cli, sockaddr_full& new_dst, bool &acc);

};

