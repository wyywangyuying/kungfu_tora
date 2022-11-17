#ifndef MD_GATEWAY_KFEXT_DEMO_H
#define MD_GATEWAY_KFEXT_DEMO_H

#include <map>
#include <string>

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/marketdata.h>
#include <kungfu/yijinjing/common.h>

#include "TORATstpXMdApi.h"
using namespace TORALEV1API;
namespace kungfu::wingchun::tora {
class MarketDataTora : public CTORATstpXMdSpi, public broker::MarketData {
public:
  explicit MarketDataTora(broker::BrokerVendor &vendor);

  virtual ~MarketDataTora();

  bool subscribe(const std::vector<longfist::types::InstrumentKey> &instruments) override;

  bool subscribe_all() override;

  bool unsubscribe(const std::vector<longfist::types::InstrumentKey> &instruments) override;

  ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() override;

  ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
  ///        -3 连接已断开
  ///        -4 网络读失败
  ///        -5 网络写失败
  ///        -6 订阅流错误
  ///        -7 流序号错误
  ///        -8 错误的心跳报文
  ///        -9 错误的报文
  void OnFrontDisconnected(int nReason) override;

  ///错误应答
  // void OnRspError(CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

  ///登录请求响应
  void OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo,
                      int nRequestID) override;

  ///深度行情通知
  // void OnRtnDepthMarketData(CTORATstpMarketDataField *pDepthMarketData) override;

  ///行情通知
  void OnRtnMarketData(CTORATstpMarketDataField *pMarketDataField) override;

  ///订阅行情应答
  void OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurityField,
                          CTORATstpRspInfoField *pRspInfoField) override;

protected:
  void on_start() override;

private:
  void login();
  int get_req_id() const;

private:
  std::string front_address_;
  std::string account_id_;
  std::string password_;
  std::string protocol_;
  //md连接api
  CTORATstpXMdApi *api_;
};
} // namespace kungfu::wingchun::tora

#endif // MD_GATEWAY_KFEXT_DEMO_H
