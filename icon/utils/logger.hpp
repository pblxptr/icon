#pragma once

#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace icon::utils
{
  constexpr auto LoggerName = "icon";

  inline std::shared_ptr<spdlog::logger> setup_logger(std::vector<spdlog::sink_ptr> sinks = {})
  {
    auto logger = spdlog::get(LoggerName);
    if (!logger)
    {
      if (sinks.size() > 0) {
        logger = std::make_shared<spdlog::logger>(LoggerName,std::begin(sinks), std::end(sinks));

        spdlog::register_logger(logger);

      }
      else {
        logger = spdlog::stdout_color_mt(LoggerName);
      }
    }


    return logger;
  }

  inline std::shared_ptr<spdlog::logger> get_logger()
  {
    auto logger = spdlog::get(LoggerName);
    if (!logger) {
      return setup_logger();
    }

    return logger;
  }
}