/*
 * GPL HEADER START
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 for more details (a copy is included
 * in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; If not, see
 * http://www.gnu.org/licenses/gpl-2.0.html
 *
 * GPL HEADER END
 */
/*
 * Copyright (c) 2018, DataDirect Networks Storage.
 * Author: Li Xi.
 *
 * Data integrity functions for OSD
 * Codes copied from kernel 3.10.0-862.el7
 * drivers/scsi/sd_dif.c and block/t10-pi.c
 */
#include <linux/blkdev.h>
#include <linux/blk_types.h>

#include <obd_cksum.h>
#include <lustre_compat.h>

#include "osd_internal.h"

#if IS_ENABLED(CONFIG_CRC_T10DIF)
/*
 * Data Integrity Field tuple.
 */
struct sd_dif_tuple {
       __be16 guard_tag;        /* Checksum */
       __be16 app_tag;          /* Opaque storage */
       __be32 ref_tag;          /* Target LBA or indirect LBA */
};

/*
 * Type 1 and Type 2 protection use the same format: 16 bit guard tag,
 * 16 bit app tag, 32 bit reference tag.
 */
static void osd_dif_type1_generate(struct blk_integrity_exchg *bix,
				   obd_dif_csum_fn *fn)
{
	void *buf = bix->data_buf;
	struct sd_dif_tuple *sdt = bix->prot_buf;
	struct bio *bio = bix->bio;
	struct osd_bio_private *bio_private = bio->bi_private;
	struct osd_iobuf *iobuf = bio_private->obp_iobuf;
	int index = bio_private->obp_start_page_idx + bix->bi_idx;
	struct niobuf_local *lnb = iobuf->dr_lnbs[index];
	__u16 *guard_buf = lnb->lnb_guards;
	sector_t sector = bix->sector;
	unsigned int i;

	for (i = 0 ; i < bix->data_size ; i += bix->sector_size, sdt++) {
		if (lnb->lnb_guard_rpc) {
			sdt->guard_tag = *guard_buf;
			guard_buf++;
		} else
			sdt->guard_tag = fn(buf, bix->sector_size);
		sdt->ref_tag = cpu_to_be32(sector & 0xffffffff);
		sdt->app_tag = 0;

		buf += bix->sector_size;
		sector++;
	}
}

static void osd_dif_type1_generate_crc(struct blk_integrity_exchg *bix)
{
	osd_dif_type1_generate(bix, obd_dif_crc_fn);
}

static void osd_dif_type1_generate_ip(struct blk_integrity_exchg *bix)
{
	osd_dif_type1_generate(bix, obd_dif_ip_fn);
}

static int osd_dif_type1_verify(struct blk_integrity_exchg *bix,
				obd_dif_csum_fn *fn)
{
	void *buf = bix->data_buf;
	struct sd_dif_tuple *sdt = bix->prot_buf;
	struct bio *bio = bix->bio;
	struct osd_bio_private *bio_private = bio->bi_private;
	struct osd_iobuf *iobuf = bio_private->obp_iobuf;
	int index = bio_private->obp_start_page_idx + bix->bi_idx;
	struct niobuf_local *lnb = iobuf->dr_lnbs[index];
	__u16 *guard_buf = lnb->lnb_guards;
	sector_t sector = bix->sector;
	unsigned int i;
	__u16 csum;

	for (i = 0 ; i < bix->data_size ; i += bix->sector_size, sdt++) {
		/* Unwritten sectors */
		if (sdt->app_tag == 0xffff)
			return 0;

		if (be32_to_cpu(sdt->ref_tag) != (sector & 0xffffffff)) {
			CERROR("%s: ref tag error on sector %lu (rcvd %u)\n",
			       bix->disk_name, (unsigned long)sector,
			       be32_to_cpu(sdt->ref_tag));
			return -EIO;
		}

		csum = fn(buf, bix->sector_size);

		if (sdt->guard_tag != csum) {
			CERROR("%s: guard tag error on sector %lu " \
			       "(rcvd %04x, data %04x)\n", bix->disk_name,
			       (unsigned long)sector,
			       be16_to_cpu(sdt->guard_tag), be16_to_cpu(csum));
			return -EIO;
		}

		*guard_buf = csum;
		guard_buf++;

		buf += bix->sector_size;
		sector++;
	}

	lnb->lnb_guard_disk = 1;
	return 0;
}

static int osd_dif_type1_verify_crc(struct blk_integrity_exchg *bix)
{
	return osd_dif_type1_verify(bix, obd_dif_crc_fn);
}

static int osd_dif_type1_verify_ip(struct blk_integrity_exchg *bix)
{
	return osd_dif_type1_verify(bix, obd_dif_ip_fn);
}

/*
 * Type 3 protection has a 16-bit guard tag and 16 + 32 bits of opaque
 * tag space.
 */
static void osd_dif_type3_generate(struct blk_integrity_exchg *bix,
				   obd_dif_csum_fn *fn)
{
	void *buf = bix->data_buf;
	struct sd_dif_tuple *sdt = bix->prot_buf;
	struct bio *bio = bix->bio;
	struct osd_bio_private *bio_private = bio->bi_private;
	struct osd_iobuf *iobuf = bio_private->obp_iobuf;
	int index = bio_private->obp_start_page_idx + bix->bi_idx;
	struct niobuf_local *lnb = iobuf->dr_lnbs[index];
	__u16 *guard_buf = lnb->lnb_guards;
	unsigned int i;

	for (i = 0 ; i < bix->data_size ; i += bix->sector_size, sdt++) {
		if (lnb->lnb_guard_rpc) {
			sdt->guard_tag = *guard_buf;
			guard_buf++;
		} else
			sdt->guard_tag = fn(buf, bix->sector_size);
		sdt->ref_tag = 0;
		sdt->app_tag = 0;

		buf += bix->sector_size;
	}
}

static void osd_dif_type3_generate_crc(struct blk_integrity_exchg *bix)
{
	osd_dif_type3_generate(bix, obd_dif_crc_fn);
}

static void osd_dif_type3_generate_ip(struct blk_integrity_exchg *bix)
{
	osd_dif_type3_generate(bix, obd_dif_ip_fn);
}

static int osd_dif_type3_verify(struct blk_integrity_exchg *bix,
				obd_dif_csum_fn *fn)
{
	void *buf = bix->data_buf;
	struct sd_dif_tuple *sdt = bix->prot_buf;
	struct bio *bio = bix->bio;
	struct osd_bio_private *bio_private = bio->bi_private;
	struct osd_iobuf *iobuf = bio_private->obp_iobuf;
	int index = bio_private->obp_start_page_idx + bix->bi_idx;
	struct niobuf_local *lnb = iobuf->dr_lnbs[index];
	__u16 *guard_buf = lnb->lnb_guards;
	sector_t sector = bix->sector;
	unsigned int i;
	__u16 csum;

	for (i = 0 ; i < bix->data_size ; i += bix->sector_size, sdt++) {
		/* Unwritten sectors */
		if (sdt->app_tag == 0xffff && sdt->ref_tag == 0xffffffff)
			return 0;

		csum = fn(buf, bix->sector_size);

		if (sdt->guard_tag != csum) {
			CERROR("%s: guard tag error on sector %lu " \
			       "(rcvd %04x, data %04x)\n", bix->disk_name,
			       (unsigned long)sector,
			       be16_to_cpu(sdt->guard_tag), be16_to_cpu(csum));
			return -EIO;
		}

		*guard_buf = csum;
		guard_buf++;

		buf += bix->sector_size;
		sector++;
	}

	lnb->lnb_guard_disk = 1;
	return 0;
}

static int osd_dif_type3_verify_crc(struct blk_integrity_exchg *bix)
{
	return osd_dif_type3_verify(bix, obd_dif_crc_fn);
}

static int osd_dif_type3_verify_ip(struct blk_integrity_exchg *bix)
{
	return osd_dif_type3_verify(bix, obd_dif_ip_fn);
}

int osd_get_integrity_profile(struct osd_device *osd,
			      integrity_gen_fn **generate_fn,
			      integrity_vrfy_fn **verify_fn)
{
	switch (osd->od_t10_type) {
	case OSD_T10_TYPE1_CRC:
		*verify_fn = osd_dif_type1_verify_crc;
		*generate_fn = osd_dif_type1_generate_crc;
		break;
	case OSD_T10_TYPE3_CRC:
		*verify_fn = osd_dif_type3_verify_crc;
		*generate_fn = osd_dif_type3_generate_crc;
		break;
	case OSD_T10_TYPE1_IP:
		*verify_fn = osd_dif_type1_verify_ip;
		*generate_fn = osd_dif_type1_generate_ip;
		break;
	case OSD_T10_TYPE3_IP:
		*verify_fn = osd_dif_type3_verify_ip;
		*generate_fn = osd_dif_type3_generate_ip;
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}
#endif /* CONFIG_CRC_T10DIF */
