#ifndef KUNGFU_MARKET_TYPE_CONVERT_TORA_H
#define KUNGFU_MARKET_TYPE_CONVERT_TORA_H

#include "TORATstpXMdApiStruct.h"
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
  }
}

inline int get_vol_multi(const std::string &instrument_id, const std::string &exchange_id) {
  if (!is_repo(instrument_id, exchange_id))
    return 1;

  return exchange_id == EXCHANGE_SSE ? 1000 : 100;
}

inline void from_tora(const CTORATstpMarketDataField &md, Quote &quote) {
  memset(&quote, 0, sizeof(Quote));
  strcpy(quote.trading_day, md.TradingDay);
  quote.data_time = nsec_from_tora_time(md.TradingDay, md.UpdateTime, md.UpdateMillisec);
  strcpy(quote.instrument_id, md.SecurityID);
  from_tora(md.ExchangeID, quote.exchange_id);
  quote.instrument_type = get_instrument_type(quote.exchange_id, quote.instrument_id);
  quote.pre_close_price = is_too_large(md.PreClosePrice) ? 0.0 : md.PreClosePrice;
  quote.last_price = is_too_large(md.LastPrice) ? 0.0 : md.LastPrice;
  quote.volume = md.Volume * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.turnover = md.Turnover;
  quote.open_price = md.OpenPrice;
  quote.high_price = md.HighestPrice;
  quote.low_price = md.LowestPrice;
  quote.upper_limit_price = md.UpperLimitPrice;
  quote.lower_limit_price = md.LowerLimitPrice;
  quote.close_price = is_too_large(md.ClosePrice) ? 0.0 : md.ClosePrice;
  //  quote.iopv = md.IOPV;
  quote.bid_price[0] = md.BidPrice1;
  quote.ask_price[0] = md.AskPrice1;
  quote.bid_volume[0] = md.BidVolume1 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.ask_volume[0] = md.AskVolume1 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.bid_price[1] = md.BidPrice2;
  quote.ask_price[1] = md.AskPrice2;
  quote.bid_volume[1] = md.BidVolume2 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.ask_volume[1] = md.AskVolume2 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.bid_price[2] = md.BidPrice3;
  quote.ask_price[2] = md.AskPrice3;
  quote.bid_volume[2] = md.BidVolume3 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.ask_volume[2] = md.AskVolume3 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.bid_price[3] = md.BidPrice4;
  quote.ask_price[3] = md.AskPrice4;
  quote.bid_volume[3] = md.BidVolume4 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.ask_volume[3] = md.AskVolume4 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.bid_price[4] = md.BidPrice5;
  quote.ask_price[4] = md.AskPrice5;
  quote.bid_volume[4] = md.BidVolume5 * get_vol_multi(quote.instrument_id, quote.exchange_id);
  quote.ask_volume[4] = md.AskVolume5 * get_vol_multi(quote.instrument_id, quote.exchange_id);
}
} // namespace kungfu::wingchun::tora

#endif // KUNGFU_TYPE_CONVERT_TORA_H
