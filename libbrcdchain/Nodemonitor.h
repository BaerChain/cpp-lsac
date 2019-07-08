#pragma once

#include <libdevcrypto/Common.h>
#include <libdevcore/FixedHash.h>
#include <libp2p/Common.h>
#include <brcd/buildinfo.h>
#include <iostream>
#include <mutex>
#include <jsonrpccpp/client/connectors/httpclient.h>

namespace dev{
namespace brc{

struct monitorData
{
    unsigned blocknum;
    Address blockAuthor;
    h256 blockhash;
    u256 blockgasused;
    int64_t blockDelay;
    size_t packagetranscations;
    size_t pendingpoolsnum;
    size_t nodenum;
    int64_t time;
    dev::p2p::PeerSessionInfos _peerInfos;
};


class NodeMonitor
{
public:
    NodeMonitor(bytes _networkrlp, std::string _ip);
    ~NodeMonitor(){}
    void run();
    
    bool checkIP();
    void getKeypair();
    void getClientVersion();
    void setData(monitorData _data);
    void exitThread(bool _flags) {m_mutex.lock(); m_threadExit = true; m_mutex.unlock();}
    Signature signatureData();
    std::string getNodeStatsStr(Signature _sign);

private:
    void analysisRet(std::string _ret);

private:
    bool m_threadExit = false;
    bool m_ipStats = false;
    //jsonrpc::HttpClient m_httpClient;
    std::string m_ip;
    uint32_t m_maxDelay;
    uint32_t m_minimumDelay;
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