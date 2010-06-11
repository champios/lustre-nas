#!/bin/bash

# needed to build MPI tests
module load comp-intel/11.1.046 mpi-mvapich2/1.2p1/intel-PIC

make rpms 2>&1 | tee log-rpms

