#include <catch2/catch_test_macros.hpp>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <queue>
#include <atomic>

#include <icon/utils/async_mutex.hpp>

using boost::asio::awaitable;
using boost::asio::use_awaitable;

using namespace icon::details::experimental;

TEST_CASE("Multiple coroutines can use the same resource.") {
  auto ctx = boost::asio::io_context{};
  auto timer = boost::asio::steady_timer{ctx};
  auto mutex = AsyncMutex{ctx};
  auto propagage_exception = [](auto eptr) {
    REQUIRE(!eptr);
  };

  auto func = [&timer, &mutex](std::chrono::milliseconds duration) -> awaitable<void> {

    co_await mutex.async_lock();

    auto ec = boost::system::error_code{};
    timer.expires_after(duration);
    co_await timer.async_wait(boost::asio::redirect_error(use_awaitable, ec));

    mutex.unlock();

    if (ec) {
      throw std::runtime_error("Unexpected excepion.");
    }
  };

  co_spawn(ctx, func(std::chrono::milliseconds(300)), propagage_exception);
  co_spawn(ctx, func(std::chrono::milliseconds(200)), propagage_exception);
  co_spawn(ctx, func(std::chrono::milliseconds(100)), propagage_exception);

  ctx.run();
}
