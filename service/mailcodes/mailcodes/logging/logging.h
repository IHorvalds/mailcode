#ifndef LOGGING_H
#define LOGGING_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ranges.h>

#include "common/paths.h"

spdlog::logger& getDefaultLogger(std::string const& svModuleName);

spdlog::logger& getSystemEventLogger();

#ifndef LOGGER
#define LOGGER() getDefaultLogger(MODULE_NAME)
#endif

#ifndef ENTERED
#define ENTERED() LOGGER().debug("Entered " __FUNCTION__)
#endif

#ifndef FINISHED
#define FINISHED(...)                                                          \
    {                                                                          \
        std::tuple args = std::make_tuple(__VA_ARGS__);                        \
        if (std::tuple_size<decltype(args)> {}())                              \
            LOGGER().debug("Finished " __FUNCTION__ ": {}",                    \
                           spdlog::fmt_lib::join(args, ", "));                 \
        else                                                                   \
            LOGGER().debug("Finished " __FUNCTION__);                          \
    }
#endif

#endif // LOGGING_H
