#pragma once

#include <functional>
#include <tuple>

namespace Notifications
{

template <typename... Args>
struct NotificationCallback
{
    NotificationCallback(std::function<bool(Args...)> _func, Args... _args)
    {
        func = _func;
        args = std::forward_as_tuple(_args...);
    }

    std::function<bool(Args...)> func;
    std::tuple<>                 args;

    bool operator()() const
    {
        bool result = false;
        std::apply(
            [this, &result](auto... a) {
                result = func(std::forward(a)...);
            },
            args);

        return result;
    }
};

} // namespace Notifications