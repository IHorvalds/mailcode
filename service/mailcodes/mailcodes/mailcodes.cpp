#include <iostream>
#include <format>
#include <boost/program_options.hpp>

#define MODULE_NAME "service-manager-cli"

#include "logging/logging.h"
#include "service/service-manager.h"
#include "common/utility-macros.h"
#include "service/mail-service.h"

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    if (argc < 1)
    {
        return EXIT_FAILURE;
    }

    std::filesystem::path execPath(argv[0]);

    if (!SetCurrentDirectory(execPath.parent_path().c_str()))
    {
        std::cerr << "Failed to set working directory. Bailing\n";
        return EXIT_FAILURE;
    }

    // clang-format off
    po::options_description options(std::string(argv[0]) + " service manager");
    options.add_options()
        ("help,h", "Show this help message")
        ("install", "Install the service. To also start, add --start")
        ("uninstall", "Uninstall the service.")
        ("start", "Start the service. It must be already installed.")
        ("stop", "Stop the service");
    // clang-format on
    po::variables_map var_map;

    try
    {
        po::store(po::parse_command_line(argc, argv, options), var_map);

        if (var_map.count("help"))
        {
            std::cout << options << std::endl;
            return 0;
        }

        unsigned short args = 0;

        args |= (var_map.count("install") > 0) << 3;
        args |= (var_map.count("uninstall") > 0) << 2;
        args |= (var_map.count("start") > 0) << 1;
        args |= (var_map.count("stop") > 0);

        if (args == 0)
        {
            Mailservice mailservice;
            if (!CServiceBase::Run(mailservice))
            {
                throw std::runtime_error(
                    std::format("Service failed to run with err {:#016Lx}",
                                GetLastError()));
            }

            return EXIT_SUCCESS;
        }

        // impossible actions
        if (COMPARE_MASK(args, 0b1100)) // install + uninstall
        {
            throw std::runtime_error(
                "Cannot install and uninstall at the same time");
        }
        if (COMPARE_MASK(args, 0b11)) // start + stop
        {
            throw std::runtime_error("Cannot stop and start at the same time");
        }
        if (COMPARE_MASK(args, 0b110)) // uninstall + start
        {
            throw std::runtime_error(
                "Cannot uninstall and start at the same time");
        }
        // done validating arguments

        // do install/uninstall
        if (COMPARE_MASK(args, 0b1000))
        {
            installService();
        }
        else if (COMPARE_MASK(args, 0b100))
        {
            uninstallService();
        }
        else if (COMPARE_MASK(args, 0b10))
        {
            startService();
        }
        else if (COMPARE_MASK(args, 0b1))
        {
            stopService();
        }
        else
        {
            Mailservice mailservice;
            if (!CServiceBase::Run(mailservice))
            {
                throw std::runtime_error(
                    std::format("Service failed to run with err {:#016Lx}",
                                GetLastError()));
            }
        }

        // do start/stop
        // if (COMPARE_MASK(args, 0b10))
        //{
        //    startService();

        //    /*Mailservice mailservice;
        //    if (!CServiceBase::Run(mailservice))
        //    {
        //        throw std::runtime_error(std::format("Service failed to run
        //        with err {:#016Lx}", GetLastError()));
        //    }*/
        //}
        // else if (COMPARE_MASK(args, 0b1))
        //{
        //    stopService();
        //}
    }
    catch (const po::error& roError)
    {
        auto error = std::format("Argument error: {}", roError.what());
        std::cerr << error << "\n";
        LOGGER().error(error);
    }
    catch (const std::runtime_error& roError)
    {
        auto error = std::format("Runtime error: {}", roError.what());
        std::cerr << error << "\n";
        LOGGER().error(error);
    }

    return EXIT_SUCCESS;
}
