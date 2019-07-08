#pragma once

#include "Common.h"
#include <libbrccore/SealEngine.h>
#include <libbrccore/Common.h>
#include <libp2p/Common.h>
#include <libdevcore/Worker.h>
#include "SHDposClient.h"
#include "SHDposHostCapability.h"
#include <libdevcore/LevelDB.h>
#include <libdevcore/DBFactory.h>
#include <libdevcore/dbfwd.h>
#include <libdevcore/db.h>
#include <libdevcore/Address.h>
#include <boost/filesystem/path.hpp>


namespace dev
{
    namespace bacd
    {
        using namespace dev ::brc;
		const size_t timesc_20y = 60 * 60 * 24 * 365 * 20;
		class SHDpos: public SealEngineBase , Worker
        {
        public:
            SHDpos();
            ~SHDpos();
            static std::string  name(){ return "SHDpos"; }
            static void         init();
            unsigned            ver(){return 1001;};
            unsigned            revision() const override { return 1; }
            unsigned            sealFields() const override { return 2; }
            strings             sealers() const override { return { "cpu" }; };
            void                generateSeal(BlockHeader const& _bi) override;
			void                populateFromParent(BlockHeader& _bi, BlockHeader const& _parent) const override;
			virtual  void       verify(Strictness _s, BlockHeader const& _bi, BlockHeader const& _parent = BlockHeader(), bytesConstRef _block = bytesConstRef()) const override;
			void                initConfigAndGenesis(ChainParams const& m_params);
            void                setDposClient(SHDposClient const* _c) { m_dpos_cleint = _c; }
            SHDposConfigParams const& dposConfig() { return m_config; }

            bool                isBolckSeal(uint64_t _now);
            bool                checkDeadline(uint64_t _now);           //check the create block time_cycle
			void                tryElect(uint64_t _now);   //判断是否完成了本轮出块，选出新一轮验证人
			inline std::vector<Address> const& getCurrCreaters() const { return m_curr_varlitors; }


			inline void         initNet(std::weak_ptr<SHDposHostcapality> _host) { m_host = _host; }
            inline void         startGeneration() { setName("SHDpos"); startWorking(); }   //loop 开启 
            
			inline int64_t      get_next_time() const{ return m_next_block_time; }

		public:
			void                onDposMsg(NodeID _nodeid, unsigned _id, RLP const& _r);
			void                requestStatus(NodeID const & _nodeID, u256 const & _peerCapabilityVersion);
		protected:
			void                workLoop() override; 
		private:
			void                sealAndSend(NodeID const& _nodeid, SHDposPacketType _type, RLPStream const& _msg_s);   // send msg to nodeId
			void                brocastMsg(SHDposPacketType _type, RLPStream& _msg_s);       //brocastMsg about SH-Dpos           
            
			void				candidateReplaceVarlitors(size_t& _replaceNum);  //候选人替换惩罚的验证人出块
            void				addBlackList(std::vector<Address>& _v);  //超过12个坏块，加入黑名单，踢出出块人出块资格
            bool				isCandidateBlock(Address _addr);  //判断候选人本轮是否能出块
            bool				chooseBlockAddr(Address const& _addr, bool _isok);
            void				punishBlockVarlitors(Address const& _addr);
            bool				punishBlockCandidate(Address& _addr);
            void				addBadBlockInfo(Address _addr, size_t _badBlockNums);
            void 				getVarlitorsAndCandidate(std::vector<Address>& _curr_varlitors);
            bool				isCurrBlock(Address const& _curr);
			void				addCandidatePunishBlock();
        private:
            bool                CheckValidator(uint64_t _now);                  //验证是否该当前节点出块
            size_t              kickoutVarlitors();      //踢出不合格的候选人, 自定义踢出候选人规则，踢出后也不能成为验证人
            void                countVotes();
            void                disorganizeVotes();
		public:
			void                dellImportBadBlock(bytes const& _block);
		private:
			SignatureStruct     signBadBlock(const Secret &sec, bytes const& _badBlock);
			bool                verifySignBadBlock( bytes const& _badBlock, SignatureStruct const& _signData);
			void                addBadBlockLocal(bytes const& _b, Address const& _verifyAddr);
			void                sendBadBlockToNet(bytes const& _block);
		private:
            enum BadBlockType : unsigned
			{
			    BadBlockPushs =1,
                BadBlockDatas,
                badBlockAll,
			};
			inline std::string  getBadBlockData(db::Slice _key)const { return m_badblock_db->lookup(_key); }
			inline void         insertBadBlock(db::Slice _key, std::string _value)const { m_badblock_db->insert(_key, db::Slice(_value)); }
			void                openBadBlockDB(boost::filesystem::path const& _dbPath);
			void                initBadBlockByDB();
			db::Slice           toSlice(Address const& _h, unsigned _sub );
			bool                load_key_value(db::Slice _key, db::Slice _val);
			void                streamBadBlockRLP(RLPStream& _s, Address _addr, BadBlockType _type);
			void                populateBadBlock(RLP const& _r, Address _addr, BadBlockType _type);

			void                updateBadBlockData();
			void                insertUpdataSet(Address const& _addr, BadBlockType _type);

		private:
			std::set<Address>				m_curr_punishVandidate;				//当前需要惩罚的候选人
			std::vector<Address>            m_curr_varlitors;                   //本轮验证人集合
			std::vector<Address>            m_curr_candidate;								//本轮候选人集合
            SHDposClient const*             m_dpos_cleint;
            int64_t                         m_next_block_time;                  // next 进入出块周期时间
			int64_t                         m_last_block_time;
            SHDposConfigParams              m_config;                           // dpos 相关配置

			size_t m_notBlockNum;                   //下一轮不能出块的人数
            std::map<Address, BadBlockPunish> m_blackList;						 //超过12个坏块，加入黑名单         
            std::map<Address, BadBlockPunish> m_punishVarlitor;					// 记录出坏块的地址及处罚信息
            BadBlockNumPunish m_badPunish;		
			std::map<Address, BadBlocksData>  m_badVarlitors;                   // SH-Dpos the varify field blockes for varlitor

			mutable  Mutex                    m_mutex;                            // lock for msg_data
			std::weak_ptr<SHDposHostcapality> m_host;                             // SH-Dpos network
			SHDposMsgQueue                    m_msg_queue;                        // msg-data

			std::unique_ptr<db::DatabaseFace> m_badblock_db;                    // SHDpos badBlock  
			std::map<Address, BadBlockType>   m_up_set;                         // update cach data


            Logger m_logger{createLogger(VerbosityDebug, "SH-Dpos")};
            Logger m_warnlog{ createLogger(VerbosityWarning, "SH-Dpos") };
        };

        
    }
}
