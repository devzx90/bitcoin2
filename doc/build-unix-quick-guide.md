UNIX Quick Build Guide
====================
This quick guide is meant for when you just want to build for the OS that you're currently using. See build-unix.md, build-windows.md or build-osx.md for more detailed instructions.

Compiling using Debian 10 is recommended if you're using a VM. The binaries will work on Ubuntu Bionic, then too but there is a known linking issue when trying to compile with GUI using Ubuntu Bionic.

`sudo apt-get update`
`sudo apt-get upgrade`

`sudo apt-get install git build-essential libtool autotools-dev automake pkg-config bsdmainutils python3`

<!-- These are required by depends:-->

`sudo apt-get install make automake cmake curl g++-multilib libtool binutils-gold bsdmainutils pkg-config python3 patch`

<!--For QT-->

sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler



`git clone https://github.com/BITC2/bitcoin2.git`

`cd depends`
`sudo chmod +x config.guess`
`sudo chmod +x config.site.in`
`sudo chmod +x config.sub`
`make`

`cd ..`

<!-- Replace "x86_64-pc-linux-gnu" with the directory's name that you created when you built the depends.-->

`./autogen.sh`

`CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure --disable-tests --enable-upnp-default --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests LDFLAGS=-static-libstdc++ CFLAGS="-O2" CXXFLAGS="-O2"`

`make`

# Create a release package:
`mkdir bitcoin2-bin`
`cp src/bitcoin2-cli src/bitcoin2d src/qt/bitcoin2-qt bitcoin2-bin`
`tar --owner=0 --group=0 -czf bitcoin2-x86_64-linux-gnu.tar.gz bitcoin2-bin`