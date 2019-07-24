



add_library(brc_db STATIC
            include/brc/database.hpp
            include/brc/types.hpp
            include/brc/objects.hpp
            include/brc/exchangeOrder.hpp
            include/brc/exception.hpp
            src/database.cpp
            src/exchangeOrder.cpp
        src/types.cpp
        )

target_link_libraries(brc_db  devcore  chainbase )
target_include_directories(brc_db PUBLIC
        ../chainbase/include
        include
        ${CMAKE_SOURCE_DIR}
        )