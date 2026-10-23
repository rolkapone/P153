#include "enh/demo_counter.hpp"

#include <gtest/gtest.h>

#include <limits>

TEST(DemoCounter, CommenceAZero) {
    const enh::DemoCounter counter;
    EXPECT_EQ(counter.count(), 0);
}

TEST(DemoCounter, IncrementeDeUn) {
    enh::DemoCounter counter;
    EXPECT_TRUE(counter.increment());
    EXPECT_TRUE(counter.increment());
    EXPECT_EQ(counter.count(), 2);
}

TEST(DemoCounter, ValeurDeDepartNegativeRameneeAZero) {
    const enh::DemoCounter counter(-5);
    EXPECT_EQ(counter.count(), 0);
}

TEST(DemoCounter, SatureAuMaximumSansDepassement) {
    constexpr int kMax = std::numeric_limits<int>::max();
    enh::DemoCounter counter(kMax - 1);
    EXPECT_TRUE(counter.increment());
    EXPECT_EQ(counter.count(), kMax);
    EXPECT_FALSE(counter.increment());
    EXPECT_EQ(counter.count(), kMax);
}
