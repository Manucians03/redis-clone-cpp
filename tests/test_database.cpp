#include "TestFramework.h"

#include "containers/HashTable.h"
#include "containers/ResizableHashTable.h"
#include "containers/StringValue.h"

#include <algorithm>

TEST(HashTableSetGetDeleteAndKeys)
{
  HashTable db(8);
  db.set("alpha", std::make_shared<StringValue>("one"));
  db.set("beta", std::make_shared<StringValue>("two"));

  std::shared_ptr<StringValue> alpha = std::dynamic_pointer_cast<StringValue>(db.get("alpha"));
  EXPECT_TRUE(alpha != nullptr);
  EXPECT_EQ(alpha->getValue(), std::string("one"));
  EXPECT_EQ(db.size(), size_t(2));

  std::vector<std::string> keys = db.keys();
  std::sort(keys.begin(), keys.end());
  EXPECT_EQ(keys.size(), size_t(2));
  EXPECT_EQ(keys[0], std::string("alpha"));
  EXPECT_EQ(keys[1], std::string("beta"));

  EXPECT_TRUE(db.del("alpha"));
  EXPECT_FALSE(db.del("missing"));
  EXPECT_TRUE(db.get("alpha") == nullptr);
}

TEST(ResizableHashTableRehashesWithoutLosingValues)
{
  ResizableHashTable db(4);

  for (int i = 0; i < 20; ++i)
    db.set("key" + std::to_string(i), std::make_shared<StringValue>("value" + std::to_string(i)));

  EXPECT_EQ(db.size(), size_t(20));
  EXPECT_TRUE(db.capacity() >= size_t(4));

  for (int i = 0; i < 20; ++i)
  {
    std::shared_ptr<StringValue> value = std::dynamic_pointer_cast<StringValue>(db.get("key" + std::to_string(i)));
    EXPECT_TRUE(value != nullptr);
    EXPECT_EQ(value->getValue(), std::string("value" + std::to_string(i)));
  }

  EXPECT_TRUE(db.del("key7"));
  EXPECT_TRUE(db.get("key7") == nullptr);
  EXPECT_EQ(db.size(), size_t(19));
}
