/*
 *   This file is part of Lustre, https://wiki.hpdd.intel.com/
 *
 *   Portals is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   Portals is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Portals; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * header for lnet ioctl
 */
/*
 * Copyright (c) 2014, Intel Corporation.
 */
#ifndef _LNETCTL_H_
#define _LNETCTL_H_

#include <lnet/types.h>

/** \addtogroup lnet_fault_simulation
 * @{ */

enum {
	LNET_CTL_DROP_ADD,
	LNET_CTL_DROP_DEL,
	LNET_CTL_DROP_RESET,
	LNET_CTL_DROP_LIST,
	LNET_CTL_DELAY_ADD,
	LNET_CTL_DELAY_DEL,
	LNET_CTL_DELAY_RESET,
	LNET_CTL_DELAY_LIST,
};

#define LNET_ACK_BIT		(1 << 0)
#define LNET_PUT_BIT		(1 << 1)
#define LNET_GET_BIT		(1 << 2)
#define LNET_REPLY_BIT		(1 << 3)

/** ioctl parameter for LNet fault simulation */
struct lnet_fault_attr {
	/**
	 * source NID of drop rule
	 * LNET_NID_ANY is wildcard for all sources
	 * 255.255.255.255@net is wildcard for all addresses from @net
	 */
	lnet_nid_t			fa_src;
	/** destination NID of drop rule, see \a dr_src for details */
	lnet_nid_t			fa_dst;
	/**
	 * Portal mask to drop, -1 means all portals, for example:
	 * fa_ptl_mask = (1 << _LDLM_CB_REQUEST_PORTAL ) |
	 *		 (1 << LDLM_CANCEL_REQUEST_PORTAL)
	 *
	 * If it is non-zero then only PUT and GET will be filtered, otherwise
	 * there is no portal filter, all matched messages will be checked.
	 */
	__u64				fa_ptl_mask;
	/**
	 * message types to drop, for example:
	 * dra_type = LNET_DROP_ACK_BIT | LNET_DROP_PUT_BIT
	 *
	 * If it is non-zero then only specified message types are filtered,
	 * otherwise all message types will be checked.
	 */
	__u32				fa_msg_mask;
	union {
		/** message drop simulation */
		struct {
			/** drop rate of this rule */
			__u32			da_rate;
			/**
			 * time interval of message drop, it is exclusive
			 * with da_rate
			 */
			__u32			da_interval;
		} drop;
		/** message latency simulation */
		struct {
			__u32			la_rate;
			/**
			 * time interval of message delay, it is exclusive
			 * with la_rate
			 */
			__u32			la_interval;
			/** latency to delay */
			__u32			la_latency;
		} delay;
		__u64			space[8];
	} u;

};

/** fault simluation stats */
struct lnet_fault_stat {
	/** total # matched messages */
	__u64				fs_count;
	/** # dropped LNET_MSG_PUT by this rule */
	__u64				fs_put;
	/** # dropped LNET_MSG_ACK by this rule */
	__u64				fs_ack;
	/** # dropped LNET_MSG_GET by this rule */
	__u64				fs_get;
	/** # dropped LNET_MSG_REPLY by this rule */
	__u64				fs_reply;
	union {
		struct {
			/** total # dropped messages */
			__u64			ds_dropped;
		} drop;
		struct {
			/** total # delayed messages */
			__u64			ls_delayed;
		} delay;
		__u64			space[8];
	} u;
};

/** @} lnet_fault_simulation */

#define LNET_DEV_ID	0
#define LNET_DEV_PATH	"/dev/lnet"

#endif
