#pragma once

#define _WIN32_WINDOWS 0x0601

#include <set>
#include <map>
#include <thread>
#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <asio.hpp>
#include <WinSock2.h>
#include <Windows.h>
#include <timeapi.h>

class SessionTcp;
class ProxyLogic;

#include <shared/shareddefines.h>
#include "defines.h"
#include <shared/Utils.h>
#include <shared/SharedMemory.h>
#include "ProxyLogic.h"
#include "ServerEvent.h"
#include "SessionTcp.h"
#include "SessionUdp.h"
#include "ServerTcp.h"
#include "ServerUdp.h"

#pragma comment(lib,"winmm.lib")