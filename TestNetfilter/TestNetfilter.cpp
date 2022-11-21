#include "stdafx.h"
#include "NetFilter.h"


bool OpenNetfilter(NetFilter* h)
{
	bool b = true;
	NF_STATUS r;
	vector<wstring> pnames = { L"r5apex.exe"/*, L"EADesktop.exe"*/ };

	nf_setOptions(1, 0);
	r = nf_init("netfilter2", h);
	printf("nf_init %d\n", r);

	NF_RULE_EX rule = {};

	for(size_t i = 0;i < pnames.size();i++)
	{
		const auto &pname = pnames[i];

		{
			//TCP
			NF_RULE_EX rule = {};
			rule.protocol = IPPROTO_TCP;
			rule.direction = NF_D_OUT;
			rule.filteringFlag = NF_INDICATE_CONNECT_REQUESTS | NF_PEND_CONNECT_REQUEST;
			lstrcpyW(rule.processName, pname.c_str());
			r = nf_addRuleEx(&rule, TRUE);
			printf("[OpenNetfilter]设置TCP劫持 %d %ws\n", r, pname.c_str());
			b &= (r == NF_STATUS_SUCCESS);
		}
		{
			//UDP
			NF_RULE_EX rule = {};
			rule.protocol = IPPROTO_UDP;
			rule.direction = NF_D_OUT;
			rule.filteringFlag = NF_FILTER;
			lstrcpyW(rule.processName, pname.c_str());
			r = nf_addRuleEx(&rule, TRUE);
			b &= (r == NF_STATUS_SUCCESS);
			printf("[OpenNetfilter]设置UDP劫持 %d %ws\n", r, pname.c_str());
		}
	}


	{
		//DNS
		NF_RULE_EX rule = {};
		rule.protocol = IPPROTO_UDP;
		rule.direction = NF_D_OUT;
		rule.filteringFlag = NF_FILTER;
		rule.remotePort = htons(53);
		r = nf_addRuleEx(&rule, TRUE);
		b &= (r == NF_STATUS_SUCCESS);
		printf("[OpenNetfilter]设置DNS劫持 %d\n", r);
	}

	{
		//回环地址数据包不过滤 由于指定了IP，所以需要指定family
		NF_RULE_EX rule = {};
		rule.filteringFlag = NF_ALLOW;
		rule.direction = NF_D_BOTH;
		rule.ip_family = AF_INET;
		(in_addr&)rule.remoteIpAddress = in4addr_loopback;
		r = nf_addRuleEx(&rule, TRUE);
		b &= (r == NF_STATUS_SUCCESS);

		rule.ip_family = AF_INET6;
		(in6_addr&)rule.remoteIpAddress = in6addr_loopback;
		r = nf_addRuleEx(&rule, TRUE);
		b &= (r == NF_STATUS_SUCCESS);
	}

	{
		//IPv4广播地址直接放行
		NF_RULE_EX rule = {};
		rule.filteringFlag = NF_ALLOW;
		rule.direction = NF_D_BOTH;
		rule.ip_family = AF_INET;
		(in_addr&)rule.remoteIpAddress = in4addr_broadcast;
		r = nf_addRuleEx(&rule, TRUE);
		b &= (r == NF_STATUS_SUCCESS);
	}


	{
		//自身进程直接放行
		NF_RULE_EX rule = {};
		rule.filteringFlag = NF_ALLOW;
		rule.direction = NF_D_BOTH;
		rule.processId = GetCurrentProcessId();
		r = nf_addRuleEx(&rule, TRUE);
		b &= (r == NF_STATUS_SUCCESS);
	}

	return b;
}

bool WaitRead(SharedMemory &sm, sm_event_info &info, int timeout_ms, int step_ms)
{
	uint64_t s = Utils::GetNowTime();
	uint64_t e = s;
	bool succ = false;

	do
	{
		succ = sm.Read(info);
		if (succ)
			break;

		Sleep(step_ms);
		e = Utils::GetNowTime();
	} while ((e - s) <= timeout_ms);

	return succ;
}

int main()
{
	Utils::InitNetwork();

	io_context ctx;
	sm_event_info info = {};
	SharedMemory sm(HEART_SM_NAME, HEART_SM_SIZE);

	if (!WaitRead(sm, info, 10000, 1000)) 
	{
		printf("读取共享信息fail 转发端口: %d, 事件端口: %d\n", info.forward_port, info.event_port);
		system("pause");
		return 0;
	}
	printf("读取共享信息succ 转发端口: %d, 事件端口: %d\n", info.forward_port, info.event_port);

	NetFilter handler(ctx, info.forward_port, info.event_port);

	bool b = OpenNetfilter(&handler);
	printf("OpenNetfilter %s\n", b ? "succ" : "fail");

	ctx.run();

	system("pause");
	nf_free();
}