#!/bin/bash

if [ -f /etc/SuSE-release ]; then
	. /usr/share/modules/init/bash
else if [ -f /etc/redhat-release ]; then
	. /usr/share/Modules/init/bash
     else
	echo "Unknown distro";
	exit 1;
     fi
fi

# needed to build MPI tests
module load /nasa/modulefiles/mpi-mvapich2/1.2p1/gcc

export RPM_BUILD_NCPUS=8
make 2>&1 | tee log-make	# Needed due to LU-788
make rpms 2>&1 | tee log-rpms

