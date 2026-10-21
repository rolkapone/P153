#include "enh/version.hpp"

#include <gtest/gtest.h>

#include <algorithm>

TEST(Version, NonVide) {
    EXPECT_FALSE(enh::versionString().empty());
}

TEST(Version, FormatMajeurMineurCorrectif) {
    const auto v = enh::versionString();
    EXPECT_EQ(std::count(v.begin(), v.end(), '.'), 2);
}
