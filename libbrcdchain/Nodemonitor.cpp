#include "Nodemonitor.h"
#include "libdevcore/RLP.h"
#include <libdevcore/Log.h>
#include <libdevcore/SHA3.h>
#include <libdevcore/Common.h>
#include <libdevcrypto/base58.h>
#include <thread>
#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/CommonJS.h>
#include <json/json.h>
#include <jsonrpccpp/common/exception.h>
#include <jsonrpccpp/common/errors.h>
#include <time.h>

using namespace dev;
using namespace brc;
using namespace std;
using namespace jsonrpc;

vector<string> split(const string &s, const string &seperator){
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while(i != s.size()){
        int flag = 0;
        while(i != s.size() && flag == 0){
            flag = 1;
            for(string_size x = 0; x < seperator.size(); ++x)
            {
                if(s[i] == seperator[x]){
                    ++i;
                    flag = 0;
                    break;
                    }
            }
        }


        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0){
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[j] == seperator[x]){
                    flag = 1;
                    break;
                    }
            if(flag == 0)
                ++j;
        }
        if(i != j){
            result.push_back(s.substr(i, j-i));
            i = j;
        }
    }
    return result;
}




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
    m_ip(_ip)
{
    cnote << "ip:" << m_ip;
    //m_ipStats = checkIP();
    getClientVersion();
    getKeypair();
    if(!_ip.empty())
    {
        std::thread t(&NodeMonitor::run, this);
        t.detach();
    }
}

bool NodeMonitor::checkIP()
{
    vector<string> _v = split(m_ip, ".:");
    if(_v.size() == 5)
    {
        jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(m_ip);
        try
        {
            std::string _post = "hello";
            std::string _ret;
           _httpClient.SendRPCMessage(_post, _ret);
           if(_ret.empty())
           {
                cerror << "The specified node server address is not a valid address";
               return false;
           }
        }
        catch(...)
        {
            cerror << "The specified node server address is not a valid address";
            return false;
        }
        return true;
    }
    return false;
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
    // if(m_ipStats == false)
    // {
    //     m_mutex.unlock();
    //     return;
    // }
    m_data.push_back(_data);
    m_mutex.unlock();
}

Signature NodeMonitor::signatureData(monitorData const& _data)
{
//    monitorData _data = m_data.back();
//    cnote << "nodeNum:" <<_data._peerInfos.size();
    m_maxDelay = 0;
    m_minimumDelay = 0;
    for(auto i : _data._peerInfos)
    {
        uint32_t _delay = chrono::duration_cast<chrono::milliseconds>(i.lastPing).count() / 2;

        if(_delay < m_minimumDelay)
        {
            m_minimumDelay = _delay;
        }

        if(_delay > m_maxDelay)
        {
            m_maxDelay = _delay;
        }
//        cnote << "ping :" << chrono::duration_cast<chrono::milliseconds>(i.lastPing).count() / 2;
    }

    RLPStream _rlp(13);
    _rlp << m_public.hex() << std::to_string(_data.blocknum) << _data.blockAuthor.hex() <<_data.blockhash.hex() <<  toJS(_data.blockgasused) << toJS(_data.blockDelay) << toJS(_data.time) <<
    m_clientVersion << toJS(_data.nodenum) << toJS(m_maxDelay) << toJS(m_minimumDelay) << toJS(_data.packagetranscations) << toJS(_data.pendingpoolsnum);
    Signature _sign = sign(m_secret, sha3(_rlp.out()));

    return _sign;
}

std::string NodeMonitor::getNodeStatsStr(monitorData const& _data, Signature _sign)
{
    clock_t time;
    Json::Value _jv;
    _jv["nodeID"] = m_public.hex();
    _jv["blockNum"] = std::to_string(_data.blocknum);
    _jv["blockAuthor"] = _data.blockAuthor.hex();
    _jv["blockHash"] = _data.blockhash.hex();
    _jv["blockgasUsed"] = toJS(_data.blockgasused);
    _jv["serverDelay"] = toJS(_data.time);
    _jv["blockDelay"] = toJS(_data.blockDelay);
    _jv["clientVersion"] = m_clientVersion;
    _jv["nodeNum"] = toJS(_data.nodenum);
    _jv["nodeMaxDelay"] = toJS(m_maxDelay);
    _jv["nodeMinimumDelay"] = toJS(m_minimumDelay);
    _jv["packageTranscations"] = toJS(_data.packagetranscations);
    _jv["pendingpoolsnum"] = toJS(_data.pendingpoolsnum);
    _jv["signatureData"] = toJS(_sign);

    std::string _value = _jv.toStyledString();
    return _value;
}

void NodeMonitor::analysisRet(std::string _ret)
{
    Json::Reader _reader;
    Json::Value _value;
    if(_reader.parse(_ret, _value))
    {
        int _code = _value["code"].asInt();
        if(_code == 500)
        {
            cerror << "sendNodeStats error :The push health state node does not belong to a valid push node";
        }
    }
}

void NodeMonitor::run()
{
    jsonrpc::HttpClient _httpClient = jsonrpc::HttpClient(m_ip);
    //_httpClient.SetTimeout();
    int errorNum = 0;
    int inputNum = 0;
    while(1)
    {
        m_mutex.lock();
        if(m_data.size() ==  0)
        {
            m_mutex.unlock();
            usleep(50);
            continue;
        }
        monitorData _data = m_data.back();
        m_data.clear();
        m_mutex.unlock();

//        if(m_data.size() > 0)
//        {
            Signature _sign = signatureData(_data);
            std::string _str = getNodeStatsStr(_data, _sign);
            if(!_str.empty())
            {
                try
                {
                    std::string _ret;
                    _httpClient.SendRPCMessage(_str, _ret);
                    cnote << "http ret:" << _ret;
                    analysisRet(_ret);
                    errorNum = 0;
                    inputNum = 0;
                }
                catch(JsonRpcException &e)
                {
                    errorNum++;
                    if(errorNum > 10)
                    {
                        inputNum++;
                        if(inputNum == 10)
                        {
                            cerror << e.GetMessage();
                            inputNum = 0;
                        }
                    }else{
                        cerror << e.GetMessage();
                    }
                }
            }
            //TO DO:signature data
//            m_data.clear();
//        }

        if(m_threadExit)
        {
            cerror << "thread return";
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        usleep(100);
    }
    return;
}