#ifndef TD_GATEWAY_KFEXT_DEMO_H
#define TD_GATEWAY_KFEXT_DEMO_H

#include <map>
#include <memory>
#include <string>

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/trader.h>
#include <kungfu/yijinjing/common.h>

#include "TORATstpTraderApi.h"

using namespace TORASTOCKAPI;
namespace kungfu::wingchun::tora {
struct ToraOrder {
  uint64_t order_id_;
  int order_ref_;
  uint32_t source_;
  std::string instrument_id_;
  std::string exchange_id_;
  std::string order_sys_id_;
  int front_id_;
  int session_id_;
};

class TraderTora : public TORASTOCKAPI::CTORATstpTraderSpi, public broker::Trader {
public:
  explicit TraderTora(broker::BrokerVendor &vendor);

  virtual ~TraderTora();

  longfist::enums::AccountType get_account_type() const override { return longfist::enums::AccountType::Stock; }

  bool insert_order(const event_ptr &event) override;

  bool cancel_order(const event_ptr &event) override;

  bool req_position() override;

  bool req_account() override;

  ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() override;

  ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
  ///@param nReason 错误原因
  ///        -3 连接已断开
  ///        -4 网络读失败
  ///        -5 网络写失败
  ///        -6 订阅流错误
  ///        -7 流序号错误
  ///        -8 错误的心跳报文
  ///        -9 错误的报文
  void OnFrontDisconnected(int nReason) override;

  ///错误应答
  void OnRspError(TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

  //登录应答
  void OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField *pRspUserLoginField,
                      TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) override;

  //报单录入应答
  void OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField *pInputOrderField,
                        TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) override;

  //报单回报
  void OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField *pOrder) override;

  //报单错误回报
  void OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField *pInputOrder,
                           TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) override;

  //撤单应答
  void OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField *pInputOrderActionField,
                        TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) override;

  //撤单错误回报
  void OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField *pInputOrderActionField,
                           TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) override;

  //成交回报
  void OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField *pTrade) override;

  //查询股东账户
  void OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField *pShareholderAccount,
                                  TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

  //查询证券信息
  void OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField *pSecurity, TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) override;

  //资金账户查询
  void OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField *pTradingAccount,
                              TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

  //投资者持仓查询
  void OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField *pPosition, TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo,
                        int nRequestID, bool bIsLast) override;

protected:
  void on_start() override;

private:
  void login();

  void req_shareholders();

  void req_instrument();

  int get_req_id() const;

private:
  std::string front_address_;
  std::string password_;
  std::string fund_id_;
  std::string department_id_;
  std::string trading_day_;
  std::string dynamic_password_;

  TORASTOCKAPI::CTORATstpTraderApi *api_;

  int front_id_;
  int session_id_;
  int order_ref_;
  int instrument_count_ = 0;

  static uint16_t m_login_time;
  std::map<std::string, std::string> exchange_2_shareholder_;

  std::unordered_map<uint64_t, ToraOrder> outbound_orders_;
  std::unordered_map<uint64_t, uint64_t> inbound_order_refs_;
  std::unordered_map<uint64_t, uint64_t> inbound_order_sysids_;
};
} // namespace kungfu::wingchun::tora

#endif // TD_GATEWAY_KFEXT_DEMO_H
