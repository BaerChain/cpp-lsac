#pragma once
#include <libdevcore/concurrent_queue.h>
#include <libdevcore/RLP.h>
#include <libdevcrypto/Common.h>
#include <libethcore/Exceptions.h>
#include <libp2p/Common.h>
#include <libdevcore/FixedHash.h>
#include <time.h>

namespace dev
{
	namespace eth
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
			VATITOR_FULL = 2
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
			int64_t     change_time;
			std::vector<Address> address;

			void streamRLPFields(RLPStream& _s) const override
			{
				PoaMsg::streamRLPFields(_s);
				unsigned tem_type = v_type;
				_s << curr_valitor_index << tem_type <<change_time << address;
			}

			void populate(RLP const& _rlp) override
			{
				int field = 0;
                try
                {
					curr_valitor_index = _rlp[field=2].toInt<u256>();
					v_type =ValitorWay(_rlp[field=3].toInt<unsigned>());
					change_time = _rlp[field = 4].toInt<int64_t>();
					address = _rlp[field = 5].toVector<Address> ();
                }
				catch(Exception const& _e)
				{
					_e << errinfo_name("invalid msg format") << BadFieldError(field, toHex(_rlp[field].data().toBytes()));
					throw;
                }
		    }
		};
		////直接调用这个函数，返回值最好是int64_t，long long应该也可以 
		//int64_t get_current_time()     
		//{
		//	time_t _t =time(NULL);
		//	return _t;
		//}
	}
}
