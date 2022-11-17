#include "marketdata_tora.h"
#include "market_serialize_tora.h"
#include "market_type_convert_tora.h"
#include <kungfu/wingchun/encoding.h>

namespace kungfu::wingchun::tora {
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

MarketDataTora::MarketDataTora(broker::BrokerVendor &vendor) : MarketData(vendor), api_(nullptr) {
  SPDLOG_DEBUG("MarketDataTora Constructor");
  KUNGFU_SETUP_LOG();
}

MarketDataTora::~MarketDataTora() {
  if (nullptr != api_) {
    api_->Release();
    api_ = nullptr;
  }
}

bool MarketDataTora::subscribe(const std::vector<longfist::types::InstrumentKey> &instruments) {
  std::vector<std::string> sh_tickers;
  std::vector<std::string> sz_tickers;
  std::vector<std::string> bse_tickers;

  for (const auto &ins : instruments) {
    if (strcmp(ins.exchange_id, EXCHANGE_SSE) == 0) {
      sh_tickers.push_back(ins.instrument_id);
    } else if (strcmp(ins.exchange_id, EXCHANGE_SZE) == 0) {
      sz_tickers.push_back(ins.instrument_id);
    } else if (strcmp(ins.exchange_id, EXCHANGE_BSE) == 0) {
      bse_tickers.push_back(ins.instrument_id);
    }
  }

  auto func = [this](std::vector<std::string> &tickers, const std::string &exchange) -> bool {
    std::vector<char *> ticker_array{};
    for (auto &ticker : tickers)
      ticker_array.push_back(&ticker.front());

    TTORATstpExchangeIDType ex;
    to_tora(exchange.c_str(), ex);
    int ret = api_->SubscribeMarketData(ticker_array.data(), ticker_array.size(), ex);
    if (ret == 0) {
      SPDLOG_INFO("[Subscribe] subscribe request success, (size) {} (exchange_id) {}", ticker_array.size(), exchange);
      return true;
    } else {
      SPDLOG_INFO("[Subscribe] failed to subscribe, (exchange_id) {}", exchange);
      return false;
    }
  };

  auto ret = func(sh_tickers, EXCHANGE_SSE);
  ret = ret && func(sz_tickers, EXCHANGE_SZE);
  ret = ret && func(bse_tickers, EXCHANGE_BSE);
  return ret;
}

bool MarketDataTora::subscribe_all() {
  std::vector<std::string> sh_tickers;
  std::vector<std::string> sz_tickers;
  std::vector<std::string> bse_tickers;
  sh_tickers.emplace_back("00000000");
  sz_tickers.emplace_back("00000000");
  bse_tickers.emplace_back("00000000");

  auto func = [this](std::vector<std::string> &tickers, const std::string &exchange) -> bool {
    std::vector<char *> ticker_array{};
    for (auto &ticker : tickers)
      ticker_array.push_back(&ticker.front());

    TTORATstpExchangeIDType ex;
    to_tora(exchange.c_str(), ex);
    int ret = api_->SubscribeMarketData(ticker_array.data(), ticker_array.size(), ex);
    if (ret == 0) {
      SPDLOG_INFO("[Subscribe] subscribe request success, (size) {} (exchange_id) {}", ticker_array.size(), exchange);
      return true;
    } else {
      SPDLOG_INFO("[Subscribe] failed to subscribe, (exchange_id) {}", exchange);
      return false;
    }
  };

  auto ret = func(sh_tickers, EXCHANGE_SSE);
  ret = ret && func(sz_tickers, EXCHANGE_SZE);
  ret = ret && func(bse_tickers, EXCHANGE_BSE);
  return ret;
}

bool MarketDataTora::unsubscribe(const std::vector<longfist::types::InstrumentKey> &instruments) {
  std::vector<std::string> sh_tickers;
  std::vector<std::string> sz_tickers;
  std::vector<std::string> bse_tickers;

  for (const auto &ins : instruments) {
    if (strcmp(ins.exchange_id, EXCHANGE_SSE) == 0) {
      sh_tickers.push_back(ins.instrument_id);
    } else if (strcmp(ins.exchange_id, EXCHANGE_SZE) == 0) {
      sz_tickers.push_back(ins.instrument_id);
    } else if (strcmp(ins.exchange_id, EXCHANGE_BSE) == 0) {
      bse_tickers.push_back(ins.instrument_id);
    }
  }

  auto func = [this](std::vector<std::string> &tickers, const std::string &exchange) -> bool {
    std::vector<char *> ticker_array{};
    for (auto &ticker : tickers)
      ticker_array.push_back(&ticker.front());

    TTORATstpExchangeIDType ex;
    to_tora(exchange.c_str(), ex);
    int ret = api_->UnSubscribeMarketData(ticker_array.data(), ticker_array.size(), ex);
    if (ret == 0) {
      SPDLOG_INFO("[UnSubscribe] unsubscribe request success, (size) {} (exchange_id) {}", ticker_array.size(),
                  exchange);
      return true;
    } else {
      SPDLOG_INFO("[UnSubscribe] failed to unsubscribe, (exchange_id) {}", exchange);
      return false;
    }
  };

  auto ret = func(sh_tickers, EXCHANGE_SSE);
  ret = ret && func(sz_tickers, EXCHANGE_SZE);
  ret = ret && func(bse_tickers, EXCHANGE_BSE);
  return ret;
}

void MarketDataTora::OnFrontConnected() {
  SPDLOG_INFO("[Connect] Api Connected");
  update_broker_state(BrokerState::Connected);

  login();
}

void MarketDataTora::OnFrontDisconnected(int nReason) {
  SPDLOG_WARN(
      "[Connect] Api Disconnected. If this is not caused by user logout, api will automatically try to reconnect.");
  update_broker_state(BrokerState::DisConnected);
}

// void MarketDataTora::OnRspError(CTORATstpRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
// {
//     if (pRspInfo->ErrorID != 0)
//     {
//         SPDLOG_ERROR("[RspError] (ErrorId) {}, (ErrorMsg) {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
//     }
// }

void MarketDataTora::OnRspUserLogin(CTORATstpRspUserLoginField *pRspUserLogin, CTORATstpRspInfoField *pRspInfo,
                                    int nRequestID) {
  if (pRspInfo->ErrorID == 0) {
    SPDLOG_INFO("[Login] Login Success, (account_id) {}", pRspUserLogin->LogInAccount);
    update_broker_state(BrokerState::LoggedIn);
    update_broker_state(BrokerState::Ready);
  } else {
    // auto error_msg = kungfu::gbk2utf8(pRspInfo->ErrorMsg);
    SPDLOG_ERROR("[Login] Login Failed, (ErrorId) {}, (ErrorMsg) {}", pRspInfo->ErrorID, gbk2utf8(pRspInfo->ErrorMsg));
    update_broker_state(BrokerState::LoginFailed);
  }
}

void MarketDataTora::OnRtnMarketData(CTORATstpMarketDataField *pMarketDataField) {
  SPDLOG_DEBUG("OnRtnMarketData CTORATstpMarketDataField: {}", to_string(*pMarketDataField));

  Quote &quote = get_writer(0)->open_data<Quote>(0);
  from_tora(*pMarketDataField, quote);
  get_writer(0)->close_data();
  SPDLOG_DEBUG("OnRtnMarketData Quote: {}", quote.to_string());
}

void MarketDataTora::OnRspSubMarketData(CTORATstpSpecificSecurityField *pSpecificSecurityField,
                                        CTORATstpRspInfoField *pRspInfoField) {
  if (pRspInfoField->ErrorID != 0) {
    SPDLOG_INFO("subscribe market data fail, error_id={} error_msg={}", pRspInfoField->ErrorID,
                pRspInfoField->ErrorMsg);
  }
}
//点击行情连接按钮后首先创建api
void MarketDataTora::on_start() {
  //将用户设置转化成json
  auto config = nlohmann::json::parse(get_config());
  protocol_ = config.value("protocol", "tcp");
  front_address_ = config.value("md_front", "");
  account_id_ = config.value("account_id", "");
  password_ = config.value("password", "");

  SPDLOG_INFO("[on_start] Connecting TORA MD for {} at {}, protocol is {}", account_id_, front_address_, protocol_);
  //如果api之前没有被创建
  if (nullptr == api_) {
    //如果采用tcp方式连接
    if (protocol_ == "tcp") {
      api_ = CTORATstpXMdApi::CreateTstpXMdApi();
      api_->RegisterSpi(this);
      api_->RegisterFront((char *)(front_address_.c_str()));
      api_->Init();
    } else {
      //如果采用udp方式连接
      api_ = CTORATstpXMdApi::CreateTstpXMdApi(TORA_TSTP_MST_MCAST, TORA_TSTP_MST_MCAST);
      api_->RegisterSpi(this);
      api_->RegisterMulticast((char *)(front_address_.c_str()), (char *)("10.166.33.1"), NULL);
      api_->Init();
      // login();
    }
    SPDLOG_INFO("[on_start] Api Version: {}", api_->GetApiVersion());
  } else {
    SPDLOG_INFO("[on_start] Already Inited");
  }
}

void MarketDataTora::login() {
  if (nullptr != api_) {
    CTORATstpReqUserLoginField req = {};
    strcpy(req.LogInAccount, account_id_.c_str());
    req.LogInAccountType = TORA_TSTP_LACT_UserID;
    strcpy(req.Password, password_.c_str());

    int ret = api_->ReqUserLogin(&req, get_req_id());
    if (ret != 0) {
      update_broker_state(BrokerState::LoginFailed);
      SPDLOG_ERROR("[Login] Login Failed, (ret) {}", ret);
    }
  }
}

int MarketDataTora::get_req_id() const {
  static int req_id = 0;
  if (req_id < 0)
    req_id = 0;
  return ++req_id;
}
} // namespace kungfu::wingchun::tora
