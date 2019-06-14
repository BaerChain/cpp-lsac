#pragma once

#include <libdevcrypto/Common.h>
#include <libdevcore/FixedHash.h>
#include <brcd/buildinfo.h>
#include <iostream>
#include <mutex>
#include <jsonrpccpp/client/connectors/httpclient.h>

namespace dev{
namespace brc{

struct monitorData
{
    unsigned blocknum;
    h256 blockhash;
    size_t packagetranscations;
    size_t pendingpoolsnum;
    size_t nodenum;
};


class NodeMonitor
{
public:
    NodeMonitor(bytes _networkrlp, std::string _ip);
    ~NodeMonitor(){}
    void run();
    
    void getKeypair();
    void getClientVersion();
    void setData(monitorData _data);
    void exitThread(bool _flags) {m_mutex.lock(); m_threadExit = true; m_mutex.unlock();}
    Signature signatureData() const;
    void sendNodeStats(Signature _sign);
private:
    bool m_threadExit = false;
    jsonrpc::HttpClient m_httpClient;
    std::string m_ip;
    bytes m_networkrlp;
    std::string m_clientVersion;
    std::vector<monitorData> m_data;
    std::mutex m_mutex;
    Secret m_secret;
    Public m_public;
    Address m_addr;

};



}
}