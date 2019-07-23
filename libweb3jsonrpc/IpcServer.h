#pragma once

#if _WIN32
#include "WinPipeServer.h"
#else
#include "UnixSocketServer.h"
#endif

namespace dev
{
#if _WIN32
using IpcServer = WindowsPipeServer;
#else
using IpcServer = UnixDomainSocketServer;
#endif
}  // namespace dev
