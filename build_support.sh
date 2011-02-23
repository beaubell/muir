
### GLOBAL VARIABLES
DIR_WORKING=$SCRATCH
DIR_ARCHIVE=$ARCHIVE
DIR_LIBRARIES=$DIR_ARCHIVE/libraries
DIR_TOOLS=$DIR_ARCHIVE/tools
DIR_TARBALLS=$DIR_ARCHIVE/tarballs

PATH=$DIR_TOOLS/bin:$PATH
LFS_TGT=$(uname -m)-lfs-linux-gnu

CFLAGS="-march=opteron -g0 -O2"
CXXFLAGS=$CFLAGS
MAKEFLAGS="-j2"

BUILD_LOG=$DIR_WORKING/build.log


### PACKAGE VARIABLES
BINUTILS_VER=2.21
BINUTILS_FILE=binutils-$BINUTILS_VER.tar.bz2
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils/$BINUTILS_FILE

GCC_VER=4.5.2
GCC_FILE=gcc-$GCC_VER.tar.bz2
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/$GCC_FILE

MPFR_VER=3.0.0
MPFR_FILE=mpfr-$MPFR_VER.tar.bz2
MPFR_URL=http://ftp.gnu.org/gnu/mpfr/$MPFR_FILE

GMP_VER=5.0.1
GMP_FILE=gmp-$GMP_VER.tar.bz2
GMP_URL=http://ftp.gnu.org/gnu/gmp/$GMP_FILE

MPC_VER=0.8.2
MPC_FILE=mpc-$MPC_VER.tar.gz
MPC_URL=http://www.multiprecision.org/mpc/download/$MPC_FILE

GLIBC_VER=2.12.2
GLIBC_FILE=glibc-$GLIBC_VER.tar.bz2
GLIBC_URL=http://ftp.gnu.org/gnu/glibc/$GLIBC_FILE

BOOST_VER=1_46_0
BOOST_FILE=boost_$BOOST_VER.tar.bz2
BOOST_URL=http://sourceforge.net/projects/boost/files/boost/1.46.0/$BOOST_FILE/download

HDF5_VER=1.8.6
HDF5_FILE=hdf5-$HDF5_VER.tar.bz2
HDF5_URL=http://www.hdfgroup.org/ftp/HDF5/current/src/$HDF5_FILE

FFTW_VER=3.2.2
FFTW_FILE=fftw-$FFTW_VER.tar.gz
FFTW_URL=http://www.fftw.org/$FFTW_FILE



### SETUP
# Exit on error
set -e

echo "Build log is located at: $BUILD_LOG"

# Check existence of source dir.
if [ -d $DIR_TARBALLS ]; then
	echo "Setup: Source directory exists"
else
	echo "Setup: Creating source directory..."
	mkdir $DIR_TARBALLS
fi

# Check existence of working dir.
if [ -d $DIR_WORKING ]; then
	echo "Setup: Working directory exists"
else 
	echo "Setup: Creating working directory..."
	mkdir $DIR_WORKING
fi 

cd $DIR_WORKING



### PACAKGE BUILD FUNCTIONS
### This follows some from LFS eBook (https://www.linuxfromscratch.org)
build_binutils_pass1()
{
## Binutils - Pass 1
cd $DIR_WORKING
tar -xf $DIR_TARBALLS/$BINUTILS_FILE
mkdir -v binutils-build
cd binutils-build
../binutils-$BINUTILS_VER/configure \
    --prefix=$DIR_TOOLS \
    --disable-nls --disable-werror
make
make install
cd ..
rm -rf binutils-build
rm -rf binutils-$BINUTILS_VER
}

build_gcc_pass1()
{
## GCC - Pass 1
cd $DIR_WORKING
tar -xf $DIR_TARBALLS/$GCC_FILE
cd gcc-$GCC_VER

tar -xf $DIR_TARBALLS/$MPFR_FILE
mv mpfr-$MPFR_VER mpfr
tar -xf $DIR_TARBALLS/$GMP_FILE
mv gmp-$GMP_VER gmp
tar -xf $DIR_TARBALLS/$MPC_FILE
mv mpc-$MPC_VER mpc

mkdir ../gcc-build
cd ../gcc-build

../gcc-$GCC_VER/configure \
    --prefix=$DIR_TOOLS \
    --disable-nls --disable-shared --disable-multilib \
    --disable-decimal-float --disable-threads \
    --disable-libmudflap --disable-libssp \
    --disable-libgomp --enable-languages=c \
    --with-gmp-include=$(pwd)/gmp --with-gmp-lib=$(pwd)/gmp/.libs \
    --without-ppl --without-cloog

make
make install
cd ..
rm -rf gcc-$GCC_VER
rm -rf gcc-build
}


build_glibc()
{
# GLIBC
echo "### GLIBC"
cd $DIR_WORKING
echo "Decompressing glibc"
tar -xf $DIR_TARBALLS/$GLIBC_FILE
cd glibc-$GLIBC_VER
echo "Applying patch"
patch -Np1 -i $DIR_TARBALLS/glibc-2.12.2-gcc_fix-1.patch
mkdir -v ../glibc-build
cd ../glibc-build
echo "Configuring.."
../glibc-$GLIBC_VER/configure --prefix=$DIR_TOOLS \
    --build=$(../glibc-$GLIBC_VER/scripts/config.guess) \
    --disable-profile --enable-add-ons \
    #--enable-kernel=2.6.22.5 --with-headers=/tools/include \
    libc_cv_forced_unwind=yes libc_cv_c_cleanup=yes
echo "Making"
make
echo "Installing"
make install
exit
}


## GCC - Pass #2
build_gcc_pass2()
{
echo "Building GCC - Pass #2"
cd $DIR_WORKING
echo "GCC: Decompressing main source tree..."
tar -xf $DIR_TARBALLS/$GCC_FILE
cd gcc-$GCC_VER

echo "GCC: Decomressing MPFR..."
tar -xf $DIR_TARBALLS/$MPFR_FILE
mv mpfr-$MPFR_VER mpfr
echo "GCC: Decompressing GMP..."
tar -xf $DIR_TARBALLS/$GMP_FILE
mv gmp-$GMP_VER gmp
echo "GCC: Decompressing MPC..."
tar -xf $DIR_TARBALLS/$MPC_FILE
mv mpc-$MPC_VER mpc

mkdir ../gcc-build
cd ../gcc-build

echo "GCC: Configuring..."
#CC="$LFS_TGT-gcc -B/tools/lib/" \
    #AR=$LFS_TGT-ar RANLIB=$LFS_TGT-ranlib \
    ../gcc-$GCC_VER/configure --prefix=$DIR_TOOLS \
    --with-local-prefix=$DIR_TOOLS --enable-clocale=gnu \
    --enable-shared --enable-threads=posix \
    --enable-__cxa_atexit --enable-languages=c,c++,fortran,objc \
    --disable-libstdcxx-pch --disable-multilib \
    --with-gmp-include=$(pwd)/gmp --with-gmp-lib=$(pwd)/gmp/.libs \
    --without-ppl --without-cloog >> $BUILD_LOG

echo "GCC: Building..."
make $MAKEFLAGS >> $BUILD_LOG

echo "GCC: Installing..."
make install >> $BUILD_LOG

echo "GCC: Cleaning up..."
cd ..
rm -rf gcc-build
rm -rf gcc-$GCC_VER
}


## Boost
build_boost()
{

echo "Boost: Decompressing..."
cd $DIR_WORKING
tar -xf $DIR_TARBALLS/$BOOST_FILE
cd boost_$BOOST_VER

echo "Boost: Configuring..."
./bootstrap.sh --prefix=$DIR_LIBRARIES/boost-$BOOST_VER >> $BUILD_LOG

echo "Boost: Making and Installing..."
./bjam install >> $BUILD_LOG

echo "Boost: Cleaning up..."
cd ..
rm -rf boost_$BOOST_VER
}


## HDF5
build_hdf5()
{
echo "HDF5: Decompressing..."
cd $DIR_WORKING
tar -xf $DIR_TARBALLS/$HDF5_FILE
cd hdf5-$HDF5_VER

echo "HDF5: Configuring..."
./configure --prefix=$DIR_LIBRARIES/hdf5-$HDF5_VER --enable-cxx --enable-production >> $BUILD_LOG
# --enable-threadsafe  not compatable with --enable-cxx

echo "HDF5: Building..."
make $MAKEFLAGS >> $BUILD_LOG

echo "HDF5: Installing..."
make install >> $BUILD_LOG

echo "HDF5: Cleaning up..."
cd ..
rm -rf hdf5-$HDF5_VER
}


## FFTW Double and Single Precision Float
build_fftw()
{
echo "FFTW: Decompressing..."
cd $DIR_WORKING
tar -xf $DIR_TARBALLS/$FFTW_FILE
cd fftw-$FFTW_VER

echo "FFTW: Configuring for double precision..."
./configure --prefix=$DIR_LIBRARIES/fftw-$FFTW_VER --enable-threads --enable-sse2 --enable-shared --enable-static >> $BUILD_LOG

echo "FFTW: Making double precision..."
make $MAKEFLAGS >> $BUILD_LOG

echo "FFTW: Installing double precision..."
make install >> $BUILD_LOG

echo "FFTW: Cleaning up for rebuild..."
make clean >> $BUILD_LOG

echo "FFTW: Configuring for single precision..."
./configure --prefix=$DIR_LIBRARIES/fftw-$FFTW_VER --enable-threads --enable-single --enable-sse --enable-shared --enable-static >> $BUILD_LOG

echo "FFTW: Making single precision..."
make $MAKEFLAGS >> $BUILD_LOG

echo "FFTW: Installing single precision..."
make install >> $BUILD_LOG

echo "FFTW: Cleaning up..."
cd ..
rm -rf fftw-$FFTW_VER
}


## Fetch
fetch_files()
{
echo "Downloading and verifying files..."
cd $DIR_TARBALLS
#wget -c $BINUTILS_URL
wget -c $GCC_URL
wget -c $MPFR_URL
wget -c $GMP_URL
wget -c $MPC_URL
#wget -c $GLIBC_URL
wget -c $BOOST_URL
wget -c $HDF5_URL
wget -c $FFTW_URL
}


### Build Order
fetch_files
build_gcc_pass2
build_boost
build_hdf5
build_fftw

