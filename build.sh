#!/bin/sh 

SOURCE_URL="http://www.pjsip.org/release/2.2.1/pjproject-2.2.1.tar.bz2"
PROJECT_DIR="pjproject-2.2.1"
ARCHIVE="${PROJECT_DIR}.tar.bz2"

copy_libs () {
	DST=${1}

	if [ -d pjlib/lib-${DST}/ ]; then
		rm -rf pjlib/lib-${DST}/
	fi
	if [ ! -d pjlib/lib-${DST}/ ]; then
		mkdir pjlib/lib-${DST}/
	fi
	cp pjlib/lib/libpj-arm-apple-darwin9.a pjlib/lib-${DST}/libpj-arm-apple-darwin9.a

	if [ -d pjlib-util/lib-${DST}/ ]; then
		rm -rf pjlib-util/lib-${DST}/
	fi
	if [ ! -d pjlib-util/lib-${DST}/ ]; then
		mkdir pjlib-util/lib-${DST}/
	fi
	cp pjlib-util/lib/libpjlib-util-arm-apple-darwin9.a pjlib-util/lib-${DST}/libpjlib-util-arm-apple-darwin9.a

	if [ -d pjmedia/lib-${DST}/ ]; then
		rm -rf pjmedia/lib-${DST}/
	fi
	if [ ! -d pjmedia/lib-${DST}/ ]; then
		mkdir pjmedia/lib-${DST}/
	fi
	cp pjmedia/lib/libpjmedia-arm-apple-darwin9.a pjmedia/lib-${DST}/libpjmedia-arm-apple-darwin9.a 
	cp pjmedia/lib/libpjmedia-audiodev-arm-apple-darwin9.a pjmedia/lib-${DST}/libpjmedia-audiodev-arm-apple-darwin9.a
	cp pjmedia/lib/libpjmedia-codec-arm-apple-darwin9.a pjmedia/lib-${DST}/libpjmedia-codec-arm-apple-darwin9.a
	cp pjmedia/lib/libpjmedia-videodev-arm-apple-darwin9.a pjmedia/lib-${DST}/libpjmedia-videodev-arm-apple-darwin9.a
	cp pjmedia/lib/libpjsdp-arm-apple-darwin9.a pjmedia/lib-${DST}/libpjsdp-arm-apple-darwin9.a

	if [ -d pjnath/lib-${DST}/ ]; then
		rm -rf pjnath/lib-${DST}/
	fi
	if [ ! -d pjnath/lib-${DST}/ ]; then
		mkdir pjnath/lib-${DST}/
	fi
	cp pjnath/lib/libpjnath-arm-apple-darwin9.a pjnath/lib-${DST}/libpjnath-arm-apple-darwin9.a

	if [ -d pjsip/lib-${DST}/ ]; then
		rm -rf pjsip/lib-${DST}/
	fi
	if [ ! -d pjsip/lib-${DST}/ ]; then
		mkdir pjsip/lib-${DST}/
	fi
	cp pjsip/lib/libpjsip-arm-apple-darwin9.a pjsip/lib-${DST}/libpjsip-arm-apple-darwin9.a
	cp pjsip/lib/libpjsip-simple-arm-apple-darwin9.a pjsip/lib-${DST}/libpjsip-simple-arm-apple-darwin9.a
	cp pjsip/lib/libpjsip-ua-arm-apple-darwin9.a pjsip/lib-${DST}/libpjsip-ua-arm-apple-darwin9.a
	cp pjsip/lib/libpjsua-arm-apple-darwin9.a pjsip/lib-${DST}/libpjsua-arm-apple-darwin9.a 

	if [ -d third_party/lib-${DST}/ ]; then
		rm -rf third_party/lib-${DST}/
	fi
	if [ ! -d third_party/lib-${DST}/ ]; then
		mkdir third_party/lib-${DST}/
	fi
	cp third_party/lib/libg7221codec-arm-apple-darwin9.a third_party/lib-${DST}/libg7221codec-arm-apple-darwin9.a
	cp third_party/lib/libgsmcodec-arm-apple-darwin9.a third_party/lib-${DST}/libgsmcodec-arm-apple-darwin9.a
	cp third_party/lib/libilbccodec-arm-apple-darwin9.a third_party/lib-${DST}/libilbccodec-arm-apple-darwin9.a
	cp third_party/lib/libresample-arm-apple-darwin9.a third_party/lib-${DST}/libresample-arm-apple-darwin9.a
	cp third_party/lib/libspeex-arm-apple-darwin9.a third_party/lib-${DST}/libspeex-arm-apple-darwin9.a
	cp third_party/lib/libsrtp-arm-apple-darwin9.a third_party/lib-${DST}/libsrtp-arm-apple-darwin9.a
}

lipo_libs () {
cp ../silk-arm-ios/libSKP_SILK_SDK.a third_party/lib/libSKP_SILK_SDK.a
cp ../opus-ios/libopus.a third_party/lib/libopus.a

xcrun -sdk iphoneos lipo -arch i386   pjlib/lib-iPhoneSimulator/libpj-arm-apple-darwin9.a \
						 -arch armv7  pjlib/lib-armv7/libpj-arm-apple-darwin9.a \
						 -arch armv7s pjlib/lib-armv7s/libpj-arm-apple-darwin9.a \
						 -arch arm64  pjlib/lib-arm64/libpj-arm-apple-darwin9.a \
						-arch x86_64  pjlib/lib-iPhoneSimulator64/libpj-arm-apple-darwin9.a \
					  -create -output pjlib/lib/libpj-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjlib-util/lib-iPhoneSimulator/libpjlib-util-arm-apple-darwin9.a \
						 -arch armv7  pjlib-util/lib-armv7/libpjlib-util-arm-apple-darwin9.a \
						 -arch armv7s pjlib-util/lib-armv7s/libpjlib-util-arm-apple-darwin9.a \
						 -arch arm64  pjlib-util/lib-arm64/libpjlib-util-arm-apple-darwin9.a \
						-arch x86_64  pjlib-util/lib-iPhoneSimulator64/libpjlib-util-arm-apple-darwin9.a \
					  -create -output pjlib-util/lib/libpjlib-util-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjmedia/lib-iPhoneSimulator/libpjmedia-arm-apple-darwin9.a \
					 	 -arch armv7  pjmedia/lib-armv7/libpjmedia-arm-apple-darwin9.a \
					 	 -arch armv7s pjmedia/lib-armv7s/libpjmedia-arm-apple-darwin9.a \
						-arch arm64  pjmedia/lib-arm64/libpjmedia-arm-apple-darwin9.a \
						-arch x86_64  pjmedia/lib-iPhoneSimulator64/libpjmedia-arm-apple-darwin9.a \
					  -create -output pjmedia/lib/libpjmedia-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjmedia/lib-iPhoneSimulator/libpjmedia-audiodev-arm-apple-darwin9.a \
					 	 -arch armv7  pjmedia/lib-armv7/libpjmedia-audiodev-arm-apple-darwin9.a \
						 -arch armv7s pjmedia/lib-armv7s/libpjmedia-audiodev-arm-apple-darwin9.a \
						-arch arm64  pjmedia/lib-arm64/libpjmedia-audiodev-arm-apple-darwin9.a \
						-arch x86_64  pjmedia/lib-iPhoneSimulator64/libpjmedia-audiodev-arm-apple-darwin9.a \
				 	  -create -output pjmedia/lib/libpjmedia-audiodev-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjmedia/lib-iPhoneSimulator/libpjmedia-codec-arm-apple-darwin9.a \
					 	 -arch armv7  pjmedia/lib-armv7/libpjmedia-codec-arm-apple-darwin9.a \
						 -arch armv7s pjmedia/lib-armv7s/libpjmedia-codec-arm-apple-darwin9.a \
						-arch arm64  pjmedia/lib-arm64/libpjmedia-codec-arm-apple-darwin9.a \
						-arch x86_64  pjmedia/lib-iPhoneSimulator64/libpjmedia-codec-arm-apple-darwin9.a \
				 	  -create -output pjmedia/lib/libpjmedia-codec-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjmedia/lib-iPhoneSimulator/libpjmedia-videodev-arm-apple-darwin9.a \
					 	 -arch armv7  pjmedia/lib-armv7/libpjmedia-videodev-arm-apple-darwin9.a \
						 -arch armv7s pjmedia/lib-armv7s/libpjmedia-videodev-arm-apple-darwin9.a \
						-arch arm64 pjmedia/lib-arm64/libpjmedia-videodev-arm-apple-darwin9.a \
						-arch x86_64 pjmedia/lib-iPhoneSimulator64/libpjmedia-videodev-arm-apple-darwin9.a \
				 	  -create -output pjmedia/lib/libpjmedia-videodev-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjmedia/lib-iPhoneSimulator/libpjsdp-arm-apple-darwin9.a \
					 	 -arch armv7  pjmedia/lib-armv7/libpjsdp-arm-apple-darwin9.a \
						 -arch armv7s pjmedia/lib-armv7s/libpjsdp-arm-apple-darwin9.a \
						-arch arm64  pjmedia/lib-arm64/libpjsdp-arm-apple-darwin9.a \
						-arch x86_64  pjmedia/lib-iPhoneSimulator64/libpjsdp-arm-apple-darwin9.a \
				 	  -create -output pjmedia/lib/libpjsdp-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjnath/lib-iPhoneSimulator/libpjnath-arm-apple-darwin9.a \
					 	 -arch armv7  pjnath/lib-armv7/libpjnath-arm-apple-darwin9.a \
						 -arch armv7s pjnath/lib-armv7s/libpjnath-arm-apple-darwin9.a \
						-arch arm64  pjnath/lib-arm64/libpjnath-arm-apple-darwin9.a \
						-arch x86_64  pjnath/lib-iPhoneSimulator64/libpjnath-arm-apple-darwin9.a \
				 	  -create -output pjnath/lib/libpjnath-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjsip/lib-iPhoneSimulator/libpjsip-arm-apple-darwin9.a \
					 	 -arch armv7  pjsip/lib-armv7/libpjsip-arm-apple-darwin9.a \
						 -arch armv7s pjsip/lib-armv7s/libpjsip-arm-apple-darwin9.a \
						-arch arm64 pjsip/lib-arm64/libpjsip-arm-apple-darwin9.a \
						-arch x86_64 pjsip/lib-iPhoneSimulator64/libpjsip-arm-apple-darwin9.a \
				 	  -create -output pjsip/lib/libpjsip-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjsip/lib-iPhoneSimulator/libpjsip-simple-arm-apple-darwin9.a \
					 	 -arch armv7  pjsip/lib-armv7/libpjsip-simple-arm-apple-darwin9.a \
						 -arch armv7s pjsip/lib-armv7s/libpjsip-simple-arm-apple-darwin9.a \
						-arch arm64  pjsip/lib-arm64/libpjsip-simple-arm-apple-darwin9.a \
						-arch x86_64  pjsip/lib-iPhoneSimulator64/libpjsip-simple-arm-apple-darwin9.a \
				 	  -create -output pjsip/lib/libpjsip-simple-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjsip/lib-iPhoneSimulator/libpjsip-ua-arm-apple-darwin9.a \
					 	 -arch armv7  pjsip/lib-armv7/libpjsip-ua-arm-apple-darwin9.a \
						 -arch armv7s pjsip/lib-armv7s/libpjsip-ua-arm-apple-darwin9.a \
						-arch arm64  pjsip/lib-arm64/libpjsip-ua-arm-apple-darwin9.a \
						-arch x86_64  pjsip/lib-iPhoneSimulator64/libpjsip-ua-arm-apple-darwin9.a \
				 	  -create -output pjsip/lib/libpjsip-ua-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   pjsip/lib-iPhoneSimulator/libpjsua-arm-apple-darwin9.a \
					 	 -arch armv7  pjsip/lib-armv7/libpjsua-arm-apple-darwin9.a \
						 -arch armv7s pjsip/lib-armv7s/libpjsua-arm-apple-darwin9.a \
						-arch arm64  pjsip/lib-arm64/libpjsua-arm-apple-darwin9.a \
						-arch x86_64  pjsip/lib-iPhoneSimulator64/libpjsua-arm-apple-darwin9.a \
				 	  -create -output pjsip/lib/libpjsua-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libg7221codec-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libg7221codec-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libg7221codec-arm-apple-darwin9.a \
						-arch arm64  third_party/lib-arm64/libg7221codec-arm-apple-darwin9.a \
						-arch x86_64  third_party/lib-iPhoneSimulator64/libg7221codec-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libg7221codec-arm-apple-darwin9.a

xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libgsmcodec-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libgsmcodec-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libgsmcodec-arm-apple-darwin9.a \
						-arch arm64  third_party/lib-arm64/libgsmcodec-arm-apple-darwin9.a \
						-arch x86_64  third_party/lib-iPhoneSimulator64/libgsmcodec-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libgsmcodec-arm-apple-darwin9.a
xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libilbccodec-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libilbccodec-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libilbccodec-arm-apple-darwin9.a \
						-arch arm64  third_party/lib-arm64/libilbccodec-arm-apple-darwin9.a \
						-arch x86_64  third_party/lib-iPhoneSimulator64/libilbccodec-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libilbccodec-arm-apple-darwin9.a
xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libresample-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libresample-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libresample-arm-apple-darwin9.a \
						-arch arm64  third_party/lib-arm64/libresample-arm-apple-darwin9.a \
						-arch x86_64  third_party/lib-iPhoneSimulator64/libresample-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libresample-arm-apple-darwin9.a
xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libspeex-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libspeex-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libspeex-arm-apple-darwin9.a \
						-arch arm64 third_party/lib-arm64/libspeex-arm-apple-darwin9.a \
						-arch x86_64 third_party/lib-iPhoneSimulator64/libspeex-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libspeex-arm-apple-darwin9.a
xcrun -sdk iphoneos lipo -arch i386   third_party/lib-iPhoneSimulator/libsrtp-arm-apple-darwin9.a \
					 	 -arch armv7  third_party/lib-armv7/libsrtp-arm-apple-darwin9.a \
						 -arch armv7s third_party/lib-armv7s/libsrtp-arm-apple-darwin9.a \
						-arch arm64 third_party/lib-arm64/libsrtp-arm-apple-darwin9.a \
						-arch x86_64 third_party/lib-iPhoneSimulator64/libsrtp-arm-apple-darwin9.a \
				 	  -create -output third_party/lib/libsrtp-arm-apple-darwin9.a
}

echo "Creating config.h..."
echo "#define PJ_CONFIG_IPHONE 1
#define PJMEDIA_HAS_OPUS_CODEC 1 
#include <pj/config_site_sample.h>" > ${PROJECT_DIR}/pjlib/include/pj/config_site.h

cd ${PROJECT_DIR}

CFLAGS=""
configure="./configure-iphone --with-silk=${PWD}/../silk-arm-ios --enable-opus-codec"

echo "Building for armv7..."
make distclean > /dev/null 2>&1
ARCH="-arch armv7" \
$configure
make -j dep
make clean
make -j
copy_libs armv7

echo "Building for armv7s..."
make distclean > /dev/null
ARCH='-arch armv7s' \
$configure > /dev/null
make -j dep > /dev/null 
make clean > /dev/null
make -j > /dev/null 2>&1
copy_libs armv7s

echo "Building for arm64..."
make distclean > /dev/null
ARCH='-arch arm64' \
$configure > /dev/null
make -j dep > /dev/null
make clean > /dev/null
make -j > /dev/null 2>&1
copy_libs arm64

echo "Building for iPhoneSimulator..."
make distclean > /dev/null
DEVPATH=/Applications/XCode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/ \
ARCH="-arch i386" \
CFLAGS="-O2 -m32 -mios-simulator-version-min=5.0" LDFLAGS="-O2 -m32 -mios-simulator-version-min=5.0" \
$configure > /dev/null
make -j dep > /dev/null 
make clean > /dev/null
make -j > /dev/null 2>&1
copy_libs iPhoneSimulator

echo "Building for iPhoneSimulator x86_64..."
make distclean > /dev/null
DEVPATH=/Applications/XCode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/ \
ARCH="-arch x86_64" \
CFLAGS="-O2 -m32 -mios-simulator-version-min=5.0" LDFLAGS="-O2 -m32 -mios-simulator-version-min=5.0" \
$configure > /dev/null
make -j dep > /dev/null 
make clean > /dev/null
make -j > /dev/null 2>&1
copy_libs iPhoneSimulator64

echo "Making universal lib..."
make distclean > /dev/null
lipo_libs
