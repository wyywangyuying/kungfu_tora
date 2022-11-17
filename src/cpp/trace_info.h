//

#ifndef KUNGFU_TRACE_INFO_H
#define KUNGFU_TRACE_INFO_H

#if (defined(_WIN32) || defined(_WIN64))
#include <atlconv.h>
#include <atomic>
#include <chrono>
#include <comdef.h>
#include <comutil.h>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <intrin.h>
#include <iostream>
#include <iphlpapi.h>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <tchar.h>
#include <thread>
//
#include <WbemIdl.h>
//
#include <Windows.h>
// 在KungFu外需要include这个文件，在KungFu内不能include，否则会出现重定义冲突
#include <WinSock2.h>
#include <nb30.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "wbemuuid.lib")

#elif defined(__linux__)
#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <linux/if.h>
#include <netdb.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace kungfu::wingchun::tora {
#if (defined(_WIN32) || defined(_WIN64))
typedef struct _ASTAT_ {
  ADAPTER_STATUS adapt;
  NAME_BUFFER NameBuff[30];
} ASTAT, *PASTAT;
#endif
class TraceInfo {
public:
  TraceInfo();

  std::string get_trace_info();
  std::string get_local_ip();
  std::string get_mac();
  std::string get_cpu_id();
  std::string get_pcn();
  std::string get_pc_type();
  std::string get_ext_info();
  std::string get_hd_sn();

  std::string info_; // 留痕总信息，使用其余信息拼接
  // 其他留痕信息
  std::string pc_type_; // *PC/OH;                  1     系统类型
  std::string lip_;     // *LIP=192.168.118.107;    1     内网IP LIP LAN IP，取值局域网IP地址
  std::string iip_; // *IIP=123.112.154.118;    NA   公网IP	IIP	Internet IP，取值访问互联网时分配的IP地址
  std::string iport_;    // *IPORT=50361;            NA   公网IP端口号	IPORT	Internet
                         // Port，取值交易终端软件访问互联网时对应公网IP的源端口号
  std::string mac_;      // *MAC=54EE750B1713;       1     MAC地址 MAC
  std::string hd_sn_;    // *HD=TF655AY91GHRVL;      1     硬盘序列号	HD	Hard Disk，取值硬盘序列号
  std::string cpu_id_;   // *CPU=bfebfbff00040651;   1     CPU序列号	CPU	取值CPU序列号
  std::string pcn_;      // *PCN=XXJSB-ZHUANGSHANG;       PC终端设备名	PCN	Personal Computer Name
  std::string ext_info_; // *拓展信息                 1
#if defined(__linux__)
private:
  int getInfoFromCmd(char *buf, const char *cmd, int bufSize);
#endif

#if (defined(_WIN32) || defined(_WIN64))
  NCB ncb;
  ASTAT Adapter;

  std::string pi_;  // 硬盘分区信息(仅windows)
  std::string vol_; // 系统盘卷标号(仅windows)
  std::string get_system_pi();
  std::string get_volume();
#endif
};
} // namespace kungfu::wingchun::tora

#endif // KUNGFU_TRACE_INFO_H