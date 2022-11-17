#ifndef KUNGFU_TYPE_CONVERT_TORA_H
#define KUNGFU_TYPE_CONVERT_TORA_H

#include "TORATstpUserApiStruct.h"
#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/common.h>
#include <kungfu/wingchun/encoding.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::wingchun::tora {
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;

inline int64_t nsec_from_tora_time(const char *date, const char *update_time, int millisec = 0) {
  static char datetime[21];
  memset(datetime, 0, 21);
  memcpy(datetime, date, 4);
  datetime[4] = '-';
  memcpy(datetime + 5, date + 4, 2);
  datetime[7] = '-';
  memcpy(datetime + 8, date + 6, 2);
  datetime[10] = ' ';
  memcpy(datetime + 11, update_time, 8);
  int64_t nano_sec = kungfu::yijinjing::time::strptime(std::string(datetime), "%Y-%m-%d %H:%M:%S");
  nano_sec += millisec * kungfu::yijinjing::time_unit::NANOSECONDS_PER_MILLISECOND;
  return nano_sec;
}

inline int64_t from_tora_timestamp(const std::string &trading_day, const int64_t &tora_time,
                                   TTORATstpExchangeIDType &tora_exchange, bool is_quote = false) {
  int64_t nsec;
  if (tora_exchange == TORA_TSTP_EXD_SSE) {
    if (is_quote)
      nsec = kungfu::yijinjing::time::today_start() +
             (tora_time / 10000 * 60 * 60 + tora_time / 100 % 100 * 60 + tora_time % 100) *
                 kungfu::yijinjing::time_unit::NANOSECONDS_PER_SECOND;
    else
      nsec = kungfu::yijinjing::time::today_start() +
             (tora_time / 1000000 * 60 * 60 + tora_time / 10000 % 100 * 60 + tora_time / 100 % 100) *
                 kungfu::yijinjing::time_unit::NANOSECONDS_PER_SECOND +
             tora_time % 100 * 10 * kungfu::yijinjing::time_unit::NANOSECONDS_PER_MILLISECOND;
  } else
    nsec = kungfu::yijinjing::time::today_start() +
           (tora_time / 10000000 * 60 * 60 + tora_time / 100000 % 100 * 60 + tora_time / 1000 % 100) *
               kungfu::yijinjing::time_unit::NANOSECONDS_PER_SECOND +
           tora_time % 1000 * kungfu::yijinjing::time_unit::NANOSECONDS_PER_MILLISECOND;
  return nsec;
}

inline std::string get_exchange(const TTORATstpExchangeIDType &ExchangeID) {
  if (ExchangeID == TORA_TSTP_EXD_SSE) {
    return EXCHANGE_SSE;
  } else if (ExchangeID == TORA_TSTP_EXD_SZSE) {
    return EXCHANGE_SZE;
  } else if (ExchangeID == TORA_TSTP_EXD_BSE) {
    return EXCHANGE_BSE;
  } else {
    return "";
  }
}

inline void from_tora(TTORATstpExchangeIDType tora_exchange, char *exchange_id) {
  if (tora_exchange == TORA_TSTP_EXD_SSE) {
    strcpy(exchange_id, EXCHANGE_SSE);
  } else if (tora_exchange == TORA_TSTP_EXD_SZSE) {
    strcpy(exchange_id, EXCHANGE_SZE);
  } else if (tora_exchange == TORA_TSTP_EXD_BSE) {
    strcpy(exchange_id, EXCHANGE_BSE);
  } else {
    strcpy(exchange_id, "");
  }
}

inline void to_tora(const char *exchange_id, TTORATstpExchangeIDType &tora_exchange) {
  if (strcmp(exchange_id, EXCHANGE_SSE) == 0) {
    tora_exchange = TORA_TSTP_EXD_SSE;
  } else if (strcmp(exchange_id, EXCHANGE_SZE) == 0) {
    tora_exchange = TORA_TSTP_EXD_SZSE;
  } else if (strcmp(exchange_id, EXCHANGE_BSE) == 0) {
    tora_exchange = TORA_TSTP_EXD_BSE;
  } else {
    tora_exchange = TORA_TSTP_EXD_COMM;
  }
}
inline void from_tora(const TTORATstpOrderPriceTypeType &ori, PriceType &des) {
  switch (ori) {
  case TORA_TSTP_OPT_LimitPrice:
    des = PriceType::Limit;
    break;
  case TORA_TSTP_OPT_BestPrice:
    des = PriceType::ReverseBest;
    break;
  case TORA_TSTP_OPT_FiveLevelPrice:
    des = PriceType::FakBest5;
    break;
  case TORA_TSTP_OPT_HomeBestPrice:
    des = PriceType::ForwardBest;
    break;
  case TORA_TSTP_OPT_AnyPrice:
    des = PriceType::Any;
  default:
    des = PriceType::UnKnown;
    break;
  }
}

inline void to_tora(const PriceType &ori, const char *exchange_id, TTORATstpOrderPriceTypeType &des) {
  switch (ori) {
  case PriceType::Any: {
    if (strcmp(exchange_id, EXCHANGE_SZE) == 0)
      des = TORA_TSTP_OPT_FiveLevelPrice;
    else
      des = TORA_TSTP_OPT_AnyPrice;
    break;
  }
  case PriceType::ForwardBest:
  case PriceType::Fak:
  case PriceType::Fok:
    des = TORA_TSTP_OPT_HomeBestPrice;
    break;
  case PriceType::ReverseBest:
    des = TORA_TSTP_OPT_BestPrice;
    break;
  case PriceType::FakBest5:
    des = TORA_TSTP_OPT_FiveLevelPrice;
    break;
  case PriceType::Limit:
    des = TORA_TSTP_OPT_LimitPrice;
    break;
  default:
    break;
  }
}

inline void from_tora(const TTORATstpDirectionType &ori, Side &des) {
  switch (ori) {
  case TORA_TSTP_D_Buy:
  case TORA_TSTP_D_ETFPur:
  case TORA_TSTP_D_Repurchase:
  case TORA_TSTP_D_IPO:
  case TORA_TSTP_D_OeFundPur:
    des = Side::Buy;
    break;

  case TORA_TSTP_D_Sell:
  case TORA_TSTP_D_ETFRed:
  case TORA_TSTP_D_ReverseRepur:
  case TORA_TSTP_D_OeFundRed:
    des = Side::Sell;
    break;

  default:
    // other tora directions are not supported
    break;
  }
}

inline void to_tora(const Side &ori, const char *instrument_id, const char *exchange_id, TTORATstpDirectionType &des) {
  if (is_repo(instrument_id, exchange_id)) {
    des = ori == Side::Sell ? TORA_TSTP_D_ReverseRepur : TORA_TSTP_D_Repurchase;
  } else {
    des = ori == Side::Buy ? TORA_TSTP_D_Buy : TORA_TSTP_D_Sell;
  }
}

inline void from_tora(const TTORATstpTimeConditionType &ori, TimeCondition &des) {
  switch (ori) {
  case TORA_TSTP_TC_IOC:
    des = TimeCondition::IOC;
    break;
  case TORA_TSTP_TC_GFS:
  case TORA_TSTP_TC_GFD:
  case TORA_TSTP_TC_GTD:
  case TORA_TSTP_TC_GFA:
    des = TimeCondition::GFD;
    break;
  case TORA_TSTP_TC_GTC:
    des = TimeCondition::GTC;
    break;
  default:
    break;
  }
}

inline void to_tora(const TimeCondition &ori, TTORATstpTimeConditionType &des) {
  switch (ori) {
  case TimeCondition::IOC:
    des = TORA_TSTP_TC_IOC;
    break;
  case TimeCondition::GFD:
    des = TORA_TSTP_TC_GFD;
    break;
  case TimeCondition::GTC:
    des = TORA_TSTP_TC_GTC;
    break;
  default:
    break;
  }
}

inline void from_tora(const TTORATstpVolumeConditionType &ori, VolumeCondition &des) {
  switch (ori) {
  case TORA_TSTP_VC_AV:
    des = VolumeCondition::Any;
    break;
  case TORA_TSTP_VC_MV:
    des = VolumeCondition::Min;
    break;
  case TORA_TSTP_VC_CV:
    des = VolumeCondition::All;
    break;
  default:
    break;
  }
}

inline void to_tora(const VolumeCondition &ori, TTORATstpVolumeConditionType &des) {
  switch (ori) {
  case VolumeCondition::Any:
    des = TORA_TSTP_VC_AV;
    break;
  case VolumeCondition::Min:
    des = TORA_TSTP_VC_MV;
    break;
  case VolumeCondition::All:
    des = TORA_TSTP_VC_CV;
    break;
  default:
    break;
  }
}

inline void from_tora(const TTORATstpOrderStatusType &ori, OrderStatus &des) {
  switch (ori) {
  case TORA_TSTP_OST_AllTraded:
    des = OrderStatus::Filled;
    break;
  case TORA_TSTP_OST_PartTraded:
    des = OrderStatus::PartialFilledActive;
    break;
  case TORA_TSTP_OST_PartTradeCanceled:
    des = OrderStatus::PartialFilledNotActive;
    break;
  case TORA_TSTP_OST_Accepted:
  case TORA_TSTP_OST_Cached:
    des = OrderStatus::Pending;
    break;
  case TORA_TSTP_OST_AllCanceled:
    des = OrderStatus::Cancelled;
    break;
  case TORA_TSTP_OST_SendTradeEngine:
    des = OrderStatus::Submitted;
    break;
  case TORA_TSTP_OST_Rejected:
    des = OrderStatus::Error;
    break;
  case TORA_TSTP_OST_Unknown:
  default:
    des = OrderStatus::Unknown;
    break;
  }
}

inline int get_vol_multi(const std::string &instrument_id, const std::string &exchange_id) {
  if (!is_repo(instrument_id, exchange_id))
    return 1;

  return exchange_id == EXCHANGE_SSE ? 1000 : 100;
}

inline void from_tora(const CTORATstpSecurityField *pSecurity, Instrument &instrument) {
  strcpy(instrument.instrument_id, pSecurity->SecurityID);
  if (pSecurity->ExchangeID == TORA_TSTP_EXD_SSE) {
    instrument.exchange_id = EXCHANGE_SSE;
  } else if (pSecurity->ExchangeID == TORA_TSTP_EXD_SZSE) {
    instrument.exchange_id = EXCHANGE_SZE;
  } else if (pSecurity->ExchangeID == TORA_TSTP_EXD_BSE) {
    instrument.exchange_id = EXCHANGE_BSE;
  } else {
    SPDLOG_ERROR("(pSecurity->ExchangeID) {} (instrument.exchange_id) {}", pSecurity->ExchangeID,
                 instrument.exchange_id);
  }
  memcpy(instrument.product_id, pSecurity->SecurityName, PRODUCT_ID_LEN);
  strcpy(instrument.open_date, pSecurity->OpenDate);
  instrument.instrument_type = get_instrument_type(instrument.exchange_id, instrument.instrument_id);
}

inline void from_tora(const CTORATstpTradingAccountField &ori, Asset &des) {
  memset(&des, 0, sizeof(Asset));
  des.update_time = yijinjing::time::now_in_nano();
  des.avail = ori.UsefulMoney;
  des.accumulated_fee = ori.Commission;
  des.frozen_cash = ori.FrozenCash;
}

inline void from_tora(const CTORATstpPositionField &ori, Position &des) {
  memset(&des, 0, sizeof(Position));
  des.update_time = yijinjing::time::now_in_nano();
  strncpy(des.trading_day, ori.TradingDay, 8);
  strcpy(des.instrument_id, ori.SecurityID);
  from_tora(ori.ExchangeID, des.exchange_id);
  des.instrument_type = get_instrument_type(des.exchange_id, des.instrument_id);
  des.direction = Direction::Long;
  des.volume = ori.CurrentPosition * get_vol_multi(des.instrument_id, des.exchange_id);
  des.yesterday_volume = ori.HistoryPos * get_vol_multi(des.instrument_id, des.exchange_id);
  des.frozen_total = (ori.HistoryPosFrozen + ori.TodayBSPosFrozen) * get_vol_multi(des.instrument_id, des.exchange_id);
  des.frozen_yesterday = ori.HistoryPosFrozen * get_vol_multi(des.instrument_id, des.exchange_id);
  des.avg_open_price = ori.OpenPosCost / des.volume;
  des.position_cost_price = ori.TotalPosCost / des.volume;
}

inline void to_tora(const OrderInput &ori, CTORATstpInputOrderField &des) {
  memset(&des, 0, sizeof(CTORATstpInputOrderField));
  strcpy(des.SecurityID, ori.instrument_id);
  switch (ori.price_type) {
  case PriceType::Limit:
    des.OrderPriceType = TORA_TSTP_OPT_LimitPrice;
    des.TimeCondition = TORA_TSTP_TC_GFD;
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  case PriceType::Any:
    if (strcmp(ori.exchange_id, EXCHANGE_SSE) == 0) {
      if (strncmp(ori.instrument_id, "688", 3) == 0) {
        des.OrderPriceType = TORA_TSTP_OPT_BestPrice;
        des.TimeCondition = TORA_TSTP_TC_GFD;
      } else {
        des.OrderPriceType = TORA_TSTP_OPT_FiveLevelPrice;
        des.TimeCondition = TORA_TSTP_TC_IOC;
      }
    } else if (strcmp(ori.exchange_id, EXCHANGE_BSE) == 0) {
      des.OrderPriceType = TORA_TSTP_OPT_FiveLevelPrice;
      des.TimeCondition = TORA_TSTP_TC_IOC;
    } else {
      des.OrderPriceType = TORA_TSTP_OPT_AnyPrice;
      des.TimeCondition = TORA_TSTP_TC_IOC;
    }
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  case PriceType::Fok:
    des.OrderPriceType = TORA_TSTP_OPT_AnyPrice;
    des.TimeCondition = TORA_TSTP_TC_IOC;
    des.VolumeCondition = TORA_TSTP_VC_CV;
    break;

  case PriceType::Fak:
    des.OrderPriceType = TORA_TSTP_OPT_AnyPrice;
    des.TimeCondition = TORA_TSTP_TC_IOC;
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  case PriceType::ReverseBest:
    if (strcmp(ori.exchange_id, EXCHANGE_SSE) == 0) {
      if (strncmp(ori.instrument_id, "688", 3) == 0) {
        des.OrderPriceType = TORA_TSTP_OPT_BestPrice;
      } else {
        des.OrderPriceType = TORA_TSTP_OPT_FiveLevelPrice;
      }
    } else {
      des.OrderPriceType = TORA_TSTP_OPT_BestPrice;
    }
    des.TimeCondition = TORA_TSTP_TC_GFD;
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  case PriceType::ForwardBest:
    des.OrderPriceType = TORA_TSTP_OPT_HomeBestPrice;
    des.TimeCondition = TORA_TSTP_TC_GFD;
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  case PriceType::FakBest5:
    des.OrderPriceType = TORA_TSTP_OPT_FiveLevelPrice;
    des.TimeCondition = TORA_TSTP_TC_IOC;
    des.VolumeCondition = TORA_TSTP_VC_AV;
    break;

  default:
    break;
  }
  to_tora(ori.side, ori.instrument_id, ori.exchange_id, des.Direction);
  des.LimitPrice = ori.limit_price;
  des.VolumeTotalOriginal = ori.volume / get_vol_multi(ori.instrument_id, ori.exchange_id);
  des.ForceCloseReason = TORA_TSTP_FCC_NotForceClose;
  to_tora(ori.exchange_id, des.ExchangeID);
  des.Operway = TORA_TSTP_OPERW_PCClient;
}

inline void from_tora(const CTORATstpInputOrderField &ori, Order &des) {
  strcpy(des.instrument_id, ori.SecurityID);
  from_tora(ori.ExchangeID, des.exchange_id);
  des.instrument_type = get_instrument_type(des.exchange_id, des.instrument_id);
  des.limit_price = ori.LimitPrice;
  des.volume = ori.VolumeTotalOriginal * get_vol_multi(des.instrument_id, des.exchange_id);
  des.volume_left = ori.VolumeTotalOriginal * get_vol_multi(des.instrument_id, des.exchange_id);
  des.status = OrderStatus::Error;
  from_tora(ori.Direction, des.side);
  from_tora(ori.OrderPriceType, des.price_type);
}

inline void from_tora(const CTORATstpOrderField &ori, Order &des) {
  strcpy(des.trading_day, ori.TradingDay);
  strcpy(des.instrument_id, ori.SecurityID);
  from_tora(ori.ExchangeID, des.exchange_id);
  des.instrument_type = get_instrument_type(des.exchange_id, des.instrument_id);
  des.limit_price = ori.LimitPrice;
  des.volume = ori.VolumeTotalOriginal * get_vol_multi(des.instrument_id, des.exchange_id);
  des.volume_left = ori.VolumeTotalOriginal - ori.VolumeTraded;
  des.volume_left *= get_vol_multi(des.instrument_id, des.exchange_id);
  from_tora(ori.OrderStatus, des.status);
  strcpy(des.error_msg, gbk2utf8(ori.StatusMsg).c_str());
  from_tora(ori.Direction, des.side);
  from_tora(ori.OrderPriceType, des.price_type);
}

inline void from_tora(const CTORATstpTradeField &ori, Trade &des) {
  memset(&des, 0, sizeof(Trade));
  des.trade_time = nsec_from_tora_time(ori.TradeDate, ori.TradeTime);
  strcpy(des.trading_day, ori.TradingDay);
  strcpy(des.instrument_id, ori.SecurityID);
  from_tora(ori.ExchangeID, des.exchange_id);
  des.instrument_type = get_instrument_type(des.exchange_id, des.instrument_id);
  from_tora(ori.Direction, des.side);
  des.price = ori.Price;
  des.volume = ori.Volume * get_vol_multi(des.instrument_id, des.exchange_id);
}

} // namespace kungfu::wingchun::tora

#endif // KUNGFU_TYPE_CONVERT_TORA_H
