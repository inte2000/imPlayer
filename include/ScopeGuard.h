#pragma once

#include <functional>

//Facebook folly
class ScopeGuardBase 
{
public:
    void Dismiss() noexcept { m_bActive = false; }
protected:
    ScopeGuardBase(bool active = true) noexcept : m_bActive(active) {}

    static ScopeGuardBase MakeEmptyGuard() noexcept {
        return ScopeGuardBase{};
    }
protected:
    bool m_bActive;
};

template<typename FuncType>
class CScopeGuard : public ScopeGuardBase {
public:
    CScopeGuard() { m_bActive = false; }

    explicit CScopeGuard(FuncType& fn)
        : CScopeGuard(std::as_const(fn), MakeFailSafe(std::is_nothrow_copy_constructible<FuncType>{}, &fn)) {
    }

    explicit CScopeGuard(const FuncType& fn)
        : CScopeGuard(fn, MakeFailSafe(std::is_nothrow_copy_constructible<FuncType>{}, &fn)) {
    }

    explicit CScopeGuard(FuncType&& fn)
        : CScopeGuard(std::move(fn), MakeFailSafe(std::is_nothrow_move_constructible<FuncType>{}, &fn)) {
    }

    CScopeGuard(CScopeGuard&& other) : m_func(std::move(other.m_func)) {
        m_bActive = std::exchange(other.m_bActive, false);
    }

    void OnGuard(FuncType& fn) {
        auto failsafe = MakeFailSafe(std::is_nothrow_copy_constructible<FuncType>{}, &fn);
        m_func = std::as_const(fn);
        m_bActive = true;
        failsafe.Dismiss();
    }

    void OnGuard(const FuncType& fn) {
        auto failsafe = MakeFailSafe(std::is_nothrow_copy_constructible<FuncType>{}, &fn);
        m_func = fn;
        m_bActive = true;
        failsafe.Dismiss();
    }
    void OnGuard(FuncType&& fn) {
        auto failsafe = MakeFailSafe(std::is_nothrow_move_constructible<FuncType>{}, &fn);
        m_func = std::move(fn);
        m_bActive = true;
        failsafe.Dismiss();
    }

    ~CScopeGuard() {
        if (m_bActive) {
            m_func();
        }
    }
private:
    static ScopeGuardBase MakeFailSafe(std::true_type, const void*) noexcept {
        return MakeEmptyGuard();
    }

    template <typename Fn>
    static auto MakeFailSafe(std::false_type, Fn* fn) noexcept
        -> CScopeGuard<decltype(std::ref(*fn))> {
        return CScopeGuard<decltype(std::ref(*fn))>{std::ref(*fn)};
    }

    template <typename Fn>
    explicit CScopeGuard(Fn&& fn, ScopeGuardBase&& failsafe)
        : ScopeGuardBase{}, m_func(std::forward<Fn>(fn)) {
        failsafe.Dismiss();
    }

private:
    FuncType m_func;
};

template <typename F>
CScopeGuard<std::decay_t<F>> MakeGuard(F&& f) 
{
    return CScopeGuard<std::decay_t<F>>(std::forward<F>(f));
}
