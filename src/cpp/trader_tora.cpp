#include "trader_tora.h"
#include "common.h"
#include "serialize_tora.h"
#include "trace_info.h"
#include "type_convert_tora.h"
#include <kungfu/wingchun/encoding.h>
namespace kungfu::wingchun::tora {
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
uint16_t TraderTora::m_login_time = 0;

TraderTora::TraderTora(broker::BrokerVendor &vendor)
    : Trader(vendor), api_(nullptr), front_id_(-1), session_id_(-1), order_ref_(-1) {
  SPDLOG_DEBUG("TraderTora Constructor");
}

TraderTora::~TraderTora() {
  if (nullptr != api_) {
    api_->Release();
    api_ = nullptr;
  }
}

void TraderTora::OnFrontConnected() {
  SPDLOG_INFO("ONFRONTCONNECTED RES");
  update_broker_state(BrokerState::Connected);
  SPDLOG_INFO("BrokerState::Connected");
  login();
}

void TraderTora::OnFrontDisconnected(int nReason) {
  SPDLOG_ERROR("ONFRONTDISCONNECTED RES nReason {}", nReason);
  update_broker_state(BrokerState::DisConnected);
  SPDLOG_INFO("BrokerState::DisConnected");
}

void TraderTora::OnRspError(TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("ONRSPERROR RES error_id {}, error_msg {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
  }
}

void TraderTora::OnRspUserLogin(TORASTOCKAPI::CTORATstpRspUserLoginField *pRspUserLoginField,
                                TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("LOGIN RES error_id {}, error_msg {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
    update_broker_state(BrokerState::LoginFailed);
    SPDLOG_INFO("BrokerState::LoginFailed");
    return;
  }

  if (pRspUserLoginField == nullptr) {
    SPDLOG_INFO("LOGIN RES pRspUserLoginField is nullptr");
    return;
  }

  front_id_ = pRspUserLoginField->FrontID;
  session_id_ = pRspUserLoginField->SessionID;
  order_ref_ = pRspUserLoginField->MaxOrderRef + 1;
  trading_day_ = pRspUserLoginField->TradingDay;
  SPDLOG_INFO("LOGIN RES front_id {}, session_id {}, trading_day {}", front_id_, session_id_, trading_day_);

  update_broker_state(BrokerState::LoggedIn);
  SPDLOG_INFO("BrokerState::LoggedIn");
  //查询持仓信息
  req_shareholders();
}

void TraderTora::OnRspQryShareholderAccount(TORASTOCKAPI::CTORATstpShareholderAccountField *pShareholderAccount,
                                            TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID,
                                            bool bIsLast) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("SHAREHOLDERS RES error_id {} error_msg {} rid {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg),
                 nRequestID);
    return;
  }

  if (pShareholderAccount == nullptr) {
    SPDLOG_INFO("SHAREHOLDERS RES pShareholderAccount is nullptr, bIsLast {}", bIsLast);
  } else {
    SPDLOG_INFO("SHAREHOLDERS RES ExchangeID {}, ShareholderID {}, bIsLast {}", pShareholderAccount->ExchangeID,
                pShareholderAccount->ShareholderID, bIsLast);

    char exchange[9] = {0};
    from_tora(pShareholderAccount->ExchangeID, exchange);
    if (exchange_2_shareholder_.find(exchange) == exchange_2_shareholder_.end())
      exchange_2_shareholder_[exchange] = pShareholderAccount->ShareholderID;
  }

  if (bIsLast) {
    if (check_if_stored_instruments(trading_day_)) {
      SPDLOG_INFO("CHECK_IF_STORED_INSTRUMENTS TRUE");
      update_broker_state(BrokerState::Ready);
      SPDLOG_INFO("BrokerState::Ready");
      return;
    }
    SPDLOG_INFO("CHECK_IF_STORED_INSTRUMENTS FALSE, no today instruments");
    req_instrument();
  }
}

void TraderTora::OnRspQrySecurity(TORASTOCKAPI::CTORATstpSecurityField *pSecurity,
                                  TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (pRspInfo != nullptr && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("(error_id) {} (error_msg) {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
    return;
  }

  if (pSecurity == nullptr) {
    SPDLOG_INFO("INSTRUMENT RES pSecurity is nullptr, bIsLast {}", bIsLast);
  } else {
    std::string exchange_id = get_exchange(pSecurity->ExchangeID);

    if (exchange_id != "") {
      std::string instrument_id(pSecurity->SecurityID);
      instrument_count_++;
      auto writer = get_writer(0);
      Instrument &instrument = writer->open_data<Instrument>(0);
      from_tora(pSecurity, instrument);
      writer->close_data();
    }
  }

  if (bIsLast) {

    SPDLOG_INFO("INSTRUMENT instrument_count_ {}", instrument_count_);
    SPDLOG_INFO("INSTRUMENT RES bIsLast {}", bIsLast);
    record_stored_instruments_trading_day(trading_day_);
    update_broker_state(BrokerState::Ready);
    SPDLOG_INFO("BrokerState::Ready");
  }
}

void TraderTora::OnRspQryTradingAccount(TORASTOCKAPI::CTORATstpTradingAccountField *pTradingAccount,
                                        TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("ASSET RES FAILED error_id {} error_msg {} rid {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg),
                 nRequestID);
    return;
  }

  if (pTradingAccount == nullptr) {
    SPDLOG_INFO("ASSET RES pTradingAccount is nullptr, bIsLast {}", bIsLast);
    return;
  }

  SPDLOG_INFO("ASSET RES *pTradingAccount {}, bIsLast {}", to_string(*pTradingAccount), bIsLast);

  if (strlen(pTradingAccount->AccountID) > 0 && pTradingAccount->AccountType == TORA_TSTP_FAT_Normal) {
    fund_id_ = pTradingAccount->AccountID;
    auto writer = get_writer(0);
    auto &account = writer->open_data<Asset>(0);
    from_tora(*pTradingAccount, account);
    strcpy(account.trading_day, trading_day_.c_str());
    account.holder_uid = get_home_uid();
    account.ledger_category = LedgerCategory::Account;
    writer->close_data();
  }
}

void TraderTora::OnRspQryPosition(TORASTOCKAPI::CTORATstpPositionField *pPosition,
                                  TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("POS RES FAILED error_id {} error_msg {} rid {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg),
                 nRequestID);
    return;
  }

  if (pPosition == nullptr) {
    SPDLOG_INFO("POS RES pPosition is nullptr, bIsLast {}", bIsLast);
  } else {
    SPDLOG_DEBUG("POS RES *pPosition {}, bIsLast {}", to_string(*pPosition), bIsLast);
    // ensure the volume(CurrentPosition) > 0;
    if (strlen(pPosition->SecurityID) > 0 && pPosition->CurrentPosition > 0) {
      auto writer = get_writer(0);
      auto &pos = writer->open_data<Position>(0);
      from_tora(*pPosition, pos);
      strcpy(pos.trading_day, trading_day_.c_str());
      pos.holder_uid = get_home_uid();
      pos.ledger_category = LedgerCategory::Account;
      writer->close_data();
    }
  }

  if (bIsLast) {
    auto writer = get_writer(0);
    auto &end = writer->open_data<PositionEnd>(now());
    end.holder_uid = get_home_uid();
    writer->close_data();
  }
}

void TraderTora::on_start() {
  auto config = nlohmann::json::parse(get_config());

  front_address_ = config.value("td_front", "");
  password_ = config.value("password", "");
  dynamic_password_ = config.value("dynamic_password", "");
  std::string sh_shareholder_id = config.value("sh_shareholder_id", "");
  if (!sh_shareholder_id.empty())
    exchange_2_shareholder_[EXCHANGE_SSE] = sh_shareholder_id;
  std::string sz_shareholder_id = config.value("sz_shareholder_id", "");
  if (!sz_shareholder_id.empty())
    exchange_2_shareholder_[EXCHANGE_SZE] = sz_shareholder_id;

  auto account_id = config.value("account_id", "");
  if (account_id.size() > 8) {
    department_id_ = account_id.substr(0, 4);
  }
  SPDLOG_INFO("ON_START CONNECTING TORA TD account {} at {}", get_account_id(), front_address_);

  api_ = TORASTOCKAPI::CTORATstpTraderApi::CreateTstpTraderApi();
  api_->RegisterSpi(this);
  api_->RegisterFront((char *)front_address_.c_str());
  api_->SubscribePrivateTopic(TORA_TERT_QUICK);
  api_->SubscribePublicTopic(TORA_TERT_QUICK);
  api_->Init();
}

void TraderTora::OnRspOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField *pInputOrderField,
                                  TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("ONRSPORDERINSERT RES error_id {}, error_msg {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
  } else {
    uint64_t orderRef_key = get_orderRef_key(front_id_, session_id_, pInputOrderField->OrderRef);
    if (inbound_order_refs_.find(orderRef_key) != inbound_order_refs_.end()) {
      auto order_id = inbound_order_refs_[orderRef_key];
      if (outbound_orders_.find(order_id) != outbound_orders_.end()) {
        outbound_orders_[order_id].order_sys_id_ = pInputOrderField->OrderSysID;
        uint64_t orderSysId_key = get_orderSysId_key(pInputOrderField->ExchangeID, pInputOrderField->OrderSysID);
        inbound_order_sysids_[orderSysId_key] = order_id;
        SPDLOG_TRACE("ONRSPORDERINSERT RES *pInputOrderField {}", to_string(*pInputOrderField));
      }
    }
  }
}

void TraderTora::OnRtnOrder(TORASTOCKAPI::CTORATstpOrderField *pOrder) {
  if (session_id_ != pOrder->SessionID && front_id_ != pOrder->FrontID) {
    return;
  }
  uint64_t orderRef_key = get_orderRef_key(front_id_, session_id_, pOrder->OrderRef);
  if (inbound_order_refs_.find(orderRef_key) == inbound_order_refs_.end()) {
    SPDLOG_WARN("ONRTNORDER RES order_ref {}, *pOrder {}", pOrder->OrderRef, to_string(*pOrder));
    return;
  }

  if (pOrder->OrderStatus == TORA_TSTP_OST_Unknown) {
    SPDLOG_WARN("ONRTNORDER RES order_ref {}, *pOrder {}", pOrder->OrderRef, to_string(*pOrder));
    return;
  }

  auto order_id = inbound_order_refs_[orderRef_key];
  if (orders_.find(order_id) == orders_.end()) {
    SPDLOG_ERROR("CANNOT FIND ORDER order_id {} with orderRef_key {}", order_id, orderRef_key);
    return;
  }
  if (strlen(pOrder->OrderSysID) != 0) {
    inbound_order_sysids_[get_orderSysId_key(pOrder->ExchangeID, pOrder->OrderSysID)] = order_id;
  }
  auto order_state = orders_.at(order_id);
  auto writer = get_writer(order_state.dest);
  auto &order = writer->open_data<Order>(0);
  memcpy(&order, &(order_state.data), sizeof(order));
  from_tora(*pOrder, order);
  order.order_id = order_state.data.order_id;
  order.insert_time = order_state.data.insert_time;
  order.update_time = time::now_in_nano();
  writer->close_data();
  SPDLOG_TRACE("ONRTNORDER RES *pOrder {}", to_string(*pOrder));
}

void TraderTora::OnErrRtnOrderInsert(TORASTOCKAPI::CTORATstpInputOrderField *pInputOrder,
                                     TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) {
  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    uint64_t orderRef_key = get_orderRef_key(front_id_, session_id_, pInputOrder->OrderRef);
    if (inbound_order_refs_.find(orderRef_key) == inbound_order_refs_.end()) {
      SPDLOG_WARN("ONERRORRTNORDERINSERT RES order_ref {}, error_id {}, error_msg {}", pInputOrder->OrderRef,
                  pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
      return;
    }

    auto order_id = inbound_order_refs_[orderRef_key];
    auto order_state = orders_.at(order_id);
    auto writer = get_writer(order_state.dest);
    auto &order = writer->open_data<Order>(0);
    memcpy(&order, &(order_state.data), sizeof(order));
    from_tora(*pInputOrder, order);
    strcpy(order.trading_day, trading_day_.c_str());
    order.error_id = pRspInfo->ErrorID;
    strcpy(order.error_msg, gbk2utf8(pRspInfo->ErrorMsg).c_str());
    order.status = OrderStatus::Error;
    order.order_id = order_state.data.order_id;
    order.insert_time = order_state.data.insert_time;
    order.update_time = time::now_in_nano();
    writer->close_data();

    SPDLOG_TRACE(" ONERRORRTNORDERINSERT RES error_id {}, error_msg {}, rid {}", pRspInfo->ErrorID,
                 gbk2utf8(pRspInfo->ErrorMsg), nRequestID);
  }
}

void TraderTora::OnRspOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField *pInputOrderActionField,
                                  TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) {
  if (session_id_ != pInputOrderActionField->SessionID && front_id_ != pInputOrderActionField->FrontID) {
    return;
  }

  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("ONRSPORDERACTION RES, error_id {}, error_msg {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
  } else {
    SPDLOG_TRACE("ONRSPORDERACTION RES, order_ref {}", pInputOrderActionField->OrderRef);
  }
}

void TraderTora::OnErrRtnOrderAction(TORASTOCKAPI::CTORATstpInputOrderActionField *pInputOrderActionField,
                                     TORASTOCKAPI::CTORATstpRspInfoField *pRspInfo, int nRequestID) {
  if (session_id_ != pInputOrderActionField->SessionID && front_id_ != pInputOrderActionField->FrontID) {
    return;
  }

  if (nullptr != pRspInfo && pRspInfo->ErrorID != 0) {
    SPDLOG_ERROR("ONERRRTNORDERACTION RES error_id {}, error_msg {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
  }
}

void TraderTora::OnRtnTrade(TORASTOCKAPI::CTORATstpTradeField *pTrade) {
  uint64_t orderSysId_key = get_orderSysId_key(pTrade->ExchangeID, pTrade->OrderSysID);
  if (inbound_order_sysids_.find(orderSysId_key) == inbound_order_sysids_.end()) {
    SPDLOG_ERROR("CANNOT FIND orderSysId_key {} in inbound_order_sysids_", orderSysId_key);
    return;
  }
  auto order_id = inbound_order_sysids_[orderSysId_key];
  if (orders_.find(order_id) == orders_.end()) {
    SPDLOG_WARN("ONRTNTRADE RES order_ref {}", pTrade->OrderRef);
    return;
  }

  auto order_state = orders_.at(order_id);
  auto writer = get_writer(order_state.dest);
  Trade &trade = writer->open_data<Trade>(0);
  from_tora(*pTrade, trade);
  trade.trade_id = writer->current_frame_uid();
  trade.order_id = order_state.data.order_id;
  trades_.emplace(trade.uid(), state<Trade>(order_state.source, order_state.dest, time::now_in_nano(), trade));
  writer->close_data();
  SPDLOG_TRACE("ONRTNTRADE RES *pTrade {}", to_string(*pTrade));
}

bool TraderTora::insert_order(const event_ptr &event) {
  const auto &input = event->data<OrderInput>();
  TORASTOCKAPI::CTORATstpInputOrderField req = {};
  to_tora(input, req);
  strcpy(req.InvestorID, get_account_id().c_str());
  req.OrderRef = order_ref_;
  ++order_ref_;
  strcpy(req.ShareholderID, exchange_2_shareholder_[input.exchange_id].c_str());

  int ret = api_->ReqOrderInsert(&req, get_req_id());

  auto nano = time::now_in_nano();
  auto writer = get_writer(event->source());
  auto &order = writer->open_data<Order>(event->gen_time());
  order_from_input(input, order);
  order.insert_time = nano;
  order.update_time = nano;

  if (ret != 0) {
    order.error_id = ret;
    order.status = OrderStatus::Error;
    SPDLOG_ERROR("INSERT ORDER, error_id {}", ret);
  }

  orders_.emplace(order.uid(), state<Order>(event->dest(), event->source(), nano, order));
  writer->close_data();
  outbound_orders_[input.order_id] = {input.order_id,    req.OrderRef, event->source(), input.instrument_id,
                                      input.exchange_id, "",           front_id_,       session_id_};
  inbound_order_refs_[get_orderRef_key(front_id_, session_id_, req.OrderRef)] = input.order_id;
  return ret == 0;
}

bool TraderTora::cancel_order(const event_ptr &event) {
  const auto &action = event->data<OrderAction>();

  if (outbound_orders_.find(action.order_id) == outbound_orders_.end()) {
    SPDLOG_ERROR("CANCEL FAILED order_id {}", action.order_id);
    return false;
  }

  const auto &order_record = outbound_orders_[action.order_id];

  TORASTOCKAPI::CTORATstpInputOrderActionField req = {};
  req.OrderActionRef = order_ref_;
  ++order_ref_;
  req.OrderRef = order_record.order_ref_;
  req.FrontID = order_record.front_id_;
  req.SessionID = order_record.session_id_;
  to_tora(order_record.exchange_id_.c_str(), req.ExchangeID);
  strcpy(req.OrderSysID, order_record.order_sys_id_.c_str());
  req.ActionFlag = TORA_TSTP_AF_Delete;
  req.Operway = TORA_TSTP_OPERW_PCClient;
  int ret = api_->ReqOrderAction(&req, get_req_id());
  SPDLOG_INFO("CANCEL ORDER ret {}", ret);

  if (ret == 0) {
    actions_.emplace(action.uid(), state<OrderAction>(event->dest(), event->source(), event->gen_time(), action));
  } else {
    auto writer = get_writer(event->source());
    auto &error = writer->open_data<OrderActionError>(event->gen_time());
    error.error_id = ret;
    error.order_id = action.order_id;
    error.order_action_id = action.order_action_id;
    writer->close_data();
    SPDLOG_ERROR("CANCEL FAILED ret {}", ret);
  }

  return ret == 0;
}

bool TraderTora::req_position() {

  TORASTOCKAPI::CTORATstpQryPositionField req = {};
  strcpy(req.InvestorID, get_account_id().c_str());

  uint64_t nano_sec = time::now_in_nano();
  uint64_t time_stamp = kungfu::yijinjing::time::strptime(std::to_string(nano_sec), "%Y-%m-%d %H:%M:%S");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int ret = api_->ReqQryPosition(&req, get_req_id());
  SPDLOG_INFO("POS REQ ret {}", ret);
  if (ret != 0) {
    update_broker_state(BrokerState::DisConnected);
    SPDLOG_INFO("BrokerState::DisConnected");
    SPDLOG_ERROR("POS REQ FAILED error_id {}", ret);
    return false;
  }

  return true;
}

bool TraderTora::req_account() {
  uint64_t nano_sec = time::now_in_nano();
  uint64_t time_stamp = kungfu::yijinjing::time::strptime(std::to_string(nano_sec), "%Y-%m-%d %H:%M:%S");
  TORASTOCKAPI::CTORATstpQryTradingAccountField req = {};

  strcpy(req.InvestorID, get_account_id().c_str());
  req.CurrencyID = TORA_TSTP_CID_CNY;
  req.AccountType = TORA_TSTP_FAT_Normal;
  strcpy(req.DepartmentID, department_id_.c_str());
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int ret = api_->ReqQryTradingAccount(&req, get_req_id());
  SPDLOG_INFO("ASSET REQ ret {}", ret);
  if (ret != 0) {
    update_broker_state(BrokerState::DisConnected);
    SPDLOG_INFO("BrokerState::DisConnected");
    SPDLOG_ERROR("ASSET REQ FAILED error_id {}", ret);
    return false;
  }

  return true;
}

void TraderTora::login() {
  TraceInfo traceInfo;
  TORASTOCKAPI::CTORATstpReqUserLoginField req = {};
  strcpy(req.LogInAccount, get_account_id().c_str());
  req.LogInAccountType = TORA_TSTP_LACT_UserID;
  strcpy(req.Password, password_.c_str());
  strcpy(req.DepartmentID, department_id_.c_str());
  strcpy(req.TerminalInfo, traceInfo.get_trace_info().c_str());
  strcpy(req.UserProductInfo, "KungFu");
  strcpy(req.DynamicPassword, dynamic_password_.c_str());

  m_login_time++;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int ret = api_->ReqUserLogin(&req, get_req_id());
  SPDLOG_INFO("LOGIN REQ ret {} req.TerminalInfo {}", ret, (const char *)req.TerminalInfo);
  std::string mac;
  if (ret != 0) {
    update_broker_state(BrokerState::LoginFailed);
    SPDLOG_INFO("BrokerState::LoginFailed");
  }
}

void TraderTora::req_shareholders() {
  TORASTOCKAPI::CTORATstpQryShareholderAccountField req = {};
  strcpy(req.InvestorID, get_account_id().c_str());
  req.ShareholderIDType = TORA_TSTP_SIDT_Normal;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int ret = api_->ReqQryShareholderAccount(&req, get_req_id());
  SPDLOG_INFO("SHAREHOLDERS REQ ret {}", ret);

  if (ret != 0) {
    SPDLOG_ERROR("SHAREHOLDERS REQ FAILED");
  }
}

void TraderTora::req_instrument() {
  TORASTOCKAPI::CTORATstpQrySecurityField field;
  memset(&field, 0, sizeof(field));
  SPDLOG_INFO("INSTRUMENT REQ");
  std::this_thread::sleep_for(std::chrono::seconds(1));
  int ret = api_->ReqQrySecurity(&field, get_req_id());
  SPDLOG_INFO("INSTRUMENT REQ ret {}", ret);

  if (ret != 0) {
    SPDLOG_ERROR("INSTRUMENT REQ FAILED");
  }
}

int TraderTora::get_req_id() const {
  static int rid = 0;
  return ++rid;
}

} // namespace kungfu::wingchun::tora
