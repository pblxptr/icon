#pragma once

  template<class TExecutor, class THandler>
  void schedule(TExecutor&& context, THandler&& handler)
  {
    boost::asio::post(std::forward<TExecutor>(context), std::forward<THandler>(handler));
  }