


#include <brc/database.hpp>

namespace dev {
    namespace brc {
        namespace ex {
            database::database(const boost::filesystem::path &data_dir, database::open_flags write,
                               uint64_t shared_file_size, bool allow_dirty) : chainbase::database(data_dir, write,
                                                                                                  shared_file_size,
                                                                                                  allow_dirty) {
            }

            database::~database() {
                std::cout << __FUNCTION__ << "  " <<  __LINE__ << "  : close exdb complete.\n";
            }
        }
    }
}