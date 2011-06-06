#!/bin/bash

. /usr/share/modules/init/bash

RELEASE=$1

if [[ -z $RELEASE ]]; then
    echo "Usage: $0 release-string"
    exit 1
fi

# needed to build MPI tests
module load comp-intel/11.1.046 mpi-mvapich2/1.2p1/intel-PIC

version=$(uname -r | sed -e 's/\(.*\)lustre.*$/\1/')
flavor=$(uname -r | sed -e 's/.*\(lustre.*\)$/\1/')
arch=$(uname -m)

if [[ -z $flavor ]]; then
    echo "Not running a Lustre patched kernel."
    exit 1
fi

# run autogen.sh

bash ./autogen.sh

./configure \
  --enable-ext4 \
  --with-o2ib=/usr/src/ofa_kernel-$flavor \
  --with-linux=/usr/src/kernels/${version}${flavor}-$arch \
  --disable-liblustre \
  --with-3rd-party-version=$RELEASE \
   2>&1 | tee log-config


