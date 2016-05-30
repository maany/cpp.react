
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_FORWARD_H_INCLUDED
#define REACT_FORWARD_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename Op>
class OpSignal;

template <typename TSignal>
using ValueT = typename std::decay_t<TSignal>::value_type;

template <typename ... S>
struct SignalPack;

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

template <typename D>
class Observer;

template <typename D>
class ScopedObserver;

template <typename TSourceDomain, typename TTargetDomain>
class Continuation;

/******************************************/ REACT_END /******************************************/

#endif // REACT_FORWARD_H_INCLUDED
