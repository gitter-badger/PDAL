/******************************************************************************
* Copyright (c) 2013, Howard Butler (hobu.inc@gmail.com)
* Copyright (c) 2014-2015, Bradley J Chambers (brad.chambers@gmail.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include <pdal/GlobalEnvironment.hpp>
#include <pdal/KernelFactory.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/pdal_config.hpp>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace pdal;

std::string headline(Utils::screenWidth(), '-');

std::string splitDriverName(std::string const& name)
{
    std::string out;

    StringList names = Utils::split2(name, '.');
    if (names.size() == 2)
        out = names[1];
    return out;
}

void outputVersion()
{
    std::cout << headline << std::endl;
    std::cout << "pdal " << GetFullVersionString() << std::endl;
    std::cout << headline << std::endl;
    std::cout << std::endl;
}


void outputCommands(int indent)
{
    KernelFactory f(false);

    std::string leading(indent, ' ');

    for (auto name : PluginManager::names(PF_PluginType_Kernel))
        std::cout << leading << splitDriverName(name) << std::endl;
}


void outputHelp(ProgramArgs& args)
{
    std::cout << "usage: pdal <options | command>";
    args.dump(std::cout, 2, Utils::screenWidth());
    std::cout << std::endl;

    std::cout << "The following commands are available:" << std::endl;

    outputCommands(2);
    std::cout << std::endl;
    std::cout << "See http://pdal.io/apps.html for more detail" << std::endl;
}


void outputDrivers()
{
    // Force plugin loading.
    StageFactory f(false);

    std::ostringstream strm;

    int nameColLen(25);
    int descripColLen(Utils::screenWidth() - nameColLen - 1);

    std::string tablehead(std::string(nameColLen, '=') + ' ' +
        std::string(descripColLen, '='));

    strm << std::endl;
    strm << tablehead << std::endl;
    strm << std::left << std::setw(nameColLen) << "Name" <<
        " Description" << std::endl;
    strm << tablehead << std::endl;

    strm << std::left;

    StringList stages = PluginManager::names(PF_PluginType_Filter |
        PF_PluginType_Reader | PF_PluginType_Writer);
    for (auto name : stages)
    {
        std::string descrip = PluginManager::description(name);
        StringList lines = Utils::wordWrap(descrip, descripColLen - 1);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            strm << std::setw(nameColLen) << name << " " <<
                lines[i] << std::endl;
            name.clear();
        }
    }

    strm << tablehead << std::endl;
    std::cout << strm.str() << std::endl;
}

void outputOptions(std::string const& n)
{
    // Force plugin loading.
    StageFactory f(false);

    std::unique_ptr<Stage> s(f.createStage(n));
    if (!s)
    {
        std::cerr << "Unable to create stage " << n << "\n";
        return;
    }

    std::string link = PluginManager::link(n);
    std::cout << n << " -- " << link << std::endl;
    std::cout << headline << std::endl;

    std::vector<Option> options = s->getDefaultOptions().getOptions();
    if (options.empty())
    {
        std::cout << "No options" << std::endl << std::endl;
        return;
    }

    for (auto const& opt : options)
    {
        std::string name = opt.getName();
        std::string defVal = Utils::escapeNonprinting(
            opt.getValue<std::string>());
        std::string description = opt.getDescription();

        std::cout << name;
        if (!defVal.empty())
            std::cout << " [" << defVal << "]";
        std::cout << std::endl;

        if (!description.empty())
        {
            StringList lines =
                Utils::wordWrap(description, headline.size() - 6);
            for (std::string& line : lines)
                std::cout << "    " << line << std::endl;
        }
        std::cout << std::endl;
    }
}


void outputOptions()
{
    // Force plugin loading.
    StageFactory f(false);

    StringList nv = PluginManager::names(PF_PluginType_Filter |
        PF_PluginType_Reader | PF_PluginType_Writer);
    for (auto const& n : nv)
        outputOptions(n);
}


int main(int argc, char* argv[])
{
    int verbose;
    bool listDrivers;
    bool listCommands;
    bool help;
    std::string logFilename;
    std::string driverOptions;
    bool version;
    bool printBuild;

    ProgramArgs args;

    Arg& verboseArg = args.add("verbose,v",
        "Output level (error=0, debug=3, max=8)", verbose);
    args.add("drivers", "List all available drivers", listDrivers);
    args.add("driver-options", "Show options for a driver", driverOptions);
    args.add("options", "Show options for a driver", driverOptions).setHidden();
    args.add("help,h", "Display program help.", help);
    Arg& logFilenameArg = args.add("log",
        "Destination filename for log output", logFilename);
    args.add("version", "Display PDAL version", version);
    args.add("list-commands", "List available commands", listCommands);
    args.add("build-info", "Print build information", printBuild);
    args.add("debug", "Print build information", printBuild).setHidden();

    // No arguments, print basic usage, plugins will be loaded
    if (argc < 2)
    {
        outputHelp(args);
        return 1;
    }

    // Discover available kernels without plugins, and test to see if
    // the positional option 'command' is a valid kernel
    KernelFactory f(true);
    std::vector<std::string> loaded_kernels;
    loaded_kernels = PluginManager::names(PF_PluginType_Kernel);

    bool isValidKernel = false;
    std::string command(argv[1]);
    std::string fullname;
    for (auto name : loaded_kernels)
    {
        if (boost::iequals(argv[1], splitDriverName(name)))
        {
            fullname = name;
            isValidKernel = true;
            break;
        }
    }

    // If the kernel was not available, then light up the plugins and retry
    if (!isValidKernel)
    {
        KernelFactory f(false);
        loaded_kernels.clear();
        loaded_kernels = PluginManager::names(PF_PluginType_Kernel);

        for (auto name : loaded_kernels)
        {
            if (boost::iequals(argv[1], splitDriverName(name)))
            {
                fullname = name;
                isValidKernel = true;
                break;
            }
        }
    }

    // Dispatch execution to the kernel, passing all remaining args
    if (isValidKernel)
    {
        int count(argc - 2); // remove 'pdal' and the kernel name
        argv += 2;
        void *kernel = PluginManager::createObject(fullname);
        std::unique_ptr<Kernel> app(static_cast<Kernel *>(kernel));
        return app->run(count, const_cast<char const **>(argv), command);
    }

    // Remove the program name.
    argv++;
    argc--;

    args.parse(argc, argv);

    if (help)
    {
        outputHelp(args);
        return 0;
    }
    if (version)
    {
        outputVersion();
        return 0;
    }
    if (printBuild)
    {
        std::cerr << getPDALDebugInformation() << std::endl;
        return 0;
    }

    // Should probably load log stuff.
    if (logFilenameArg.set())
        GlobalEnvironment::get().setLogFilename(logFilename);
    if (verboseArg.set())
        GlobalEnvironment::get().setLogLevel(LogLevel::Enum(verbose));
    if (listCommands)
    {
        outputCommands(0);
        return 0;
    }
    if (listDrivers)
    {
        outputDrivers();
        return 0;
    }
    if (driverOptions.size())
    {
        outputOptions(driverOptions);
        return 0;
    }
}

