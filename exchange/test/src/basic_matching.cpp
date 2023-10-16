#include "client_manager/client_manager.hpp"
#include "lib.hpp"
#include "matching/engine.hpp"
#include "util/macros.hpp"
#include "util/messages.hpp"

#include <gtest/gtest.h>

using nutc::messages::BUY;
using nutc::messages::SELL;

class BasicMatching : public ::testing::Test {
protected:
    void
    SetUp() override
    {
        manager.add_client("ABC");
        manager.add_client("DEF");
        manager.modify_holdings("ABC", "ETHUSD", 1000);
        manager.modify_holdings("DEF", "ETHUSD", 1000);
    }

    ClientManager manager;
    Engine engine;
};

TEST_F(BasicMatching, SimpleMatch)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"DEF", SELL, "MARKET", "ETHUSD", 1, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", BUY, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", BUY, 1, 0);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
}

TEST_F(BasicMatching, NoMatchThenMatchBuy)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 1, 1000};
    MarketOrder order2{"DEF", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order3{"DEF", BUY, "MARKET", "ETHUSD", 1, 2};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    auto [matches3, ob_updates3] = engine.match_order(order3, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(matches3.size(), 1);
}

TEST_F(BasicMatching, NoMatchThenMatchSell)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"DEF", BUY, "MARKET", "ETHUSD", 1, 1000};

    MarketOrder order3{"DEF", SELL, "MARKET", "ETHUSD", 1, 500};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    auto [matches3, ob_updates3] = engine.match_order(order1, manager);
    auto [matches4, ob_updates4] = engine.match_order(order3, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(matches3.size(), 0);
    EXPECT_EQ(matches4.size(), 1);
}

TEST_F(BasicMatching, PassivePriceMatch)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 1, 2};
    MarketOrder order2{"DEF", SELL, "MARKET", "ETHUSD", 1, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", BUY, 2, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", BUY, 2, 0);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "ABC", "DEF", SELL, 2, 1);
}

TEST_F(BasicMatching, PartialFill)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 2, 1};
    MarketOrder order2{"DEF", SELL, "MARKET", "ETHUSD", 1, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", BUY, 1, 2);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 2);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", BUY, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(1), "ETHUSD", BUY, 1, 1);
}

TEST_F(BasicMatching, MultipleFill)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order3{"DEF", SELL, "MARKET", "ETHUSD", 2, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", BUY, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", BUY, 1, 1);

    auto [matches3, ob_updates3] = engine.match_order(order3, manager);
    EXPECT_EQ(matches3.size(), 2);
    EXPECT_EQ(ob_updates3.size(), 2);
    EXPECT_EQ_MATCH(matches3.at(0), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
    EXPECT_EQ_MATCH(matches3.at(1), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(0), "ETHUSD", BUY, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(1), "ETHUSD", BUY, 1, 0);
}

TEST_F(BasicMatching, MultiplePartialFill)
{
    MarketOrder order1{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"ABC", BUY, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order3{"DEF", SELL, "MARKET", "ETHUSD", 3, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", BUY, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", BUY, 1, 1);

    auto [matches3, ob_updates3] = engine.match_order(order3, manager);
    EXPECT_EQ(matches3.size(), 2);
    EXPECT_EQ(ob_updates3.size(), 3);
    EXPECT_EQ_MATCH(matches3.at(0), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
    EXPECT_EQ_MATCH(matches3.at(1), "ETHUSD", "ABC", "DEF", SELL, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(0), "ETHUSD", BUY, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(1), "ETHUSD", BUY, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(2), "ETHUSD", SELL, 1, 1);
}

TEST_F(BasicMatching, SimpleMatchReversed)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"DEF", BUY, "MARKET", "ETHUSD", 1, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", SELL, 1, 1);
    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
}

TEST_F(BasicMatching, PassivePriceMatchReversed)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"DEF", BUY, "MARKET", "ETHUSD", 1, 2};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", SELL, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ(matches2.at(0).price, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
}

TEST_F(BasicMatching, PartialFillReversed)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 2, 1};
    MarketOrder order2{"DEF", BUY, "MARKET", "ETHUSD", 1, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", SELL, 1, 2);
    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 1);
    EXPECT_EQ(ob_updates2.size(), 2);
    EXPECT_EQ_MATCH(matches2.at(0), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(1), "ETHUSD", SELL, 1, 1);
}

TEST_F(BasicMatching, MultipleFillReversed)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order3{"DEF", BUY, "MARKET", "ETHUSD", 2, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", SELL, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", SELL, 1, 1);

    auto [matches3, ob_updates3] = engine.match_order(order3, manager);
    EXPECT_EQ(matches3.size(), 2);
    EXPECT_EQ(ob_updates3.size(), 2);
    EXPECT_EQ_MATCH(matches3.at(0), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
    EXPECT_EQ_MATCH(matches3.at(1), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(0), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(1), "ETHUSD", SELL, 1, 0);
}

TEST_F(BasicMatching, MultiplePartialFillReversed)
{
    MarketOrder order1{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order2{"ABC", SELL, "MARKET", "ETHUSD", 1, 1};
    MarketOrder order3{"DEF", BUY, "MARKET", "ETHUSD", 3, 1};
    auto [matches, ob_updates] = engine.match_order(order1, manager);
    EXPECT_EQ(matches.size(), 0);
    EXPECT_EQ(ob_updates.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates.at(0), "ETHUSD", SELL, 1, 1);

    auto [matches2, ob_updates2] = engine.match_order(order2, manager);
    EXPECT_EQ(matches2.size(), 0);
    EXPECT_EQ(ob_updates2.size(), 1);
    EXPECT_EQ_OB_UPDATE(ob_updates2.at(0), "ETHUSD", SELL, 1, 1);

    auto [matches3, ob_updates3] = engine.match_order(order3, manager);
    EXPECT_EQ(matches3.size(), 2);
    EXPECT_EQ(ob_updates3.size(), 3);
    EXPECT_EQ_MATCH(matches3.at(0), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
    EXPECT_EQ_MATCH(matches3.at(1), "ETHUSD", "DEF", "ABC", BUY, 1, 1);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(0), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(1), "ETHUSD", SELL, 1, 0);
    EXPECT_EQ_OB_UPDATE(ob_updates3.at(2), "ETHUSD", BUY, 1, 1);
}
