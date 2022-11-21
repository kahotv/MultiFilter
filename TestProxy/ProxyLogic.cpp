#include "stdafx.h"
#include "ProxyLogic.h"


bool ProxyLogic::IsCDN(std::shared_ptr<SessionTcp> cli, sockaddr_full& new_dst, bool &acc)
{
	return false;
}
bool ProxyLogic::IsCN2(std::shared_ptr<SessionTcp> cli, sockaddr_full& new_dst, bool &acc)
{
	return false;
}