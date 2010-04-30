#!/bin/bash
set -x

TMP=${TMP:-/tmp}

TESTSUITELOG=${TESTSUITELOG:-$TMP/recovery-mds-scale}
LOG=${TESTSUITELOG}_$(basename $0)-$(hostname)
DEBUGLOG=${LOG}.debug

mkdir -p ${LOG%/*}

rm -f $LOG $DEBUGLOG
exec 2>$DEBUGLOG

. $(dirname $0)/functions.sh

IOR=${IOR:-"$(which IOR)"}

assert_env MOUNT END_RUN_FILE LOAD_PID_FILE IOR

trap signaled TERM

# if MACHINEFILE set and exists -- use it
if [ -z $MACHINEFILE ] || [ ! -e $MACHINEFILE ]; then
    MACHINEFILE=$TMP/$(basename $0)-$(hostname).machines
    echo $(hostname) >$MACHINEFILE
fi

THREADS_PER_CLIENT=${THREADS_PER_CLIENT:-3}
NUM_CLIENTS=$(cat $MACHINEFILE | wc -l)

# recovery-*-scale scripts use this to signal the client loads to die
echo $$ >$LOAD_PID_FILE

TESTDIR=${TESTDIR:-$MOUNT/d0.ior-$(hostname)}

CONTINUE=true
while [ ! -e "$END_RUN_FILE" ] && $CONTINUE; do
    echoerr "$(date +'%F %H:%M:%S'): IOR run starting"
    mkdir -p $TESTDIR
    # need this only if TESTDIR is not default
    chmod -R 777 $TESTDIR

    mpi_run -np $((NUM_CLIENTS * THREADS_PER_CLIENT)) $MACHINEFILE_OPTION ${MACHINEFILE} \
        $IOR -a POSIX -b 1g -o $TESTDIR/IOR-file -s 1 -t 1m -v -w -r 1>$LOG &
    load_pid=$!
    wait $load_pid
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
	echoerr "$(date +'%F %H:%M:%S'): IOR succeeded"
	cd $TMP
	rm -rf $TESTDIR
	echoerr "$(date +'%F %H:%M:%S'): IOR run finished"
    else
	echoerr "$(date +'%F %H:%M:%S'): IOR failed"
	if [ -z "$ERRORS_OK" ]; then
	    echo $(hostname) >> $END_RUN_FILE
	fi
	if [ $BREAK_ON_ERROR ]; then
	    # break
            CONTINUE=false
	fi
    fi
done

echoerr "$(date +'%F %H:%M:%S'): IOR run exiting"
