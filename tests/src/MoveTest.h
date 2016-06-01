
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include "react/Domain.h"
#include "react/Signal.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MoveTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TParams>
class MoveTest : public testing::Test
{
public:
    template <EPropagationMode mode>
    class MyEngine : public TParams::template EngineT<mode> {};

    REACTIVE_DOMAIN(MyDomain, TParams::mode, MyEngine)

    struct Stats
    {
        int moveCount = 0;
    };

    struct CopyCounter
    {
        int v = 0;
        Stats* stats = nullptr;

        CopyCounter(int x, Stats* s) :
            v( x ),
            stats( s )
        {}

        CopyCounter(const CopyCounter& other) = delete;
        CopyCounter& operator=(const CopyCounter& other) = delete;

        CopyCounter(CopyCounter&& other) :
            v( other.v ),
            stats( other.stats )
        {
            stats->moveCount++;
        }

        CopyCounter& operator=(CopyCounter&& other)
        {
            v = other.v;
            stats = other.stats;
            stats->moveCount++;
            return *this;
        }

        CopyCounter operator+(const CopyCounter& r) const
        {
            return CopyCounter{ v + r.v, stats };
        }

        bool operator==(const CopyCounter& other) const
        {
            return v == other.v;
        }
    };
};

TYPED_TEST_CASE_P(MoveTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Copy1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(MoveTest, Copy1)
{
    using D = typename Copy1::MyDomain;
    using CopyCounter = typename Copy1::CopyCounter;
    using Stats = typename Copy1::Stats;

    Stats stats1;

    auto a = MakeVar<D>(CopyCounter{1,&stats1});
    auto b = MakeVar<D>(CopyCounter{10,&stats1});
    auto c = MakeVar<D>(CopyCounter{100,&stats1});
    auto d = MakeVar<D>(CopyCounter{1000,&stats1});

    // 4x move to value_
    ASSERT_EQ(4, stats1.moveCount);

    auto x = a + b + c + d;

    ASSERT_EQ(7, stats1.moveCount);
    ASSERT_EQ(1111, x().v);

    a <<= CopyCounter{2,&stats1};

    ASSERT_EQ(10, stats1.moveCount);
    ASSERT_EQ(1112, x().v);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    MoveTest,
    Copy1
);

} // ~namespace
