/* Start Header ************************************************************************/
/*!
\file       Logger.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the Logger system.
            This file is used to create a logger using spdlog.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Logger.h"

using namespace Ermine;

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

void Logger::Init()
{
    spdlog::set_pattern("%^[%T] %n: %v%$");

    s_CoreLogger = spdlog::stdout_color_mt("Ermine Engine: ");
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = spdlog::stdout_color_mt("Ermine Editor: ");
    s_ClientLogger->set_level(spdlog::level::trace);
}
