#include "enh/app/glass_quality_governor.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <limits>

namespace {

using enh::app::GlassQuality;
using enh::app::GlassQualityGovernor;
using Ms = GlassQualityGovernor::Milliseconds;
using namespace std::chrono_literals;

// Envoie `frames` mesures identiques, une toutes les 16 ms a partir de `t` (avance `t`).
GlassQuality feed(GlassQualityGovernor& g, double ms, int frames, bool processing,
                  GlassQualityGovernor::Clock::time_point& t) {
    GlassQuality q = g.quality();
    for (int i = 0; i < frames; ++i) {
        q = g.update(Ms{ms}, processing, t);
        t += 16ms;
    }
    return q;
}

class GovernorTest : public ::testing::Test {
protected:
    GlassQualityGovernor governor;
    GlassQualityGovernor::Clock::time_point t{}; // horloge factice : le test fait avancer le temps
};

} // namespace

TEST_F(GovernorTest, AuReposToujoursComplet) {
    EXPECT_EQ(feed(governor, 50.0, 100, false, t), GlassQuality::Full);
    EXPECT_FALSE(governor.average().has_value()); // l'historique est oublie hors traitement
}

TEST_F(GovernorTest, AttendTrenteImagesAvantDeDecider) {
    EXPECT_EQ(feed(governor, 20.0, 29, true, t), GlassQuality::Full);
    EXPECT_FALSE(governor.average().has_value());
    EXPECT_EQ(feed(governor, 20.0, 1, true, t), GlassQuality::Opaque);
}

TEST_F(GovernorTest, PlusDeQuatreMsDonneAllege) {
    EXPECT_EQ(feed(governor, 5.0, 30, true, t), GlassQuality::Lite);
    ASSERT_TRUE(governor.average().has_value());
    EXPECT_DOUBLE_EQ(governor.average()->count(), 5.0);
}

TEST_F(GovernorTest, SeuilsStrictsQuatreEtHuitMs) {
    EXPECT_EQ(feed(governor, 4.0, 30, true, t), GlassQuality::Full);
    EXPECT_EQ(feed(governor, 8.0, 30, true, t), GlassQuality::Lite);
    EXPECT_EQ(feed(governor, 8.5, 30, true, t), GlassQuality::Opaque);
}

TEST_F(GovernorTest, AllegeNeRemontePasAvantDixSecondes) {
    feed(governor, 9.0, 30, true, t);
    ASSERT_EQ(governor.quality(), GlassQuality::Opaque);
    // 30 images sous le budget pour vider la fenetre, puis presque 10 s de calme.
    feed(governor, 0.5, 30, true, t);
    const auto start = t;
    while (t - start < 9900ms) {
        governor.update(Ms{0.5}, true, t);
        t += 100ms;
    }
    EXPECT_EQ(governor.quality(), GlassQuality::Opaque);
}

TEST_F(GovernorTest, RemonteDUnCranToutesLesDixSecondes) {
    feed(governor, 9.0, 30, true, t);
    ASSERT_EQ(governor.quality(), GlassQuality::Opaque);
    feed(governor, 0.5, 30, true, t); // la moyenne passe sous 1 ms : debut du calme
    const auto calm = t - 16ms;
    governor.update(Ms{0.5}, true, calm + 10s);
    EXPECT_EQ(governor.quality(), GlassQuality::Lite);
    governor.update(Ms{0.5}, true, calm + 15s);
    EXPECT_EQ(governor.quality(), GlassQuality::Lite); // le cran suivant redemande 10 s
    governor.update(Ms{0.5}, true, calm + 20s);
    EXPECT_EQ(governor.quality(), GlassQuality::Full);
}

TEST_F(GovernorTest, PicAuDessusDuBudgetInterromptLeCalme) {
    feed(governor, 5.0, 30, true, t);
    ASSERT_EQ(governor.quality(), GlassQuality::Lite);
    feed(governor, 0.2, 30, true, t);
    feed(governor, 3.0, 30, true, t); // entre 1 et 4 ms : le compteur de calme repart a zero
    feed(governor, 0.2, 30, true, t);
    governor.update(Ms{0.2}, true, t + 9s);
    EXPECT_EQ(governor.quality(), GlassQuality::Lite);
}

TEST_F(GovernorTest, ValeursAberrantesIgnoreesOuBornees) {
    constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
    constexpr double kInf = std::numeric_limits<double>::infinity();
    feed(governor, kNan, 30, true, t);
    feed(governor, -3.0, 30, true, t);
    feed(governor, kInf, 30, true, t);
    EXPECT_FALSE(governor.average().has_value());
    EXPECT_EQ(governor.quality(), GlassQuality::Full);
    feed(governor, 1.0e12, 30, true, t);
    ASSERT_TRUE(governor.average().has_value());
    EXPECT_DOUBLE_EQ(governor.average()->count(), GlassQualityGovernor::kMaxSample.count());
    EXPECT_EQ(governor.quality(), GlassQuality::Opaque);
}

TEST_F(GovernorTest, FinDeTraitementRetablitLeVerreComplet) {
    feed(governor, 9.0, 30, true, t);
    ASSERT_EQ(governor.quality(), GlassQuality::Opaque);
    EXPECT_EQ(governor.update(Ms{9.0}, false, t), GlassQuality::Full);
}
