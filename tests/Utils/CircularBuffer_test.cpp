#include "Utils/CircularBuffer.h"
#include <gtest/gtest.h>

namespace {

// Test template CircularBuffer with different sizes
class CircularBufferTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Set up code here, if needed
  }

  void TearDown() override {
    // Clean up code here, if needed
  }

  utils::CircularBuffer<32> small_buffer_;
  utils::CircularBuffer<256> large_buffer_;
};

TEST_F(CircularBufferTest, PushBackAndPopFront) {
  uint8_t data = 42;
  EXPECT_TRUE(small_buffer_.push_back(data));
  uint8_t result;
  EXPECT_TRUE(small_buffer_.pop_front(&result));
  EXPECT_EQ(result, data);
}

TEST_F(CircularBufferTest, BufferFull_SmallBuffer) {
  // Fill buffer to capacity (32 elements)
  for (size_t i = 0; i < 32; ++i) {
    EXPECT_TRUE(small_buffer_.push_back(static_cast<uint8_t>(i)));
  }
  // Next push should fail
  EXPECT_FALSE(small_buffer_.push_back(255));
}

TEST_F(CircularBufferTest, BufferFull_LargeBuffer) {
  // Fill buffer to capacity (256 elements)
  for (size_t i = 0; i < 256; ++i) {
    EXPECT_TRUE(large_buffer_.push_back(static_cast<uint8_t>(i)));
  }
  // Next push should fail
  EXPECT_FALSE(large_buffer_.push_back(255));
}

TEST_F(CircularBufferTest, FillAndEmptyBuffer) {
  constexpr size_t test_size = 32;
  for (size_t i = 0; i < test_size; ++i) {
    EXPECT_TRUE(small_buffer_.push_back('A'));
  }

  for (size_t i = 0; i < test_size; ++i) {
    uint8_t result;
    EXPECT_TRUE(small_buffer_.pop_front(&result));
    EXPECT_EQ(result, 'A');
  }

  // Buffer should be empty now
  uint8_t result;
  EXPECT_FALSE(small_buffer_.pop_front(&result));
}

TEST_F(CircularBufferTest, BufferEmpty) {
  uint8_t result;
  EXPECT_FALSE(small_buffer_.pop_front(&result));  // Buffer should be empty
}

TEST_F(CircularBufferTest, WrapAround) {
  // Test circular behavior by filling, partially emptying, and refilling

  // Fill half
  for (size_t i = 0; i < 16; ++i) {
    EXPECT_TRUE(small_buffer_.push_back(static_cast<uint8_t>(i)));
  }

  // Empty half
  for (size_t i = 0; i < 16; ++i) {
    uint8_t result;
    EXPECT_TRUE(small_buffer_.pop_front(&result));
    EXPECT_EQ(result, static_cast<uint8_t>(i));
  }

  // Fill to capacity (should wrap around)
  for (size_t i = 0; i < 32; ++i) {
    EXPECT_TRUE(small_buffer_.push_back(static_cast<uint8_t>(100 + i)));
  }

  // Verify all values
  for (size_t i = 0; i < 32; ++i) {
    uint8_t result;
    EXPECT_TRUE(small_buffer_.pop_front(&result));
    EXPECT_EQ(result, static_cast<uint8_t>(100 + i));
  }
}

TEST_F(CircularBufferTest, SequentialOperations) {
  // Interleave push and pop operations
  for (size_t round = 0; round < 100; ++round) {
    EXPECT_TRUE(small_buffer_.push_back(static_cast<uint8_t>(round)));
    uint8_t result;
    EXPECT_TRUE(small_buffer_.pop_front(&result));
    EXPECT_EQ(result, static_cast<uint8_t>(round));
  }

  // Buffer should be empty
  uint8_t result;
  EXPECT_FALSE(small_buffer_.pop_front(&result));
}

TEST_F(CircularBufferTest, UsedEntries) {
  EXPECT_EQ(small_buffer_.used_entries(), 0);
  small_buffer_.push_back(1);
  EXPECT_EQ(small_buffer_.used_entries(), 1);
  uint8_t result;
  small_buffer_.pop_front(&result);
  EXPECT_EQ(small_buffer_.used_entries(), 0);
}

TEST_F(CircularBufferTest, ClearBuffer) {
  small_buffer_.push_back(1);
  small_buffer_.push_back(2);
  small_buffer_.clear();
  EXPECT_EQ(small_buffer_.used_entries(), 0);
  uint8_t result;
  EXPECT_FALSE(small_buffer_.pop_front(&result));  // Buffer should be empty
}

}  // namespace