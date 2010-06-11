#!/bin/bash

RELEASE=$1

if [ -z "$RELEASE" ]; then
    echo "Usage: $0 release-string"
    exit 1
fi

# needed to build MPI tests
module load comp-intel/11.1.046 mpi-mvapich2/1.2p1/intel-PIC

version=$(uname -r | cut -f-2 -d-)
flavor=$(uname -r | cut -f3 -d-)
#nas_release=$(cat NAS_RELEASE)

# run autogen.sh

bash ./autogen.sh

./configure \
  --with-o2ib=/usr/src/ofa_kernel-$flavor \
  --with-linux=/usr/src/linux-${flavor}-${version} \
  --with-linux-config=/usr/src/linux-obj/x86_64/$flavor/.config \
  --with-linux-obj=/usr/src/linux-obj/x86_64/$flavor \
  --disable-liblustre \
  --with-3rd-party-version=$RELEASE \
   2>&1 | tee log-config


