#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
#KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "Attempting Deep Cleaning"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mproper 
    echo "Attemting to make .config file"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "Building Kernal image"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    #Reading may have said not to do this part
    echo "Building modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    #Device tree
    echo "File Tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"

if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
cd "$OUTDIR"
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
echo "Cloning Busy Box"
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Cleaning busybox"
    make distclean
    make defconfig
else
    cd busybox
    echo "busybox is already cloned"
fi

# TODO: Make and install busybox
echo "making busy box"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
echo "Making root fs"
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
#
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cd "${SYSROOT}"
echo "Sysroot: ${SYSROOT}"
echo "copying executables"

dep3=$(find . -name libm.so.6)
echo "dep3: ${dep3}"
dep4=$(find . -name libc.so.6)
echo "dep4: ${dep4}"

echo "hard code copying /lib/ld-linux-aarch64.so.1"
echo "hard code copying /lib64/libresolv-2.31.so"

#cd lib64
#dep1=$(find . -name ld-linux-aarch64.s0.1)
#echo "dep1: ${dep1}"
#dep2=$(find . -name libresolv-2.31.s)
#echo "dep2: ${dep2}"

echo "Performing copying"
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp ${dep3} ${OUTDIR}/rootfs/lib64/
cp ${dep4} ${OUTDIR}/rootfs/lib64/

echo "Copy was a success"

#cp $(find . -name ld-linux-aarch64.s0.1) ${OUTDIR}/rootfs/lib
#cp $(find . -name libm.so.6) ${OUTDIR}/rootfs/lib64
#cp $(find . -name libesolve.so.2) ${OUTDIR}/rootfs/lib64
#cp $(find . -name libc.so.6) ${OUTDIR}/rootfs/lib64

# TODO: Make device nodes
echo "Making device nodes"
cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1
echo "Making device nodes a success"

# TODO: Clean and build the writer utility
echo "Making writer utility"
cd /home/aaron/Desktop/assignment-3-part-2-aaronaprati/finder-app
make clean CROSS_COMPILE=${CROSS_COMPILE}
make CROSS_COMPILE=${CROSS_COMPILE}
echo "Made writer utility"
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying finder stuff"

mkdir -p ${OUTDIR}/rootfs
mkdir -p ${OUTDIR}/rootfs/home/conf

#echo "Copying finder-app/conf lets see if it works" 
#cp -R /home/aaron/Desktop/assignment-2-aaronaprati/finder-app/conf ${OUTDIR}/rootfs/home/conf
echo "Copying finder-app/conf lets see if it works" 

cp /home/aaron/Desktop/assignment-2-aaronaprati/finder-app/writer.sh ${OUTDIR}/rootfs/home

cp /home/aaron/Desktop/assignment-2-aaronaprati/finder-app/finder*.sh ${OUTDIR}/rootfs/home

cp /home/aaron/Desktop/assignment-2-aaronaprati/finder-app/conf/username.txt ${OUTDIR}/rootfs/home/conf

cp /home/aaron/Desktop/assignment-2-aaronaprati/finder-app/conf/assignment.txt ${OUTDIR}/rootfs/home/conf

cp /home/aaron/Desktop/assignment-3-part-2-aaronaprati/finder-app/autorun-qemu.sh ${OUTDIR}/rootfs/home
echo "Done copying finder stuff"
# TODO: Chown the root directory
#cd ${SYSROOT}/rootfs
sudo chown -R root:root ${SYSROOT}/rootfs

echo "Done with owner thing"

# TODO: Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
pwd
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
