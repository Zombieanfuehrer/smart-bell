#include <gtest/gtest.h>
#include "Utils/CircularBuffer.h"

namespace {

class CircularBufferTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up code here, if needed
  }

  void TearDown() override {
    // Clean up code here, if needed
  }

  utils::CircularBuffer buffer{}; 
};

TEST_F(CircularBufferTest, PushBackAndPopFront) {
  uint8_t data = 42;
  EXPECT_TRUE(buffer.push_back(data));
  uint8_t result;
  EXPECT_TRUE(buffer.pop_front(&result));
  EXPECT_EQ(result, data);
}

TEST_F(CircularBufferTest, BufferFull) {
  for (int i = 0; i < 250; ++i) {
    EXPECT_TRUE(buffer.push_back(i));
  }
  EXPECT_FALSE(buffer.push_back(251));  // Buffer should be full
}

TEST_F(CircularBufferTest, FillAndEmptyBuffer) {
  for (int i = 0; i < 250; ++i) {
    EXPECT_TRUE(buffer.push_back('A'));
  }
  for (int i = 0; i < 250; ++i) {
    uint8_t result;
    EXPECT_TRUE(buffer.pop_front(&result));
    EXPECT_EQ(result, 'A');
  }
  EXPECT_FALSE(buffer.pop_front(nullptr));  // Buffer should be empty
}

TEST_F(CircularBufferTest, BufferEmpty) {
  uint8_t result;
  EXPECT_FALSE(buffer.pop_front(&result));  // Buffer should be empty
}

TEST_F(CircularBufferTest, UsedEntries) {
  EXPECT_EQ(buffer.used_entries(), 0);
  buffer.push_back(1);
  EXPECT_EQ(buffer.used_entries(), 1);
  uint8_t result;
  buffer.pop_front(&result);
  EXPECT_EQ(buffer.used_entries(), 0);
}

TEST_F(CircularBufferTest, ClearBuffer) {
  buffer.push_back(1);
  buffer.push_back(2);
  buffer.clear();
  EXPECT_EQ(buffer.used_entries(), 0);
  uint8_t result;
  EXPECT_FALSE(buffer.pop_front(&result));  // Buffer should be empty
}

}  // namespace