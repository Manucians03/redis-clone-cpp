#include "TestFramework.h"
#include "TestSupport.h"

#include "protocol/ByteBuffer.h"
#include "protocol/ProtocolHelper.h"

TEST(ByteBufferAppendAndConsume)
{
  ByteBuffer buf;
  const uint8_t first[] = {1, 2, 3};
  const uint8_t second[] = {4, 5};

  buf.append(first, 3);
  buf.append(second, 2);
  EXPECT_EQ(buf.size(), size_t(5));
  EXPECT_EQ(buf.data()[0], uint8_t(1));
  EXPECT_EQ(buf.data()[4], uint8_t(5));

  buf.consume(2);
  EXPECT_EQ(buf.size(), size_t(3));
  EXPECT_EQ(buf.data()[0], uint8_t(3));

  buf.consume(99);
  EXPECT_EQ(buf.size(), size_t(0));
  EXPECT_TRUE(buf.data() == nullptr);
}

TEST(ProtocolHelperParsesAndLowercasesCommands)
{
  ProtocolHelper helper;
  ByteBuffer buf = makeRequestBuffer({"SET", "MyKey", "Hello"});

  std::unique_ptr<Request> request = helper.parseRequest(buf);
  EXPECT_TRUE(request != nullptr);
  EXPECT_EQ(request->args.size(), size_t(3));
  EXPECT_EQ(request->args[0], std::string("set"));
  EXPECT_EQ(request->args[1], std::string("MyKey"));
  EXPECT_EQ(request->args[2], std::string("Hello"));
  EXPECT_EQ(buf.size(), size_t(0));
}

TEST(ProtocolHelperWaitsForCompleteMessages)
{
  ProtocolHelper helper;
  ByteBuffer complete = makeRequestBuffer({"GET", "alpha"});

  ByteBuffer partial;
  partial.append(complete.data(), 4);
  EXPECT_TRUE(helper.parseRequest(partial) == nullptr);

  partial.append(complete.data() + 4, complete.size() - 4);
  std::unique_ptr<Request> request = helper.parseRequest(partial);
  EXPECT_TRUE(request != nullptr);
  EXPECT_EQ(request->args[0], std::string("get"));
  EXPECT_EQ(request->args[1], std::string("alpha"));
}

TEST(ResponseSerializationRoundTripsStructuredValues)
{
  Response response = Response::array({
      Response::str("alpha"),
      Response::integer(42),
      Response::dbl(3.5),
      Response::nil(),
      Response::error(static_cast<uint32_t>(ErrorCode::ERR_UNKNOWN), "boom"),
  });

  DecodedResponse decoded = decodeResponse(response);
  EXPECT_EQ(decoded.tag, ResponseTag::TAG_ARR);
  EXPECT_EQ(decoded.arrayValues.size(), size_t(5));
  EXPECT_EQ(decoded.arrayValues[0].tag, ResponseTag::TAG_STR);
  EXPECT_EQ(decoded.arrayValues[0].stringValue, std::string("alpha"));
  EXPECT_EQ(decoded.arrayValues[1].intValue, int64_t(42));
  EXPECT_NEAR(decoded.arrayValues[2].doubleValue, 3.5, 1e-9);
  EXPECT_EQ(decoded.arrayValues[3].tag, ResponseTag::TAG_NIL);
  EXPECT_EQ(decoded.arrayValues[4].tag, ResponseTag::TAG_ERR);
  EXPECT_EQ(decoded.arrayValues[4].errorCode, static_cast<uint32_t>(ErrorCode::ERR_UNKNOWN));
  EXPECT_EQ(decoded.arrayValues[4].stringValue, std::string("boom"));
}
