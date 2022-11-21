#pragma once

#define _WIN32_WINDOWS 0x0601

#include <iostream>
#include <thread>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <evntrace.h>
#include <psapi.h>
#include <direct.h>
#include <mutex>
#include <evntcons.h>
#include <devguid.h>
#include <map>
#include <shared_mutex>
#include <functional>
#include <concurrentqueue.h>
#include <netfiltersdk2/include/nfapi.h>
#include <asio.hpp>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Rpcrt4.lib")
#pragma comment(lib,"netfiltersdk2/lib/Win32/MT/nfapi.lib")

using namespace nfapi;
using namespace std;
using namespace asio;
using namespace asio::ip;

#include <shared/shareddefines.h>
#include "defines.h"
#include <shared/Utils.h>
#include <shared/SharedMemory.h>
#include "ClientEvent.h"
#include "NetFilter.h"


#pragma comment(lib,"winmm.lib")