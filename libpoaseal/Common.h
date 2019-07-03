#pragma once
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libbrccore/Exceptions.h>
#include <libp2p/Common.h>
#include <libdevcore/FixedHash.h>
#include <time.h>

namespace dev
{
	namespace brc
	{
	    using NodeID = p2p::NodeID;
		enum PoapPacketType : byte
		{
			PoaStatusPacket = 0x15, //在已经存在的消息上增加偏移量
            PoaRequestValitor ,
			PoaValitorData ,

            PoaPacketCount
		};
		enum ValitorWay
		{
			VATITOR_ADD = 0,     
			VATITOR_DEL = 1,
			VATITOR_FULL = 2,

			VATITOR_MAX
		};


		struct PoaMsgPacket
		{
			h512        node_id;     // peer id
			unsigned    packet_id;   //msg id
			bytes       data;        //rlp data

			PoaMsgPacket() :node_id(h512(0)), packet_id(0) {}
            PoaMsgPacket(h512 _id, unsigned _pid, bytesConstRef _data)
				:node_id(_id), packet_id(_pid), data(_data.toBytes()) {}
		};
        using PoaMsgQueue = dev::concurrent_queue<PoaMsgPacket>;

		struct PoaMgBase
		{
			u256        total_valitor_size;
			int64_t     last_time;
			PoaMgBase():total_valitor_size(0), last_time(0)
			{
			}
			virtual void streamRLPFields(RLPStream& _s) const
			{
				_s << total_valitor_size << last_time;
			}
			virtual void populate(RLP const& _rlp)
			{
				int field = 0;
				try
				{
					total_valitor_size = _rlp[field=0].toInt<u256>();
					field++;
					last_time = _rlp[field=1].toInt<int64_t>();
				}
				catch(Exception const& _e)
				{
					_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
					throw;
				}
			}
		};

		struct PoaMsg :public PoaMgBase
		{
			u256        curr_valitor_index;
			ValitorWay  v_type;
			//std::string address;
			//int64_t     change_time;
			std::vector<Address> address;
			PoaMsg() :curr_valitor_index(0), v_type(VATITOR_MAX)
			{
				address.clear();
			}
			void streamRLPFields(RLPStream& _s) const override
			{
				PoaMgBase::streamRLPFields(_s);
				unsigned tem_type = v_type;
				_s << curr_valitor_index << tem_type << address;
			}

			void populate(RLP const& _rlp) override
			{
				int field = 0;
			    PoaMgBase::populate(_rlp);
                try
                {
					curr_valitor_index = _rlp[field=2].toInt<u256>();
					v_type =ValitorWay(_rlp[field=3].toInt<unsigned>());
					address = _rlp[field = 4].toVector<Address> ();
                }
				catch(Exception const& _e)
				{
					_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
					throw;
                }
		    }
			void printLog()
			{
				std::cout << "PoaMsg Data: " << total_valitor_size << " |" << last_time << " |" << curr_valitor_index << " |"<< v_type;
				for(auto val : address)
					std::cout <<" || "<< val;
				std::cout << " :endl" << std::endl;
			}
		};
		
		struct DelPoaValitor
		{
			Address     m_del_address;
			Address     m_next_address;

			DelPoaValitor() :m_del_address(Address()), m_next_address(Address()){}
			bool operator == (Address const& other)
			{
				if(m_del_address == other)
					return true;
				return false;
			}
		};
	}
}
