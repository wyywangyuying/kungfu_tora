#ifndef KFEXT_TORA_COMMON_H
#define KFEXT_TORA_COMMON_H
#include "TORATstpUserApiStruct.h"

namespace kungfu::wingchun::tora {
inline uint64_t get_orderSysId_key(const char exchangeId, const char *orderSysId) {
  uint32_t hashed_exchangeId = kungfu::hash_32((const unsigned char *)(&exchangeId), sizeof(TTORATstpExchangeIDType));
  uint32_t hashed_orderSysId = kungfu::hash_32((const unsigned char *)orderSysId, sizeof(TTORATstpOrderSysIDType));
  return ((uint64_t)hashed_exchangeId << 32u) | hashed_orderSysId;
}

inline uint64_t get_orderRef_key(const int frontId, const int sessionId, int orderRef) {
  uint32_t front_session_id = ((uint32_t)frontId << 16u) | (uint16_t)sessionId;
  std::string strRef = std::to_string(orderRef);
  uint32_t hashed_orderRef = kungfu::hash_32((const unsigned char *)(strRef.c_str()), strRef.length());
  return ((uint64_t)front_session_id << 32u) | hashed_orderRef;
}
} // namespace kungfu::wingchun::tora

#endif // KFEXT_TORA_COMMON_H