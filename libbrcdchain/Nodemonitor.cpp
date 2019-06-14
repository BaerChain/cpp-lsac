#include "Nodemonitor.h"
#include "libdevcore/RLP.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <thread>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonJS.h>
#include <json/json.h>


using namespace dev;
using namespace brc;

KeyPair networkAlias(bytes _b)
{
    RLP r(_b);
    if (r.itemCount() == 3 && r[0].isInt() && r[0].toInt<unsigned>() >= 3)
        return KeyPair(Secret(r[1].toBytes()));
    else
        return KeyPair::create();
}


NodeMonitor::NodeMonitor(bytes _networkrlp, std::string _ip):
    m_networkrlp(_networkrlp),
    m_ip(_ip),
    m_httpClient(jsonrpc::HttpClient(_ip))
{
    cnote << "iop:" << m_ip;
    getClientVersion();
    getKeypair();
}

void NodeMonitor::getKeypair()
{
    KeyPair _keypair = networkAlias(m_networkrlp);
    m_secret = _keypair.secret();
    m_public = _keypair.pub();
    m_addr = _keypair.address();
}


void NodeMonitor::getClientVersion()
{
    const auto *info = brcd_get_buildinfo();
    m_clientVersion = info->git_commit_hash;
}

void NodeMonitor::setData(monitorData _data)
{
    m_mutex.lock();
    if(m_ip.empty())
    {
        m_mutex.unlock();
        return;
    }
    m_data.push_back(_data);
    m_mutex.unlock();
}

Signature NodeMonitor::signatureData() const
{
    if(m_data.size() == 0)
    {
        return Signature();
    }
    monitorData _data = m_data.back();
    RLPStream _rlp(7);
    _rlp << m_public << _data.blocknum << _data.blockhash << m_clientVersion << _data.nodenum << _data.packagetranscations << _data.pendingpoolsnum;
    Signature _sign = sign(m_secret, sha3(_rlp.out()));
    return _sign;
}

void NodeMonitor::sendNodeStats(Signature _sign)
{
    if(m_data.size() == 0)
    {
        return;
    }
    monitorData _data = m_data.back();
    Json::Value _jv;
    _jv["nodeID"] = toJS(m_public);
    _jv["blockNum"] = _data.blocknum;
    _jv["blockHash"] = toJS(_data.blockhash);
    _jv["clientVersion"] = m_clientVersion;
    _jv["nodeNum"] = toJS(_data.nodenum);
    _jv["packageTranscations"] = toJS(_data.packagetranscations);
    _jv["pendingpoolsnum"] = toJS(_data.pendingpoolsnum);
    _jv["signatureData"] = toJS(_sign);

    std::string _ret;
    std::string _value = _jv.toStyledString();
    try
    {
        cnote << "_value:" << _value;
        m_httpClient.SendRPCMessage(_value, _ret);
    }
    catch(const std::exception& e)
    {
        m_ip.clear();
        return;
    }
    

    
    cnote << "_ret:" << _ret;

}

void NodeMonitor::run()
{
    while(1)
    {
        m_mutex.lock();
        if(m_data.size() > 0)
        {
            Signature _sign = signatureData();
            sendNodeStats(_sign);
            cnote << "signdata:" << _sign << "  threadid:" << std::this_thread::get_id();
            //TO DO:signature data
            m_data.clear();
        }

        if(m_threadExit)
        {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
    }
    return;
}