#pragma once

#include "protocol/ByteBuffer.h"
#include "protocol/ProtocolHelper.h"
#include "protocol/Response.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

struct DecodedResponse
{
  ResponseTag tag;
  uint32_t errorCode = 0;
  std::string stringValue;
  int64_t intValue = 0;
  double doubleValue = 0;
  std::vector<DecodedResponse> arrayValues;
};

inline DecodedResponse decodePayload(const uint8_t *data, size_t size, size_t &used)
{
  DecodedResponse decoded{};
  if (size == 0)
    throw std::runtime_error("empty payload");

  decoded.tag = static_cast<ResponseTag>(data[0]);
  used = 1;

  switch (decoded.tag)
  {
  case ResponseTag::TAG_NIL:
    return decoded;
  case ResponseTag::TAG_ERR:
  {
    uint32_t len = 0;
    std::memcpy(&decoded.errorCode, data + used, 4);
    std::memcpy(&len, data + used + 4, 4);
    decoded.stringValue.assign(reinterpret_cast<const char *>(data + used + 8), len);
    used += 8 + len;
    return decoded;
  }
  case ResponseTag::TAG_STR:
  {
    uint32_t len = 0;
    std::memcpy(&len, data + used, 4);
    decoded.stringValue.assign(reinterpret_cast<const char *>(data + used + 4), len);
    used += 4 + len;
    return decoded;
  }
  case ResponseTag::TAG_INT:
    std::memcpy(&decoded.intValue, data + used, 8);
    used += 8;
    return decoded;
  case ResponseTag::TAG_DBL:
    std::memcpy(&decoded.doubleValue, data + used, 8);
    used += 8;
    return decoded;
  case ResponseTag::TAG_ARR:
  {
    uint32_t count = 0;
    std::memcpy(&count, data + used, 4);
    used += 4;
    for (uint32_t i = 0; i < count; ++i)
    {
      size_t childUsed = 0;
      decoded.arrayValues.push_back(decodePayload(data + used, size - used, childUsed));
      used += childUsed;
    }
    return decoded;
  }
  }

  throw std::runtime_error("unknown response tag");
}

inline DecodedResponse decodeResponse(const Response &response)
{
  ProtocolHelper helper;
  std::vector<uint8_t> bytes = helper.serialize(response);
  uint32_t len = 0;
  std::memcpy(&len, bytes.data(), 4);

  size_t used = 0;
  DecodedResponse decoded = decodePayload(bytes.data() + 4, len, used);
  if (used != len)
    throw std::runtime_error("response decode length mismatch");

  return decoded;
}

inline ByteBuffer makeRequestBuffer(const std::vector<std::string> &args)
{
  ByteBuffer buf;
  uint32_t msglen = 4;
  for (const std::string &arg : args)
    msglen += 4 + static_cast<uint32_t>(arg.size());

  std::vector<uint8_t> raw(4 + msglen);
  std::memcpy(raw.data(), &msglen, 4);

  uint32_t nargs = static_cast<uint32_t>(args.size());
  std::memcpy(raw.data() + 4, &nargs, 4);

  size_t cursor = 8;
  for (const std::string &arg : args)
  {
    uint32_t len = static_cast<uint32_t>(arg.size());
    std::memcpy(raw.data() + cursor, &len, 4);
    std::memcpy(raw.data() + cursor + 4, arg.data(), len);
    cursor += 4 + len;
  }

  buf.append(raw.data(), raw.size());
  return buf;
}

inline std::string uniqueKey(const std::string &prefix)
{
  static int counter = 0;
  return prefix + "_" + std::to_string(++counter);
}

inline void sleepForMs(int ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
