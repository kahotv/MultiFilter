#include "stdafx.h"
#include "Utils.h"


void Utils::InitNetwork()
{
	WSAData data;
	WSAStartup(0x0202, &data);
}

const char* Utils::GetFamilyString(uint16_t family)
{
	if (family == AF_INET)
		return "IPv4";
	if (family == AF_INET6)
		return "IPv6";
	return "";
}

string Utils::IPGetEndpoint(const sockaddr* sa)
{
	if (sa == nullptr)
	{
		return "地址为NULL";
	}

	if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)
	{
		sockaddr_full addr;
		if (sa->sa_family == AF_INET)
			addr.si4 = *(sockaddr_in*)sa;
		else
			addr.si6 = *(sockaddr_in6*)sa;

		addr.family = sa->sa_family;

		char strAddr[64] = { 0 };
		DWORD dwLen = sizeof(strAddr);
		WSAAddressToStringA((sockaddr*)sa, sa->sa_family == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in), NULL, strAddr, &dwLen);

		return strAddr;
	}
	else
	{
		return "不支持的网络版本";
	}
}

uint64_t Utils::GetNowTime()
{
	static bool succ = false;
	static bool inited = false;
	static mutex locker;
	static LARGE_INTEGER iFrequency = {};
	if (!inited)
	{
		locker.lock();
		if (!inited)
		{
			if (QueryPerformanceFrequency(&iFrequency)) 
			{
				succ = true;
			}
			inited = true;
		}
		locker.unlock();
	}

	if (succ)
	{
		uint64_t currTime;
		LARGE_INTEGER iCounter;

		QueryPerformanceCounter(&iCounter);
		currTime = (iCounter.QuadPart * 1000) / iFrequency.QuadPart;

		return currTime;
	}
	else 
	{
		return TimeGetTime64();
	}
}

uint64_t Utils::TimeGetTime64()
{
	uint64_t tick = GetTickCount64();    // 10-16ms tolerance
	uint64_t time = timeGetTime();       // 1ms tolerance, but 49.5 days limit.

										// bias 4096ms [0-4095]
										// if equal, return directly.
	uint64_t result = (tick & 0xFFFFFFFFFFFFF000) | (time & 0x0000000000000FFF);

	// compare result with tick
	if (result > tick)
	{
		uint64_t diff;
		tick > time ? diff = tick - time : diff = time - tick;

		if (result - tick > diff)
		{
			result -= 0x1000;
		}
	}
	else
		if (result < tick)
		{
			uint64_t diff;
			tick > time ? diff = tick - time : diff = time - tick;

			if (tick - result > diff)
			{
				result += 0x1000;
			}
		}
	return result;
}