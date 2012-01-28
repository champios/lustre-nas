#!/bin/bash
# Argument= -k kernel_version -f flavor -r downstream_release

Usage()
{
cat << EOF
Usage: $0 options

OPTIONS:
  -k    kernel version
          E.g. el6.1:      2.6.32-131.6.1.el6.20111108
	       sles11sp1:  2.6.32.23-0.3.1.20110428
  -f    flavor	
          E.g. nasa,lustre186,lustre210
  -r    downstream release string
          E.g. 2nasS_ofed153
EOF
exit 1
}


downstream_release=""

while getopts "k:f:r:" OPTION
do
    	case $OPTION in
	    k) echo "option -k"
		kversion_base="$OPTARG"
		;;
	    f) echo "option -f"
	 	flavor="$OPTARG"
		;;
	    r) echo "option -r"
		downstream_release="$OPTARG"
		;;
	    *) break;;
	esac
done
shift $(($OPTIND - 1))

arch=$(uname -m)
if [ -f /etc/SuSE-release ]; then
	distro=sles;
	flavor=nasa;	# Only nasa kernel (client) is supported in sles
else if [ -f /etc/redhat-release ]; then
	distro=rhel;
     fi
fi


[ -n "$kversion_base" ] || Usage
[ -n "$flavor" ] || flavor=nasa
[ -n "$downstream_release" ] || Usage

echo "distro=$distro, kversion_base=$kversion_base, flavor=$flavor, downstream=$downstream_release."

if [ "$distro" = "rhel" ]; then
    kversion=${kversion_base}.${arch}.${flavor}
    . /usr/share/Modules/init/bash
else if [ "$distro" = "sles" ]; then
    kversion=linux-${kversion_base}${flavor}
    . /usr/share/modules/init/bash
else
    Usage
fi
fi

module load /nasa/modulefiles/mpi-mvapich2/1.2p1/gcc

# run autogen.sh
bash ./autogen.sh

if [ $distro == "rhel" ]; then
  ./configure \
  --enable-ext4 \
  --disable-liblustre \
  --with-o2ib=/usr/src/ofa_kernel-$flavor \
  --with-linux=/usr/src/kernels/${kversion} \
  --with-downstream-release=$downstream_release \
   2>&1 | tee log-config
else
  ./configure \
  --enable-ext4 \
  --libexecdir=/usr/lib64 \
  --enable-tests \
  --enable-mpitests=yes \
  --disable-liblustre \
  --with-o2ib=/usr/src/ofa_kernel-${flavor} \
  --with-linux=/usr/src/${kversion} \
  --with-linux-obj=/usr/src/${kversion}-obj/${arch}/${flavor} \
  --with-downstream-release=$downstream_release \
   2>&1 | tee log-config
fi
