劫持模块与代理模块分离，从而实现多劫持模块指向同一个代理模块，增加劫持成功率。

目前只实现了NetFilterSDK的劫持，待添加LSP和WINTUN的劫持。

初版代码很乱，TCP和UDP转发已实现，测试APEX可以正常玩，TCP Close还需要完善。

## Build

vs2015 Release x86

## Use

1. Install Driver    `"third_party\netfiltersdk2\bin\install*.bat"`
2. Run TestProxy.exe、TestNetfilter.exe
3. Run apex.exe

   ```cpp
   bool OpenNetfilter(NetFilter* h)
   {
   	bool b = true;
   	NF_STATUS r;
   	vector<wstring> pnames = { L"r5apex.exe"/*, L"EADesktop.exe"*/ };
   	...
   }
   ```

   

## Support

- TCP/UDP Local Proxy

~~~mermaid
graph LR
LSP --> MultiFilter
Netfilter --> MultiFilter
Wintun --> MultiFilter
... --> MultiFilter
MultiFilter --> GameServer
~~~

~~~mermaid
graph LR
LSP --> FilterLsp --succ--> MultiFilter
FilterLsp --fail-->Netfilter

Netfilter -->FilterNF --succ--> MultiFilter
FilterNF --fail-->Wintun
Wintun -->FilterWT --succ--> MultiFilter
FilterWT --fail-->Direct

Direct --> GameServer
MultiFilter --> GameServer

FilterLsp{filter}
FilterNF{filter}
FilterWT{filter}
~~~

~~~mermaid

~~~

