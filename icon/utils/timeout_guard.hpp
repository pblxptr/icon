#pragma once

#include <spdlog/spdlog.h>
#include <chrono>

namespace icon::details {
template<class Executor, class Handler>
class TimeoutGuard
{
public:
  template<class ExecutorA, class HandlerA, class Duration = std::chrono::milliseconds>
  TimeoutGuard(ExecutorA&& executor, HandlerA&& handler, Duration duration = std::chrono::milliseconds{ 0 })
    : timer_{ std::forward<ExecutorA>(executor) }, handler_{ std::forward<HandlerA>(handler) }, duration_{ std::move(duration) }
  {
    spdlog::debug("TimeoutGuard: ctor");
  }

  template<class Duration>
  void setup(Duration duration)
  {
    duration_ = std::move(duration);
  }

  void spawn()
  {
    if (duration_.count() == 0) {
      return;
    }

    expired_ = false;

    timer_.expires_after(duration_);

    boost::asio::co_spawn(
      timer_.get_executor(), [this]() -> awaitable<void> {
          auto ec = boost::system::error_code{};
          co_await timer_.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));

          if (ec != boost::asio::error::operation_aborted)
          {
            spdlog::debug("TimeoutGuard: timer expired, calling expiration handler.");
            handle_expired();
          }
          else {
            spdlog::debug("TimeoutGuard: operation cancelled.");
          } }, [](auto eptr) { if (eptr) { std::rethrow_exception(eptr); } });
  }

  bool expired() const
  {
    return expired_;
  }

private:
  void handle_expired()
  {
    expired_ = true;
    handler_();
  }

private:
  boost::asio::steady_timer timer_;
  Handler handler_;
  std::chrono::milliseconds duration_;
  bool expired_{ false };
};

template<class Executor, class Handler, class Duration>
TimeoutGuard(Executor&&, Handler&&, Duration) -> TimeoutGuard<Executor, Handler>;
}// namespace icon::details