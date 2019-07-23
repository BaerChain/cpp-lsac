#pragma once

#include "IpcServerBase.h"

#include <windows.h>

namespace dev
{
class WindowsPipeServer : public IpcServerBase<HANDLE>
{
public:
    WindowsPipeServer(std::string const& _appId);

protected:
    void Listen() override;
    void CloseConnection(HANDLE _socket) override;
    size_t Write(HANDLE _connection, std::string const& _data) override;
    size_t Read(HANDLE _connection, void* _data, size_t _size) override;
};

}  // namespace dev
