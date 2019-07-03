#sh




#   $1  source directory
#   $2  libcryptopp.a cp directory
#
#
cd $1
#make disclean
#make clean
make
if [[ ! -f "libcryptopp.a" ]]; then
	echo "build crptycpp error."
else
    ehco "copy libcryptopp.a to $2"
	cp "libcryptopp.a"  $2
fi
