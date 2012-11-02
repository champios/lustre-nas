FSNAME=${FSNAME:-lustre}

# facet hosts
mds_HOST=${mds_HOST:-`hostname`}
mdsfailover_HOST=${mdsfailover_HOST}
mds1_HOST=${mds1_HOST:-$mds_HOST}
mds1failover_HOST=${mds1failover_HOST:-$mdsfailover_HOST}
mgs_HOST=${mgs_HOST:-$mds1_HOST}
ost_HOST=${ost_HOST:-`hostname`}
ostfailover_HOST=${ostfailover_HOST}
CLIENTS=""

TMP=${TMP:-/tmp}

DAEMONSIZE=${DAEMONSIZE:-500}
MDSCOUNT=${MDSCOUNT:-1}
[ $MDSCOUNT -gt 4 ] && MDSCOUNT=4
[ $MDSCOUNT -gt 1 ] && IAMDIR=yes
for num in $(seq $MDSCOUNT); do
    eval mds${num}_HOST=\$\{mds${num}_HOST:-$mds_HOST\}
    eval mds${num}failover_HOST=\$\{mds${num}failover_HOST:-$mdsfailover_HOST\}
done
MDSDEVBASE=${MDSDEVBASE:-$TMP/${FSNAME}-mdt}
MDSSIZE=${MDSSIZE:-200000}
#
# Format options of facets can be specified with these variables:
#
#   - <facet_type>OPT
#
# Arguments for "--mkfsoptions" shall be specified with these
# variables:
#
#   - <fstype>_MKFS_OPTS
#   - <facet_type>_FS_MKFS_OPTS
#
# A number of other options have their own specific variables.  See
# mkfs_opts().
#
MDSOPT=${MDSOPT:-}
MDS_FS_MKFS_OPTS=${MDS_FS_MKFS_OPTS:-}
MDS_MOUNT_OPTS=${MDS_MOUNT_OPTS:-}

MGSDEV=${MGSDEV:-$MDSDEV1}
MGSSIZE=${MGSSIZE:-$MDSSIZE}
MGSOPT=${MGSOPT:-}
MGS_FS_MKFS_OPTS=${MGS_FS_MKFS_OPTS:-}
MGS_MOUNT_OPTS=${MGS_MOUNT_OPTS:-}

OSTCOUNT=${OSTCOUNT:-2}
OSTDEVBASE=${OSTDEVBASE:-$TMP/${FSNAME}-ost}
OSTSIZE=${OSTSIZE:-200000}
OSTOPT=${OSTOPT:-}
OST_FS_MKFS_OPTS=${OST_FS_MKFS_OPTS:-}
OST_MOUNT_OPTS=${OST_MOUNT_OPTS:-}
# Can specify individual ost devs with
# OSTDEV1="/dev/sda"
# on specific hosts with
# ost1_HOST="uml2"

NETTYPE=${NETTYPE:-tcp}
MGSNID=${MGSNID:-`h2$NETTYPE $mgs_HOST`}

#
# Back end file system type(s) of facets can be specified with these
# variables:
#
#   1. <facet>_FSTYPE
#   2. <facet_type>FSTYPE
#   3. FSTYPE
#
# More specific ones override more general ones.  See facet_fstype().
#
FSTYPE=${FSTYPE:-ldiskfs}

LDISKFS_MKFS_OPTS=${LDISKFS_MKFS_OPTS:-}
ZFS_MKFS_OPTS=${ZFS_MKFS_OPTS:-}

#
# If any OST is "remote" and the non-default implementation (e.g.,
# current OFD) is desired, then make sure that either a)
# LOAD_MODULES_REMOTE is true or b) modprobe(8) is configured to
# blacklist the undesired (and aliased the other, if necessary).
#
USE_OFD=${USE_OFD:-no}

STRIPE_BYTES=${STRIPE_BYTES:-1048576}
STRIPES_PER_OBJ=${STRIPES_PER_OBJ:-0}
SINGLEMDS=${SINGLEMDS:-"mds1"}
TIMEOUT=${TIMEOUT:-20}
PTLDEBUG=${PTLDEBUG:-"vfstrace rpctrace dlmtrace neterror ha config ioctl super"}
SUBSYSTEM=${SUBSYSTEM:-"all -lnet -lnd -pinger"}

# promise 2MB for every cpu
if [ -f /sys/devices/system/cpu/possible ]; then
    _debug_mb=$((($(cut -d "-" -f 2 /sys/devices/system/cpu/possible)+1)*2))
else
    _debug_mb=$(($(getconf _NPROCESSORS_CONF)*2))
fi

DEBUG_SIZE=${DEBUG_SIZE:-$_debug_mb}

ENABLE_QUOTA=${ENABLE_QUOTA:-""}
QUOTA_TYPE="ug3"
QUOTA_USERS=${QUOTA_USERS:-"quota_usr quota_2usr sanityusr sanityusr1"}
LQUOTAOPTS=${LQUOTAOPTS:-"hash_lqs_cur_bits=3"}

#client
MOUNT=${MOUNT:-/mnt/${FSNAME}}
MOUNT1=${MOUNT1:-$MOUNT}
MOUNT2=${MOUNT2:-${MOUNT}2}
MOUNTOPT=${MOUNTOPT:-"-o user_xattr,flock"}
DIR=${DIR:-$MOUNT}
DIR1=${DIR:-$MOUNT1}
DIR2=${DIR2:-$MOUNT2}

if [ $UID -ne 0 ]; then
        log "running as non-root uid $UID"
        RUNAS_ID="$UID"
        RUNAS_GID=`id -g $USER`
        RUNAS=""
else
        RUNAS_ID=${RUNAS_ID:-500}
        RUNAS_GID=${RUNAS_GID:-$RUNAS_ID}
        RUNAS=${RUNAS:-"runas -u $RUNAS_ID -g $RUNAS_GID"}
fi

PDSH=${PDSH:-no_dsh}
FAILURE_MODE=${FAILURE_MODE:-SOFT} # or HARD
POWER_DOWN=${POWER_DOWN:-"powerman --off"}
POWER_UP=${POWER_UP:-"powerman --on"}
SLOW=${SLOW:-no}
FAIL_ON_ERROR=${FAIL_ON_ERROR:-true}

MPIRUN=$(which mpirun 2>/dev/null) || true
MPI_USER=${MPI_USER:-mpiuser}
MACHINEFILE_OPTION=${MACHINEFILE_OPTION:-"-machinefile"}
SHARED_DIR_LOGS=${SHARED_DIR_LOGS:-""}

# This is used by a small number of tests to share state between the client
# running the tests, or in some cases between the servers (e.g. lfsck.sh).
# It needs to be a non-lustre filesystem that is available on all the nodes.
SHARED_DIRECTORY=${SHARED_DIRECTORY:-""}	# bug 17839 comment 65
