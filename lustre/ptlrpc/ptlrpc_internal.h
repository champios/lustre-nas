/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Copyright (C) 2003 Cluster File Systems, Inc.
 *
 *   This file is part of Lustre, http://www.lustre.org.
 *
 *   Lustre is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   Lustre is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Lustre; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Intramodule declarations for ptlrpc. */

#ifndef PTLRPC_INTERNAL_H
#define PTLRPC_INTERNAL_H

#include "../ldlm/ldlm_internal.h"

struct ldlm_namespace;
struct obd_import;
struct ldlm_res_id;
struct ptlrpc_request_set;

void ptlrpc_daemonize(void);

void ptlrpc_request_handle_notconn(struct ptlrpc_request *);
void lustre_assert_wire_constants(void);

#ifdef __KERNEL__
void ptlrpc_lprocfs_register_service(struct proc_dir_entry *proc_entry,
                                     struct ptlrpc_service *svc);
void ptlrpc_lprocfs_unregister_service(struct ptlrpc_service *svc);
void ptlrpc_lprocfs_rpc_sent(struct ptlrpc_request *req);
void ptlrpc_lprocfs_do_request_stat (struct ptlrpc_request *req,
                                     long q_usec, long work_usec);
#else
#define ptlrpc_lprocfs_register_service(params...) do{}while(0)
#define ptlrpc_lprocfs_unregister_service(params...) do{}while(0)
#define ptlrpc_lprocfs_rpc_sent(params...) do{}while(0)
#define ptlrpc_lprocfs_do_request_stat(params...) do{}while(0)
#endif /* __KERNEL__ */

/* recovd_thread.c */
int llog_init_commit_master(void);
int llog_cleanup_commit_master(int force);

static inline int opcode_offset(__u32 opc) {
        if (opc < OST_LAST_OPC) {
                 /* OST opcode */
                return (opc - OST_FIRST_OPC);
        } else if (opc < MDS_LAST_OPC) {
                /* MDS opcode */
                return (opc - MDS_FIRST_OPC +
                        (OST_LAST_OPC - OST_FIRST_OPC));
        } else if (opc < LDLM_LAST_OPC) {
                /* LDLM Opcode */
                return (opc - LDLM_FIRST_OPC +
                        (MDS_LAST_OPC - MDS_FIRST_OPC) +
                        (OST_LAST_OPC - OST_FIRST_OPC));
        } else if (opc < PTLBD_LAST_OPC) {
                /* Portals Block Device */
                return (opc - PTLBD_FIRST_OPC +
                        (LDLM_LAST_OPC - LDLM_FIRST_OPC) +
                        (MDS_LAST_OPC - MDS_FIRST_OPC) +
                        (OST_LAST_OPC - OST_FIRST_OPC));
        } else if (opc < OBD_LAST_OPC) {
                /* OBD Ping */
                return (opc - OBD_FIRST_OPC +
                        (PTLBD_LAST_OPC - PTLBD_FIRST_OPC) +
                        (LDLM_LAST_OPC - LDLM_FIRST_OPC) +
                        (MDS_LAST_OPC - MDS_FIRST_OPC) +
                        (OST_LAST_OPC - OST_FIRST_OPC));
        } else {
                /* Unknown Opcode */
                return -1;
        }
}

#define LUSTRE_MAX_OPCODES ((PTLBD_LAST_OPC - PTLBD_FIRST_OPC) + \
                            (LDLM_LAST_OPC - LDLM_FIRST_OPC)   + \
                            (MDS_LAST_OPC - MDS_FIRST_OPC)     + \
                            (OST_LAST_OPC - OST_FIRST_OPC)     + \
                            (OBD_LAST_OPC - OBD_FIRST_OPC))

enum {
        PTLRPC_REQWAIT_CNTR     = 0,
        PTLRPC_SVCIDLETIME_CNTR = 1,
        //PTLRPC_SVCEQDEPTH_CNTR,
        PTLRPC_LAST_CNTR
};

int ptlrpc_expire_one_request(struct ptlrpc_request *req);

/* pinger.c */
int ptlrpc_start_pinger(void);
int ptlrpc_stop_pinger(void);
void ptlrpc_pinger_sending_on_import(struct obd_import *imp);

/* ptlrpc_local.c */
int local_send_rpc(struct ptlrpc_request *req);
int local_reply(struct ptlrpc_request *req);
int local_error(struct ptlrpc_request *req);
int local_bulk_move(struct ptlrpc_bulk_desc *desc);

#endif /* PTLRPC_INTERNAL_H */
