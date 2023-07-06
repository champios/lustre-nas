#!/bin/bash

set -e

ONLY=${ONLY:-"$*"}

LUSTRE=${LUSTRE:-$(dirname $0)/..}
. $LUSTRE/tests/test-framework.sh
init_test_env $@
init_logging

ALWAYS_EXCEPT="$SCRUB_PERFORMANCE_EXCEPT"
build_test_filter

[ "$SLOW" = "no" ] &&
	skip "skip scrub performance test under non-SLOW mode" && exit 0

[ "$mds1_FSTYPE" != ldiskfs ] &&
	skip "ldiskfs only test"
[[ "$MDS1_VERSION" -lt $(version_code 2.2.90) ]] &&
	skip "Need MDS version at least 2.2.90"
require_dsh_mds || exit 0

NTHREADS=${NTHREADS:-0}
UNIT=${UNIT:-1048576}
BACKUP=${BACKUP:-0}
MINCOUNT=${MINCOUNT:-8192}
MAXCOUNT=${MAXCOUNT:-32768}
FACTOR=${FACTOR:-2}

RCMD="do_facet ${SINGLEMDS}"
RLCTL="${RCMD} ${LCTL}"
MDT_DEV="${FSNAME}-MDT0000"
MDT_DEVNAME=$(mdsdevname ${SINGLEMDS//mds/})
SHOW_SCRUB="${RLCTL} get_param -n osd-ldiskfs.${MDT_DEV}.oi_scrub"
remote_mds && ECHOCMD=${RCMD} || ECHOCMD="eval"

if [ ${NTHREADS} -eq 0 ]; then
	CPUCORE=$(${RCMD} cat /proc/cpuinfo | grep "processor.*:" | wc -l)
	NTHREADS=$((CPUCORE * 2))
fi

cleanupall

if ! combined_mgs_mds ; then
	do_rpc_nodes $(facet_active_host mgs) load_modules_local
	add mgs $(mkfs_opts mgs $(mgsdevname)) --backfstype ldiskfs \
		--reformat $(mgsdevname) $(mgsvdevname) ${quiet:+>/dev/null} ||
		exit 1

	start mgs $(mgsdevname) $MGS_MOUNT_OPTS || error "Fail to start MGS!"
fi

do_rpc_nodes $(facet_active_host $SINGLEMDS) load_modules_local
reformat_external_journal ${SINGLEMDS}
add ${SINGLEMDS} $(mkfs_opts ${SINGLEMDS} ${MDT_DEVNAME}) --backfstype ldiskfs \
	--reformat ${MDT_DEVNAME} $(mdsvdevname 1) ${quiet:+>/dev/null} ||
	exit 2

scrub_attach() {
	${ECHOCMD} "${LCTL} <<-EOF
		attach echo_client scrub-MDT0000 scrub-MDT0000_UUID
		setup ${MDT_DEV} mdd
	EOF"
}

scrub_detach() {
	${ECHOCMD} "${LCTL} <<-EOF
		--device scrub-MDT0000 cleanup
		--device scrub-MDT0000 detach
	EOF"
}

scrub_create() {
	local echodev=$(${RLCTL} dl | grep echo_client|awk '{print $1}')
	local j

	${ECHOCMD} "${LCTL} --device ${echodev} test_mkdir ${tdir}"

	for ((j = 1; j < ${threads}; j++)); do
		${ECHOCMD} "${LCTL} --device ${echodev} test_mkdir ${tdir}${j}"
	done

	${ECHOCMD} "${LCTL} --device ${echodev} \
		--threads ${threads} 0 ${echodev} \
		test_create -d${tdir} -D${threads} -b${lbase} -c0 -n${usize}"
	EOF"
}

scrub_cleanup() {
	cleanupall
	do_rpc_nodes $(facet_active_host $SINGLEMDS) unload_modules
	if ! combined_mgs_mds ; then
		do_rpc_nodes $(facet_active_host mgs) unload_modules
	fi
	REFORMAT="yes" cleanup_and_setup_lustre
}

scrub_create_nfiles() {
	local total=$1
	local lbase=$2
	local threads=$3
	local ldir="/test-${lbase}"
	local cycle=0
	local count=${UNIT}

	while true; do
		[ ${count} -eq 0 -o  ${count} -gt ${total} ] && count=${total}
		local usize=$((count / NTHREADS))
		[ ${usize} -eq 0 ] && break
		local tdir=${ldir}-${cycle}-

		echo "[cycle: ${cycle}] [threads: ${threads}]"\
		     "[files: ${count}] [basedir: ${tdir}]"
		start ${SINGLEMDS} $MDT_DEVNAME $MDS_MOUNT_OPTS ||
			error "Fail to start MDS!"
		scrub_attach
		scrub_create
		scrub_detach
		stop ${SINGLEMDS} || error "Fail to stop MDS!"

		total=$((total - usize * NTHREADS))
		[ ${total} -eq 0 ] && break
		lbase=$((lbase + usize))
		cycle=$((cycle + 1))
	done
}

test_0() {
	local BASECOUNT=0
	local i

	for ((i=$MINCOUNT; i<=$MAXCOUNT; i=$((i * FACTOR)))); do
		local nfiles=$((i - BASECOUNT))
		local stime=$(date +%s)

		echo "+++ start to create for ${i} files set at: $(date) +++"
		scrub_create_nfiles ${nfiles} ${BASECOUNT} ${NTHREADS} ||
			error "Fail to create files!"
		echo "+++ end to create for ${i} files set at: $(date) +++"
		local etime=$(date +%s)
		local delta=$((etime - stime))
		[ $delta -gt 0 ] || delta=1
		echo "create ${nfiles} files used ${delta} seconds"
		echo "create speed is $((nfiles / delta))/sec"

		BASECOUNT=${i}
		if [ ${BACKUP} -ne 0 ]; then
			stime=$(date +%s)
			echo "backup/restore ${i} files start at: $(date)"
			mds_backup_restore $SINGLEMDS ||
				error "Fail to backup/restore!"
			echo "backup/restore ${i} files end at: $(date)"
			etime=$(date +%s)
			delta=$((etime - stime))
			[ $delta -gt 0 ] || delta=1
			echo "backup/restore ${i} files used ${delta} seconds"
			echo "backup/restore speed is $((i / delta))/sec"
		else
			mds_remove_ois $SINGLEMDS ||
				error "Fail to remove/recreate!"
		fi

		echo "--- start to rebuild OI for $i files set at: $(date) ---"
		start ${SINGLEMDS} $MDT_DEVNAME $MDS_MOUNT_OPTS > /dev/null ||
			error "Fail to start MDS!"

		while true; do
			local STATUS=$($SHOW_SCRUB |
					awk '/^status/ { print $2 }')
			[ "$STATUS" == "completed" ] && break
			sleep 3 # check status every 3 seconds
		done

		echo "--- end to rebuild OI for ${i} files set at: $(date) ---"
		local RTIME=$($SHOW_SCRUB | awk '/^run_time/ { print $2 }')
		echo "rebuild OI for ${i} files used ${RTIME} seconds"
		local SPEED=$($SHOW_SCRUB | awk '/^average_speed/ { print $2 }')
		echo "rebuild speed is ${SPEED}/sec"
		stop ${SINGLEMDS} > /dev/null || error "Fail to stop MDS!"
	done
}
run_test 0 "OI scrub performance test"

# cleanup the system at last
scrub_cleanup
complete $SECONDS
check_and_cleanup_lustre
exit_status
