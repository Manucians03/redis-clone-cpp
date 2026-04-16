#include "TestFramework.h"
#include "TestSupport.h"

#include "command/DelCommand.h"
#include "command/GetCommand.h"
#include "command/KeysCommand.h"
#include "command/PExpireCommand.h"
#include "command/PTTLCommand.h"
#include "command/SetCommand.h"
#include "command/ZAddCommand.h"
#include "command/ZQueryCommand.h"
#include "command/ZRemCommand.h"
#include "command/ZScoreCommand.h"
#include "containers/ResizableHashTable.h"
#include "server/TTLManager.h"

#include <algorithm>

TEST(SetGetAndDelCommandsWorkTogether)
{
  ResizableHashTable db(8);
  SetCommand setCommand;
  GetCommand getCommand;
  DelCommand delCommand;

  DecodedResponse setResponse = decodeResponse(setCommand.execute(&db, {"set", "name", "redis"}));
  EXPECT_EQ(setResponse.tag, ResponseTag::TAG_NIL);

  DecodedResponse getResponse = decodeResponse(getCommand.execute(&db, {"get", "name"}));
  EXPECT_EQ(getResponse.tag, ResponseTag::TAG_STR);
  EXPECT_EQ(getResponse.stringValue, std::string("redis"));

  DecodedResponse delResponse = decodeResponse(delCommand.execute(&db, {"del", "name"}));
  EXPECT_EQ(delResponse.tag, ResponseTag::TAG_INT);
  EXPECT_EQ(delResponse.intValue, int64_t(1));

  DecodedResponse missingGet = decodeResponse(getCommand.execute(&db, {"get", "name"}));
  EXPECT_EQ(missingGet.tag, ResponseTag::TAG_NIL);
}

TEST(KeysCommandReturnsAllStoredKeys)
{
  ResizableHashTable db(8);
  SetCommand setCommand;
  KeysCommand keysCommand;

  setCommand.execute(&db, {"set", "alpha", "1"});
  setCommand.execute(&db, {"set", "beta", "2"});

  DecodedResponse response = decodeResponse(keysCommand.execute(&db, {"keys"}));
  EXPECT_EQ(response.tag, ResponseTag::TAG_ARR);
  EXPECT_EQ(response.arrayValues.size(), size_t(2));

  std::vector<std::string> keys;
  for (const DecodedResponse &entry : response.arrayValues)
    keys.push_back(entry.stringValue);
  std::sort(keys.begin(), keys.end());

  EXPECT_EQ(keys[0], std::string("alpha"));
  EXPECT_EQ(keys[1], std::string("beta"));
}

TEST(SortedSetCommandsSupportAddUpdateQueryScoreAndRemove)
{
  ResizableHashTable db(8);
  ZAddCommand zadd;
  ZQueryCommand zquery;
  ZScoreCommand zscore;
  ZRemCommand zrem;

  EXPECT_EQ(decodeResponse(zadd.execute(&db, {"zadd", "leaders", "2.5", "bob"})).intValue, int64_t(1));
  EXPECT_EQ(decodeResponse(zadd.execute(&db, {"zadd", "leaders", "1.0", "alice"})).intValue, int64_t(1));
  EXPECT_EQ(decodeResponse(zadd.execute(&db, {"zadd", "leaders", "3.0", "carl"})).intValue, int64_t(1));
  EXPECT_EQ(decodeResponse(zadd.execute(&db, {"zadd", "leaders", "4.0", "bob"})).intValue, int64_t(0));

  DecodedResponse scoreResponse = decodeResponse(zscore.execute(&db, {"zscore", "leaders", "bob"}));
  EXPECT_EQ(scoreResponse.tag, ResponseTag::TAG_DBL);
  EXPECT_NEAR(scoreResponse.doubleValue, 4.0, 1e-9);

  DecodedResponse queryResponse = decodeResponse(zquery.execute(&db, {"zquery", "leaders", "0", "", "0", "3"}));
  EXPECT_EQ(queryResponse.tag, ResponseTag::TAG_ARR);
  EXPECT_EQ(queryResponse.arrayValues.size(), size_t(6));
  EXPECT_EQ(queryResponse.arrayValues[0].stringValue, std::string("alice"));
  EXPECT_NEAR(queryResponse.arrayValues[1].doubleValue, 1.0, 1e-9);
  EXPECT_EQ(queryResponse.arrayValues[2].stringValue, std::string("carl"));
  EXPECT_NEAR(queryResponse.arrayValues[3].doubleValue, 3.0, 1e-9);
  EXPECT_EQ(queryResponse.arrayValues[4].stringValue, std::string("bob"));
  EXPECT_NEAR(queryResponse.arrayValues[5].doubleValue, 4.0, 1e-9);

  EXPECT_EQ(decodeResponse(zrem.execute(&db, {"zrem", "leaders", "alice"})).intValue, int64_t(1));
  EXPECT_EQ(decodeResponse(zscore.execute(&db, {"zscore", "leaders", "alice"})).tag, ResponseTag::TAG_NIL);
}

TEST(PExpireAndPTTLUseMillisecondPrecisionAndDelClearsTTL)
{
  ResizableHashTable db(8);
  SetCommand setCommand;
  PExpireCommand pexpire;
  PTTLCommand pttl;
  DelCommand del;

  const std::string key = uniqueKey("ttl_key");
  setCommand.execute(&db, {"set", key, "value"});

  DecodedResponse expireResponse = decodeResponse(pexpire.execute(&db, {"pexpire", key, "150"}));
  EXPECT_EQ(expireResponse.tag, ResponseTag::TAG_INT);
  EXPECT_EQ(expireResponse.intValue, int64_t(1));

  DecodedResponse ttlResponse = decodeResponse(pttl.execute(&db, {"pttl", key}));
  EXPECT_EQ(ttlResponse.tag, ResponseTag::TAG_INT);
  EXPECT_TRUE(ttlResponse.intValue > 0);
  EXPECT_TRUE(ttlResponse.intValue <= 150);

  decodeResponse(del.execute(&db, {"del", key}));
  DecodedResponse clearedResponse = decodeResponse(pttl.execute(&db, {"pttl", key}));
  EXPECT_EQ(clearedResponse.intValue, int64_t(-1));
}

TEST(ExpiredKeysAreRemovedWhenCleanupRuns)
{
  ResizableHashTable db(8);
  SetCommand setCommand;
  PExpireCommand pexpire;
  GetCommand get;
  PTTLCommand pttl;

  const std::string key = uniqueKey("expiring_key");
  setCommand.execute(&db, {"set", key, "value"});
  decodeResponse(pexpire.execute(&db, {"pexpire", key, "25"}));

  sleepForMs(50);
  TTLManager::instance().removeExpired(&db);

  EXPECT_EQ(decodeResponse(get.execute(&db, {"get", key})).tag, ResponseTag::TAG_NIL);
  EXPECT_EQ(decodeResponse(pttl.execute(&db, {"pttl", key})).intValue, int64_t(-1));
}
