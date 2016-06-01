
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED

#include "react/detail/Defs.h"

#include <memory>
#include <utility>
#include <type_traits>

#include "GraphBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename L, typename R>
bool Equals(const L& lhs, const R& rhs);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class SignalNode : public ObservableNode<D>
{
public:

    template <typename T>
    explicit SignalNode(T&& value) :
        value_{std::forward<T>(value)}
    {}

    const S& ValueRef() const
    {
        return value_;
    }

protected:
    S value_;
};

template <typename D, typename S>
using SignalNodePtrT = std::shared_ptr<SignalNode<D,S>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class VarNode :
    public SignalNode<D,S>,
    public IInputNode
{
    using Engine = typename VarNode::Engine;

public:
    template <typename T>
    VarNode(T&& value) :
        VarNode::SignalNode( std::forward<T>(value) )
    {
        Engine::OnNodeCreate(*this);
    }

    ~VarNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "VarNode"; }
    virtual bool        IsInputNode() const override        { return true; }
    virtual int         DependencyCount() const override    { return 0; }

    virtual void Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Ticked VarNode\n");
    }

    template <typename V>
    void AddInput(V&& newValue)
    {
        pushNewValue(std::forward<V>(newValue));

        isInputAdded_ = true;

        // isInputAdded_ takes precedences over isInputModified_
        // the only difference between the two is that isInputModified_ doesn't/can't compare
        isInputModified_ = false;
    }

    // This is signal-specific
    template <typename F>
    void ModifyInput(F& func)
    {
        // There hasn't been any Set(...) input yet, modify.
        if (! isInputAdded_)
        {
            func(this->value_);

            isInputModified_ = true;
        }
        // There's a newValue, modify newValue instead.
        // The modified newValue will handled like before, i.e. it'll be compared to value_
        // in ApplyInput
        else
        {
            func(getNewValue());
        }
    }

    virtual bool ApplyInput(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        if (isInputAdded_)
        {
            isInputAdded_ = false;

            if (! Equals(this->value_, getNewValue()))
            {
                popNewValue();
                Engine::OnInputChange(*this, turn);
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (isInputModified_)
        {            
            isInputModified_ = false;

            Engine::OnInputChange(*this, turn);
            return true;
        }

        else
        {
            return false;
        }
    }

private:

    S & getNewValue()
    {
        return *reinterpret_cast<S*>(newValueData);
    }

    template <typename V>
    void pushNewValue(V && v)
    {
        new (newValueData) S{std::forward<V>(v)};
    }

    void popNewValue()
    {
        this->value_ = std::move(*reinterpret_cast<S*>(newValueData));
    }

    // so that we don't require S to be default constructible
    std::aligned_storage_t<sizeof(S), alignof(S)> newValueData[1];

    bool    isInputAdded_ = false;
    bool    isInputModified_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FunctionOp
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename S,
    typename F,
    typename ... TDeps
>
class FunctionOp : public ReactiveOpBase<TDeps...>
{
public:
    template <typename FIn, typename ... TDepsIn>
    FunctionOp(FIn&& func, TDepsIn&& ... deps) :
        FunctionOp::ReactiveOpBase( DontMove(), std::forward<TDepsIn>(deps) ... ),
        func_( std::forward<FIn>(func) )
    {}

    S Evaluate() 
    {
        return apply(EvalFunctor( func_ ), this->deps_);
    }

private:
    // Eval
    struct EvalFunctor
    {
        EvalFunctor(F& f) : MyFunc( f )   {}

        template <typename ... T>
        S operator()(T&& ... args)
        {
            return MyFunc(eval(args) ...);
        }

        template <typename T>
        static auto eval(T& op) -> decltype(op.Evaluate())
        {
            return op.Evaluate();
        }

        template <typename T>
        static auto eval(const std::shared_ptr<T>& depPtr) -> decltype(depPtr->ValueRef())
        {
            return depPtr->ValueRef();
        }

        F& MyFunc;
    };

private:
    F   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalOpNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename TOp
>
class SignalOpNode :
    public SignalNode<D,S>
{
    using Engine = typename SignalOpNode::Engine;

public:
    explicit SignalOpNode(TOp op) :
        SignalOpNode::SignalNode{op.Evaluate()},
        op_{std::move(op)}
    {
        Engine::OnNodeCreate(*this);
        op_.template Attach<D>(*this);
    }

    ~SignalOpNode()
    {
        if (!wasOpStolen_)
            op_.template Detach<D>(*this);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        bool changed = false;

        {// timer
            using TimerT = typename SignalOpNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, 1 );

            S newValue = op_.Evaluate();

            if (! Equals(this->value_, newValue))
            {
                this->value_ = std::move(newValue);
                changed = true;
            }
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SignalOpNode"; }
    virtual int         DependencyCount() const override    { return TOp::dependency_count; }

    TOp StealOp()
    {
        REACT_ASSERT(wasOpStolen_ == false, "Op was already stolen.");
        wasOpStolen_ = true;
        op_.template Detach<D>(*this);
        return std::move(op_);
    }

private:
    TOp     op_;
    bool    wasOpStolen_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TOuter,
    typename TInner
>
class FlattenNode : public SignalNode<D,TInner>
{
    using Engine = typename FlattenNode::Engine;

public:
    FlattenNode(const std::shared_ptr<SignalNode<D,TOuter>>& outer,
                const std::shared_ptr<SignalNode<D,TInner>>& inner) :
        FlattenNode::SignalNode( inner->ValueRef() ),
        outer_( outer ),
        inner_( inner )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *outer_);
        Engine::OnNodeAttach(*this, *inner_);
    }

    ~FlattenNode()
    {
        Engine::OnNodeDetach(*this, *inner_);
        Engine::OnNodeDetach(*this, *outer_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        auto newInner = GetNodePtr(outer_->ValueRef());

        if (newInner != inner_)
        {
            // Topology has been changed
            auto oldInner = inner_;
            inner_ = newInner;

            Engine::OnDynamicNodeDetach(*this, *oldInner, turn);
            Engine::OnDynamicNodeAttach(*this, *newInner, turn);

            return;
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! Equals(this->value_, inner_->ValueRef()))
        {
            this->value_ = inner_->ValueRef();
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual const char* GetNodeType() const override        { return "FlattenNode"; }
    virtual bool        IsDynamicNode() const override      { return true; }
    virtual int         DependencyCount() const override    { return 2; }

private:
    std::shared_ptr<SignalNode<D,TOuter>>   outer_;
    std::shared_ptr<SignalNode<D,TInner>>   inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED
