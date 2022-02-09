#pragma once

#include <boost/asio.hpp>
#include <icon/utils/logger.hpp>
#include <queue>
#include <atomic>

namespace icon::details::experimental {
template<class Executor>
class AsyncMutex
{
public:
  using Timer_t = boost::asio::steady_timer;

  explicit AsyncMutex(Executor& executor)
    : executor_{ executor }
  {}

  AsyncMutex(const AsyncMutex&) = delete;
  AsyncMutex(AsyncMutex&&) = delete;
  AsyncMutex& operator=(const AsyncMutex&) = delete;
  AsyncMutex& operator=(AsyncMutex&&) = delete;

  boost::asio::awaitable<void> async_lock()
  {
    icon::utils::get_logger()->debug("AsyncMutex: async_lock");

    if (counter_++ == 0) {
      icon::utils::get_logger()->debug("AsyncMutex: first lock, retrun");
      co_return;
    }

    icon::utils::get_logger()->debug("AsyncMutex: trying to acquire lock");

    auto ec = boost::system::error_code{};
    auto timer = Timer_t{ executor_ };
    timer.expires_after(boost::asio::steady_timer::duration::max());
    waiters_.push(&timer);

    icon::utils::get_logger()->debug("AsyncMutex: lock already acquired, waiting");
    co_await timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    icon::utils::get_logger()->debug("AsyncMutex: locl acquired");
  }

  void unlock()
  {
    if (counter_.load() == 0) {
      return;
    }

    --counter_;

    if (waiters_.empty()) {
      return;
    }

    icon::utils::get_logger()->debug("AsyncMutex: unlock");

    auto* next = waiters_.front();
    next->cancel();

    waiters_.pop();
  }

private:
  Executor& executor_;
  std::queue<Timer_t*> waiters_{};
  std::atomic<int> counter_;
};
}// namespace icon::details::experimental