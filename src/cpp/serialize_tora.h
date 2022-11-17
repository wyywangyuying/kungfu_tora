//
// Created by PolarAir on 2019-06-18.
//

#ifndef KUNGFU_SERIALIZE_TORA_H
#define KUNGFU_SERIALIZE_TORA_H

#include "TORATstpUserApiStruct.h"
#include <nlohmann/json.hpp>

using namespace TORASTOCKAPI;
namespace kungfu::wingchun::tora {
inline void to_json(nlohmann::json &j, const CTORATstpOrderField &md) {
  j["InvestorID"] = md.InvestorID;
  j["SecurityID"] = md.SecurityID;
  j["OrderRef"] = md.OrderRef;
  // j["UserID"] = md.UserID;
  j["Direction"] = md.Direction;
  j["LimitPrice"] = md.LimitPrice;
  j["VolumeTotalOriginal"] = md.VolumeTotalOriginal;
  j["OrderLocalID"] = md.OrderLocalID;
  j["ExchangeID"] = md.ExchangeID;
  // j["MarketID"] = md.MarketID;
  j["OrderSysID"] = md.OrderSysID;
  j["OrderStatus"] = md.OrderStatus;
  j["VolumeTraded"] = md.VolumeTraded;
  j["VolumeCanceled"] = md.VolumeCanceled;
  j["InsertDate"] = md.InsertDate;
  j["InsertTime"] = md.InsertTime;
  j["CancelTime"] = md.CancelTime;
  j["AccountID"] = md.AccountID;
  j["IPAddress"] = md.IPAddress;
  j["Turnover"] = md.Turnover;
}

inline void to_json(nlohmann::json &j, const CTORATstpTradeField &md) {
  j["InvestorID"] = md.InvestorID;
  j["SecurityID"] = md.SecurityID;
  // j["UserID"] = md.UserID;
  j["ExchangeID"] = md.ExchangeID;
  j["TradeID"] = md.TradeID;
  j["Direction"] = md.Direction;
  j["OrderLocalID"] = md.OrderLocalID;
  j["OrderSysID"] = md.OrderSysID;
  // j["MarketID"] = md.MarketID;
  j["Price"] = md.Price;
  j["Volume"] = md.Volume;
  j["TradeDate"] = md.TradeDate;
  j["TradeTime"] = md.TradeTime;
  j["TradeID"] = md.TradeID;
  j["OrderLocalID"] = md.OrderLocalID;
  j["TradingDay"] = md.TradingDay;
  j["BusinessUnitID"] = md.BusinessUnitID;
  j["AccountID"] = md.AccountID;
  j["OrderRef"] = md.OrderRef;
}

inline void to_json(nlohmann::json &j, const CTORATstpTradingAccountField &md) {
  j["AccountID"] = md.AccountID;
  j["UsefulMoney"] = md.UsefulMoney;
  j["FetchLimit"] = md.FetchLimit;
  j["CurrencyID"] = md.CurrencyID;
  j["Deposit"] = md.Deposit;
  j["Withdraw"] = md.Withdraw;
  j["UnDeliveredMoney"] = md.UnDeliveredMoney;
  j["FrozenCash"] = md.FrozenCash;
  j["FrozenCommission"] = md.FrozenCommission;
  j["PreUnDeliveredMoney"] = md.PreUnDeliveredMoney;
  j["Commission"] = md.Commission;
  j["AccountType"] = md.AccountType;
  j["InvestorID"] = md.InvestorID;
  j["DepartmentID"] = md.DepartmentID;
  j["BankID"] = md.BankID;
  j["BankAccountID"] = md.BankAccountID;
  j["UnDeliveredFrozenCash"] = md.UnDeliveredFrozenCash;
  j["UnDeliveredFrozenCommission"] = md.UnDeliveredFrozenCommission;
  j["UnDeliveredCommission"] = md.UnDeliveredCommission;
}

inline void to_json(nlohmann::json &j, const CTORATstpPositionField &md) {
  j["SecurityID"] = md.SecurityID;
  j["HistoryPos"] = md.HistoryPos;
  j["TodayBSPos"] = md.TodayBSPos;
  j["CurrentPosition"] = md.CurrentPosition;
  j["TotalPosCost"] = md.TotalPosCost;
  j["OpenPosCost"] = md.OpenPosCost;
  // j["LastPrice"] = md.LastPrice;
}

inline void to_json(nlohmann::json &j, const CTORATstpInputOrderField &md) {
  j["ShareholderID"] = md.ShareholderID;
  j["SecurityID"] = md.SecurityID;
  j["VolumeTotalOriginal"] = md.VolumeTotalOriginal;
  j["OrderSysID"] = md.OrderSysID;
}

template <typename T> std::string to_string(const T &ori) {
  nlohmann::json j;
  to_json(j, ori);
  return j.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
}
} // namespace kungfu::wingchun::tora

#endif // KUNGFU_SERIALIZE_TORA_H
