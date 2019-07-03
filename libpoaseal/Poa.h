#pragma once
/*
    Poa ....
*/
#include "Common.h"
#include "PoaHostCapability.h"
#include <libbrccore/SealEngine.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
namespace dev
{
namespace brc
{
class Poa : public SealEngineBase, Worker
{
public:
    Poa(){}
	virtual ~Poa(){ stopWorking();}

    static std::string  name(){ return "Poa"; }
    unsigned            revision() const override { return 1; }
    unsigned            sealFields() const override { return 2; }

    strings             sealers() const override { return {"cpu"}; };

    void                generateSeal(BlockHeader const& _bi) override;
    bool                shouldSeal(Interface*) override;
protected:
	void                workLoop() override;

public:
    static void         init();
	void                initEnv(std::weak_ptr<PoaHostCapability> _host);
	inline void         startGeneration()   {setName("Poa"); startWorking(); }   //loop 开启

	inline void         set_valitor_change_time( int64_t _time){ m_lastChange_valitor_time = _time; }
    inline void         cancelGeneration() override { stopWorking(); }
    bool                verifySeal(BlockHeader const& ) const { return true; }
    bool                quickVerifySeal(BlockHeader const& ) const { return true; }

public:
	bool                updateValitor(Address const& _address, bool _flag, int64_t _time);
	inline std::vector<Address>& getPoaValidatorAccounts() { return m_poaValidatorAccount; }
	void                initPoaValidatorAccounts(std::vector<Address> const& _addresses);

	bool                isvalidators(Address const& _our_address, Address const& _currBlock_address);
private:
	void                sendAllUpdateValitor(Address const& _address, bool _flag);
public:
	void                onPoaMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
	void                requestStatus(NodeID const& _nodeID, u256 const& _peerCapabilityVersion);

private:
	// 广播消息
	void                brocastMsg(PoapPacketType _type, RLPStream& _msg_s);
    //指定发送
	void                sealAndSend(NodeID const& _nodeid, PoapPacketType _type, RLPStream const& _msg_s);
	void                updateValitor(PoaMsg const& _poadata);

private:
	mutable Mutex                       m_mutex;
	std::vector<Address>                m_poaValidatorAccount;  //验证人集合
	int64_t                             m_lastChange_valitor_time; //上次更改验证人时间
    std::weak_ptr<PoaHostCapability>    m_host;
	PoaMsgQueue                         m_msg_queue;	// msg queue 消息队列
//	bool                                m_cfg_err;
	std::vector<DelPoaValitor>          m_del_poaValitors;       //被删除验证人集合，主要是验证轮流出块时 使用一次，使用后删除
};

}  // namespace brc
}  // namespace dev