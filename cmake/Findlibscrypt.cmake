


find_library(LIBSCRYPT_LIBRARIES
        NAMES scrypt
        PATHS /usr/lib /usr/local/Cellar/libscrypt /usr/local/lib
        DOC "libscrypt library dir"
        )

find_path(LIBSCRYPT_INCLUDE
        NAMES libscrypt.h
        PATHS /usr/include /usr/local/Cellar/libscrypt/1.21/include /usr/local/include
        DOC "libscrypt include dir")
message(STATUS "Found scrypt lib: ${LIBSCRYPT_LIBRARIES} ${LIBSCRYPT_INCLUDE}")