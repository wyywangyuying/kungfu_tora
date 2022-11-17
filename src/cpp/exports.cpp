#include "marketdata_tora.h"
#include "trader_tora.h"

#include <kungfu/wingchun/extension.h>

KUNGFU_EXTENSION(tora) {
  KUNGFU_DEFINE_MD(kungfu::wingchun::tora::MarketDataTora);
  KUNGFU_DEFINE_TD(kungfu::wingchun::tora::TraderTora);
}
