#pragma once

#include <memory>
#include <libdevcore/db.h>
#include <libdevcore/Common.h>
#include <libdevcore/Log.h>
#include <libdevcore/StateCacheDB.h>

namespace dev
{

class OverlayDB: public StateCacheDB
{
public:
    explicit OverlayDB(std::unique_ptr<db::DatabaseFace> _db = nullptr)
      : m_db(_db.release(), [](db::DatabaseFace* db) {
//            clog(VerbosityDebug, "overlaydb") << "Closing state DB";
            delete db;
        })
    {
//    	cwarn << "create OverlayDB ......";
    }

    ~OverlayDB();

    // Copyable
    OverlayDB(OverlayDB const&) = default;
    OverlayDB& operator=(OverlayDB const&) = default;
    // Movable
    OverlayDB(OverlayDB&&) = default;
    OverlayDB& operator=(OverlayDB&&) = default;

    void commit();
	void rollback();

	std::string lookup(h256 const& _h) const;
	bool exists(h256 const& _h) const;
	void kill(h256 const& _h);

	bytes lookupAux(h256 const& _h) const;


private:
	using StateCacheDB::clear;
	std::shared_ptr<db::DatabaseFace> m_db;

};

}
