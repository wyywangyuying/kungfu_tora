#include "trace_info.h"
#include <kungfu/wingchun/common.h>
namespace kungfu::wingchun::tora {
#if (defined(_WIN32) || defined(_WIN64))
char *ConvertLPWSTRToLPSTR(LPWSTR lpwszStrIn) {
  LPSTR pszOut = NULL;
  if (lpwszStrIn != NULL) {
    int nInputStrLen = wcslen(lpwszStrIn);

    // Double NULL Termination
    int nOutputStrLen = WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, NULL, 0, 0, 0) + 2;
    pszOut = new char[nOutputStrLen];

    if (pszOut) {
      memset(pszOut, 0x00, nOutputStrLen);
      WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, pszOut, nOutputStrLen, 0, 0);
    }
  }
  return pszOut;
}
#endif

TraceInfo::TraceInfo() {
  get_local_ip();
  get_mac();
  get_cpu_id();
  get_pcn();
  get_pc_type();
  get_ext_info();
  get_hd_sn();
#if (defined(_WIN32) || defined(_WIN64))
  get_system_pi();
  get_volume();
#endif
}

std::string TraceInfo::get_pc_type() {
#if defined(__linux__)
  pc_type_ = "OH";
// #elif (defined(_WIN32) || defined(_WIN64))
#else
  pc_type_ = "PC";
#endif
  return pc_type_;
}

std::string TraceInfo::get_ext_info() {
  ext_info_ = "Kungfu_2.3";
  return ext_info_;
}

std::string TraceInfo::get_local_ip() {
#if (defined(_WIN32) || defined(_WIN64))
  WSADATA wsaData;
  char name[156];
  PHOSTENT hostinfo;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) == 0) {
    if (gethostname(name, sizeof(name)) == 0) {
      if ((hostinfo = gethostbyname(name)) != NULL) {
        std::string tmp = inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
        lip_ = tmp;
      }
    }
    WSACleanup();
  }

#elif defined(__linux__)

  int sd;
  struct sockaddr_in sin;
  struct ifreq ifr;

  std::string eth0 = "eth0";
  std::string eno1 = "eno1";

  const int IP_SIZE = 32;
  char ip[IP_SIZE] = {0};

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (-1 == sd) {
    printf("socket error: %s\n", strerror(errno));
    return lip_;
  }

  // eth0
  strncpy(ifr.ifr_name, eth0.c_str(), IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = 0;
  if (ioctl(sd, SIOCGIFADDR, &ifr) >= 0) {
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));
    close(sd);
    // cout << eth0 << ip << endl;
    lip_ = std::string(ip);
    return lip_;
  }

  // eno1
  strncpy(ifr.ifr_name, eno1.c_str(), IFNAMSIZ);
  ifr.ifr_name[IFNAMSIZ - 1] = 0;
  if (ioctl(sd, SIOCGIFADDR, &ifr) >= 0) {
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));
    close(sd);
    // cout << eno1 << ip << endl;
    lip_ = std::string(ip);
    return lip_;
  }
  if (lip_.empty()) {
    char buf[1024];
    int cmdRet = getInfoFromCmd(buf, "hostname -I | cut -d ' ' -f 1", 1024);
    if (cmdRet == 0) {
      buf[strcspn(buf, "\n")] = 0;
      lip_ = buf;
    }
  }

#endif
  return lip_;
}

std::string TraceInfo::get_mac() {
#if (defined(_WIN32) || defined(_WIN64))
  char tmp[256];
  LANA_ENUM lana_enum;
  UCHAR uRetCode;
  memset(&ncb, 0, sizeof(ncb));
  memset(&lana_enum, 0, sizeof(lana_enum));

  ncb.ncb_command = NCBENUM;
  ncb.ncb_buffer = (unsigned char *)&lana_enum;
  ncb.ncb_length = sizeof(LANA_ENUM);
  uRetCode = Netbios(&ncb);
  if (uRetCode != NRC_GOODRET) {
    return "";
  }

  for (int lana = 0; lana < lana_enum.length; lana++) {
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lana_num = lana_enum.lana[lana];
    uRetCode = Netbios(&ncb);
    if (uRetCode == NRC_GOODRET)
      break;
  }
  if (uRetCode != NRC_GOODRET) {
    return "";
  }

  memset(&ncb, 0, sizeof(ncb));
  ncb.ncb_command = NCBASTAT;
  ncb.ncb_lana_num = lana_enum.lana[0];
  strcpy_s((char *)ncb.ncb_callname, 2, "*");
  ncb.ncb_buffer = (unsigned char *)&Adapter;
  ncb.ncb_length = sizeof(Adapter);
  uRetCode = Netbios(&ncb);
  if (uRetCode != NRC_GOODRET) {
    return "";
  }
  sprintf_s(tmp, 18, "%02x%02x%02x%02x%02x%02x", Adapter.adapt.adapter_address[0], Adapter.adapt.adapter_address[1],
            Adapter.adapt.adapter_address[2], Adapter.adapt.adapter_address[3], Adapter.adapt.adapter_address[4],
            Adapter.adapt.adapter_address[5]);

  this->mac_ = std::string(tmp);

#elif defined(__linux__)

  struct ifreq ifr;
  int sd;
  std::string eth0 = "eth0";
  std::string eno1 = "eno1";

  const int MAC_SIZE = 32;
  char mac[MAC_SIZE] = {0};

  bzero(&ifr, sizeof(struct ifreq));

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return mac_;
  }

  strcpy(ifr.ifr_name, eth0.c_str());
  if (ioctl(sd, SIOCGIFHWADDR, &ifr) >= 0) {
    snprintf(mac, MAC_SIZE, "%02x%02x%02x%02x%02x%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[0],
             (unsigned char)ifr.ifr_hwaddr.sa_data[1], (unsigned char)ifr.ifr_hwaddr.sa_data[2],
             (unsigned char)ifr.ifr_hwaddr.sa_data[3], (unsigned char)ifr.ifr_hwaddr.sa_data[4],
             (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    close(sd);
    mac_ = mac;
    return mac_;
  }

  strcpy(ifr.ifr_name, eno1.c_str());
  if (ioctl(sd, SIOCGIFHWADDR, &ifr) >= 0) {
    snprintf(mac, MAC_SIZE, "%02x%02x%02x%02x%02x%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[0],
             (unsigned char)ifr.ifr_hwaddr.sa_data[1], (unsigned char)ifr.ifr_hwaddr.sa_data[2],
             (unsigned char)ifr.ifr_hwaddr.sa_data[3], (unsigned char)ifr.ifr_hwaddr.sa_data[4],
             (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
    close(sd);
    mac_ = mac;
    return mac_;
  }
  if (mac_.empty()) {
    char buf[1024];
    int cmdRet = getInfoFromCmd(buf, "cat /sys/class/net/`ls /sys/class/net/ | grep 'wl.*' | head -n1`/address", 1024);
    if (cmdRet == 0) {
      buf[strcspn(buf, "\n")] = 0;
      mac_ = buf;
    }
  }
#endif
  return mac_;
}

std::string TraceInfo::get_cpu_id() {
#if (defined(_WIN32) || defined(_WIN64))
  char tmp[64];
  INT32 deBuf[4];
  __cpuidex(deBuf, 01, 0);
  sprintf_s(tmp, 18, "%08x%08x", deBuf[3], deBuf[0]);
  this->cpu_id_ = std::string(tmp);

#elif defined(__linux__)

  char buf[1024];
  FILE *f = popen("dmidecode -s system-serial-number", "r");
  fgets(buf, sizeof(buf), f);
  pclose(f);

  buf[strcspn(buf, "\n")] = 0;
  cpu_id_ = buf;
#endif
  return cpu_id_;
}

std::string TraceInfo::get_pcn() {
#if (defined(_WIN32) || defined(_WIN64))
  TCHAR buf[MAX_COMPUTERNAME_LENGTH + 2];
  DWORD buf_size;
  buf_size = sizeof(buf) - 1;
  GetComputerName(buf, &buf_size);
  std::stringstream ss;
  ss << buf;
  pcn_ = ss.str();

#elif defined(__linux__)
  char name[256];
  gethostname(name, sizeof(name));
  pcn_ = name;
#endif
  return pcn_;
}

std::string TraceInfo::get_hd_sn() {
#if (defined(_WIN32) || defined(_WIN64))
  char bord[128];
  HRESULT hres;
  // Step 1:  初始化COM
  // hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  // //网上的代码都是使用这行语句进行初始化，但是我在实际使用中，发现也可以采用下面的语句进行初始化
  hres = CoInitialize(0);

  // Step 3:  获得WMI连接COM接口
  IWbemLocator *pLoc = NULL;
  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

  if (FAILED(hres)) {
    // std::cout << "Failed to create IWbemLocator object."
    // << " Err code = 0x" << std::hex << hres << std::endl;
    CoUninitialize();
    return ""; // Program has failed.
  }

  // Step 4:  通过连接接口连接WMI的内核对象名"ROOT//CIMV2"
  IWbemServices *pSvc = NULL;

  hres = pLoc->ConnectServer(

      _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
      NULL,                    // User name. NULL = current user
      NULL,                    // User password. NULL = current
      0,                       // Locale. NULL indicates current
      NULL,                    // Security flags.
      0,                       // Authority (e.g. Kerberos)
      0,                       // Context object
      &pSvc                    // pointer to IWbemServices proxy
  );

  if (FAILED(hres)) {
    std::cout << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
    pLoc->Release();
    CoUninitialize();
    return ""; // Program has failed.
  }
  // cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;

  // Step 5:  设置请求代理的安全级别
  hres = CoSetProxyBlanket(pSvc,                        // Indicates the proxy to set
                           RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                           RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                           NULL,                        // Server principal name
                           RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
                           RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                           NULL,                        // client identity
                           EOAC_NONE                    // proxy capabilities
  );
  if (FAILED(hres)) {
    std::cout << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return ""; // Program has failed.
  }
  // Step 6: 通过请求代理来向WMI发送请求----
  // For example, get the name of the operating system
  IEnumWbemClassObject *pEnumerator = NULL;
  hres =
      pSvc->ExecQuery(bstr_t("WQL"),
                      bstr_t("SELECT * FROM Win32_DiskDrive WHERE (SerialNumber IS NOT NULL) AND (MediaType LIKE "
                             "'Fixed hard disk%')"), //只需要通过修改这里的查询语句，就可以实现对MAC地址等其他信息的查询
                      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

  if (FAILED(hres)) {
    std::cout << "Query for Network Adapter Configuration failed."
              << " Error code = 0x" << std::hex << hres << std::endl;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return ""; // Program has failed.
  }

  // Step 7:  循环枚举所有的结果对象

  IWbemClassObject *pclsObj;
  ULONG uReturn = 0;

  bool success = false;
  while (pEnumerator) {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn) {
      break;
    }
    success = true;
    VARIANT vtProp;
    VariantInit(&vtProp);

    hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);

    if (!FAILED(hr)) {
      // CW2A tmpstr1(vtProp.bstrVal);
      int bordLen = strlen(_com_util::ConvertBSTRToString(vtProp.bstrVal)) + 1;
      strcpy_s(bord, bordLen, _com_util::ConvertBSTRToString(vtProp.bstrVal));
    }

    VariantClear(&vtProp);
  } // end while

  // 释放资源
  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  if (success) {
    pclsObj->Release();
  }
  CoUninitialize();

  std::string tmp = std::string(bord);
  tmp.erase(0, tmp.find_first_not_of(" "));
  hd_sn_ = tmp.empty() ? "TF655AY91GHRVL" : tmp;
#elif defined(__linux__)
  char buf[1024];
  int cmdRet = getInfoFromCmd(buf, "udevadm info --query=all --name=/dev/sda | grep ID_SERIAL_SHORT=", 1024);
  if (cmdRet == -1) {
    return hd_sn_;
  } else if (cmdRet == -2) {
    SPDLOG_INFO("/dev/sda no content read!");
    errno = 0;
    cmdRet = getInfoFromCmd(buf,
                            "udevadm info --query=all --name=/dev/`lsblk 2>/dev/null | grep disk | head -n1 | cut -d ' "
                            "' -f 1` | grep ID_SERIAL_SHORT=",
                            1024);
    if (cmdRet != 0) {
      return hd_sn_;
    }
  }

  buf[strcspn(buf, "\n")] = 0;
  hd_sn_ = buf + strcspn(buf, "=") + 1;
#endif
  return hd_sn_; // Program successfully completed.
}

#if defined(__linux__)
int TraceInfo::getInfoFromCmd(char *buf, const char *cmd, int bufSize) {
  memset(buf, 0, bufSize);
  errno = 0;
  FILE *f = popen(cmd, "r");
  if (f == nullptr) {
    SPDLOG_ERROR("open errno={}", errno);
    return -1;
  }
  char *pGet = fgets(buf, bufSize, f);
  pclose(f);
  if (pGet == nullptr) {
    SPDLOG_INFO("pGet get no content!");
    return -2;
  }
  return 0;
}
#endif

#if (defined(_WIN32) || defined(_WIN64))
std::string TraceInfo::get_system_pi() {
  DWORD dwSize = MAX_PATH;
  TCHAR szLogicalDrives[MAX_PATH] = {0};
  DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);
  const DWORD nVolumeNameSize = 128;
  CHAR lpVolumeNameBuffer[nVolumeNameSize];
  unsigned long lpVolumeSerialNumber = 128;
  unsigned long lpMaximumComponentLength = 128;
  unsigned long lpFileSystemFlags = 128;
  const DWORD nFileSystemNameSize = 128;
  CHAR lpFileSystemNameBuffer[nFileSystemNameSize];
  std::stringstream ss;

  if (dwResult > 0 && dwResult <= MAX_PATH) {
    TCHAR *szSingleDrive = szLogicalDrives;
    GetVolumeInformation(szSingleDrive, lpVolumeNameBuffer, nVolumeNameSize, &lpVolumeSerialNumber,
                         &lpMaximumComponentLength, &lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);

    uint64_t availabel, total, free;
    if (GetDiskFreeSpaceEx(szSingleDrive, (ULARGE_INTEGER *)&availabel, (ULARGE_INTEGER *)&total,
                           (ULARGE_INTEGER *)&free)) {
      uint64_t Total, Availabel, Free;
      Total = total >> 20;
      ss << szSingleDrive[0] << "^" << lpFileSystemNameBuffer << "^" << Total / 1024 << "G";
    }
  }
  pi_ = ss.str();
  return pi_;
}

std::string TraceInfo::get_volume() {
  char label[128];
  HRESULT hres;
  hres = CoInitialize(0);

  IWbemLocator *pLoc = NULL;
  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

  if (FAILED(hres)) {
    // SPDLOG_ERROR("Failed to create IWbemLocator object. {}", hres);
    CoUninitialize();
    return vol_; // Program has failed.
  }

  IWbemServices *pSvc = NULL;
  hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
                             NULL,                    // User name. NULL = current user
                             NULL,                    // User password. NULL = current
                             0,                       // Locale. NULL indicates current
                             NULL,                    // Security flags.
                             0,                       // Authority (e.g. Kerberos)
                             0,                       // Context object
                             &pSvc                    // pointer to IWbemServices proxy
  );

  if (FAILED(hres)) {
    // SPDLOG_ERROR("Could not connect. Error code = 0x , {}", hres);
    pLoc->Release();
    CoUninitialize();
    return vol_; // Program has failed.
  }

  // Step 5:  设置请求代理的安全级别
  hres = CoSetProxyBlanket(pSvc,                        // Indicates the proxy to set
                           RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                           RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                           NULL,                        // Server principal name
                           RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
                           RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                           NULL,                        // client identity
                           EOAC_NONE                    // proxy capabilities
  );
  if (FAILED(hres)) {
    // SPDLOG_ERROR("Could not set proxy blanket. Error code = 0x , {}", hres);
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return vol_; // Program has failed.
  }

  IEnumWbemClassObject *pEnumerator = NULL;
  hres = pSvc->ExecQuery(bstr_t("WQL"),
                         bstr_t("SELECT * FROM win32_logicaldisk where DeviceID=\"C:\""), //获取C盘卷标号
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

  if (FAILED(hres)) {
    // SPDLOG_ERROR("Query for Network Adapter Configuration failed. Error code = 0x{}", hres);
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return vol_; // Program has failed.
  }

  // Step 7:  循环枚举所有的结果对象

  IWbemClassObject *pclsObj;
  ULONG uReturn = 0;

  while (pEnumerator) {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn) {
      break;
    }
    VARIANT vtProp;
    VariantInit(&vtProp);

    // hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0,
    // 0);//查询不同的硬件信息，除了修改上面的查询语句，这里的字段也要修改
    hr = pclsObj->Get(L"VolumeSerialNumber", 0, &vtProp, 0, 0); //获取C盘卷标号
    if (!FAILED(hr)) {
      // CW2A tmpstr1(vtProp.bstrVal);
      int lenlabelLen = strlen(_com_util::ConvertBSTRToString(vtProp.bstrVal)) + 1;
      strcpy_s(label, lenlabelLen,
               _com_util::ConvertBSTRToString(
                   vtProp.bstrVal)); //这里的200是可调的，自己根据实际情况设置，但是肯定不能大于bord的长度
    }
    VariantClear(&vtProp);
  } // end while

  // 释放资源
  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  pclsObj->Release();
  CoUninitialize();

  TCHAR Root[] = _T("C:\\");
  CHAR Volumelabel[20];
  DWORD SerialNumber;
  DWORD MaxCLength;
  DWORD FileSysFlag;
  CHAR FileSysName[10];
  USES_CONVERSION;
  BOOL bResult =
      GetVolumeInformation(Root, Volumelabel, 255, &SerialNumber, &MaxCLength, &FileSysFlag, FileSysName, 255);
  // printf("卷标名称(Volumelabel) = %s\n", Volumelabel);
  // printf("卷标序列号(SerialNumber) = 0x%x\n", *(&SerialNumber));
  // printf("系统允许最大文件名长度(MaxCLength) = 0x%x\n", *(&MaxCLength));
  // printf("文件系统标识(FileSysFlag) = 0x%x\n", *(&FileSysFlag));
  // printf("文件系统名称(FileSysName) = %s\n", FileSysName);
  WideCharToMultiByte(CP_ACP, 0, A2W(Volumelabel), -1, Volumelabel, wcslen(A2W(Volumelabel)), 0, 0);
  Volumelabel[wcslen(A2W(Volumelabel))] = 0;
  std::string str(label);
  str.insert(4, "-");
  vol_ = str;
  return vol_; // Program successfully completed.
}

#endif

std::string TraceInfo::get_trace_info() {
  std::stringstream info;
  info << pc_type_ << ";";

  if (iip_.empty()) {
    info << "IIP=NA;";
  } else {
    info << "IIP=" << iip_ << ";";
  }

  if (iport_.empty()) {
    info << "IPORT=NA;";
  } else {
    info << "IPORT=" << iport_ << ";";
  }

  if (lip_.empty()) {
    info << "LIP=NA;";
  } else {
    info << "LIP=" << lip_ << ";";
  }

  if (mac_.empty()) {
    info << "MAC=NA;";
  } else {
    info << "MAC=" << mac_ << ";";
  }

  if (hd_sn_.empty()) {
    info << "HD=NA;";
  } else {
    info << "HD=" << hd_sn_ << ";";
  }

  // if (pcn_.empty())
  // {
  //   info << "PCN=NA;";
  // }
  // else
  // {
  //   info << "PCN=" << pcn_ << ";";
  // }

  // if (cpu_id_.empty())
  // {
  //   info << "CPU=NA;";
  // }
  // else
  // {
  //   info << "CPU=" << cpu_id_;
  // }

#if (defined(_WIN32) || defined(_WIN64))
  // if (pi_.empty())
  // {
  //   info << ";PI=NA;";
  // }
  // else
  // {
  //   info << ";PI=" << pi_ << ";";
  // }

  // if (vol_.empty())
  // {
  //   info << "VOL=NA";
  // }
  // else
  // {
  //   info << "VOL=" << vol_;
  // }
#endif

  info << "@" << ext_info_;
  info_ = info.str();
  return info.str();
}
} // namespace kungfu::wingchun::tora