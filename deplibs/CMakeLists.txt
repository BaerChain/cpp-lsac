



include(ExternalProject)


set(dep_libraries_dir ${CMAKE_CURRENT_BINARY_DIR}/libs)


#ExternalProject_Add(dep_secp256k1
#        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1
#        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1/configure --prefix=${dep_libraries_dir} --enable-module-ecdh --enable-experimental --enable-coverage --enable-module-recovery
#        BUILD_COMMAND make
#        INSTALL_DIR "${dep_libraries_dir}"
#        INSTALL_COMMAND make install
#        BUILD_BYPRODUCTS "${dep_libraries_dir}/secp256k1.a"
#        )
#
#add_library(secp256k1 STATIC IMPORTED)
##set_property(TARGET secp256k1 PROPERTY IMPORTED_LOCATION ${dep_libraries_dir}/libsecp256k1${CMAKE_STATIC_LIBRARY_SUFFIX})
##set_property(TARGET secp256k1 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1/include)
#add_dependencies(secp256k1 dep_secp256k1)


################################ mpir ###################################################
#set(MPIR_LIBRARY "${dep_libraries_dir}/mpir.a")
#set(MPIR_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mpir")
#ExternalProject_Add(dep_mpir
#        PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mpir
#        CMAKE_ARGS
#        -DCMAKE_INSTALL_PREFIX="${dep_libraries_dir}/lib"
#        -DCMAKE_INSTALL_LIBDIR=${dep_libraries_dir}/lib
#        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
#        -DMPIR_GMP=On
#        INSTALL_COMMAND make install
#        BUILD_BYPRODUCTS "${MPIR_LIBRARY}"
#        )
#
#add_library(mpir STATIC IMPORTED)
##set_property(TARGET mpir PROPERTY IMPORTED_LOCATION_RELEASE ${MPIR_LIBRARY})
##set_property(TARGET mpir PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MPIR_INCLUDE_DIR})
#add_dependencies(mpir dep_mpir)
################################ dep_ff ###################################################
#set(libff_inlcude_dir "${CMAKE_CURRENT_SOURCE_DIR}/libff/include")
#ExternalProject_Add(libff
#        PREFIX "${CMAKE_CURRENT_SOURCE_DIR}"
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libff
#        CMAKE_ARGS
#        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
#        -DCMAKE_INSTALL_PREFIX=${dep_libraries_dir}/
#        -DCMAKE_INSTALL_LIBDIR=${dep_libraries_dir}/lib
#        -DGMP_INCLUDE_DIR=${LIBSCRYPT_INCLUDE}
#        -DGMP_LIBRARY=${GMP_LIBRARIES}
#        -DCURVE=ALT_BN128 -DPERFORMANCE=Off -DWITH_PROCPS=Off
#        -DUSE_PT_COMPRESSION=Off
#        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
#        INSTALL_COMMAND make install
#        BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/libff/libffi.a"
#        )
#
#add_dependencies(libff mpir)
## Create snark imported library
#add_library(libff::ff STATIC IMPORTED)
#file(MAKE_DIRECTORY ${libff_inlcude_dir})
##set_property(TARGET libff::ff PROPERTY IMPORTED_CONFIGURATIONS Release)
##set_property(TARGET libff::ff PROPERTY IMPORTED_LOCATION_RELEASE ${libff_library})
##set_property(TARGET libff::ff PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${libff_inlcude_dir})
##set_property(TARGET libff::ff PROPERTY INTERFACE_LINK_LIBRARIES MPIR::mpir)
#add_dependencies(libff::ff libff)
#


################################ cryptopp ###################################################
#ExternalProject_Add(dep_cryptopp
#        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp
#        BUILD_IN_SOURCE 1
#        BUILD_ALWAYS 1
#        CONFIGURE_COMMAND ""
#        BUILD_COMMAND make
##        INSTALL_DIR "${dep_libraries_dir}"
#        INSTALL_COMMAND "make install DESTDIR=${dep_libraries_dir}"
#        BUILD_BYPRODUCTS "${dep_libraries_dir}/libcryptopp.a"
#        )
#add_library(cryptopp STATIC IMPORTED)




#ExternalProject_Add(dep_cryptopp
#        PREFIX "${CMAKE_CURRENT_SOURCE_DIR}"
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp
#        CMAKE_ARGS
#        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
#        -DCMAKE_INSTALL_PREFIX=${dep_libraries_dir}/
#        -DCMAKE_INSTALL_LIBDIR=${dep_libraries_dir}/lib
#        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
#        INSTALL_COMMAND make install
#        BUILD_BYPRODUCTS "${dep_libraries_dir}/lib/libcryptopp.a"
#        )
#add_library(cryptopp STATIC IMPORTED)
#add_dependencies(cryptopp dep_cryptopp)
add_subdirectory(secp256k1)
add_subdirectory(brchash)
add_subdirectory(mpir)
add_subdirectory(libff)
add_subdirectory(cryptopp)
