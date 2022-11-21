#pragma once
class Utils
{
public:
	static void InitNetwork();
	static const char* GetFamilyString(uint16_t family);
	static string IPGetEndpoint(const sockaddr* sa);
	static uint64_t GetNowTime();
	static uint64_t TimeGetTime64();
};

