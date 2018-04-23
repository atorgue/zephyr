/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#include <openamp/open_amp.h>

#define RSC_TABLE_ADDRESS       0x04000000

#if defined(CPU_LPC54114J256BD64_cm4)
OPENAMP_PACKED_BEGIN
struct lpc_resource_table {
	uint32_t ver;
	uint32_t num;
	uint32_t reserved[2];
	uint32_t offset[2];
	struct fw_rsc_rproc_mem mem;
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0, vring1;
} OPENAMP_PACKED_END;

#elif defined(CONFIG_BOARD_STM32M4_DISCO)

OPENAMP_PACKED_BEGIN
struct  stm32_resource_table { 
	unsigned int ver;
	unsigned int num;
	unsigned int reserved[2];
	unsigned int offset[2];
	/* text carveout entry */
	struct fw_rsc_carveout cm_retram_cout;
	struct fw_rsc_carveout cm_mcuram_cout;

	/* firmware checksum */
	struct fw_rsc_fw_chksum fw_chksum;
	/* rpmsg vdev entry */
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
	struct fw_rsc_trace cm_trace;
} OPENAMP_PACKED_END;
#endif

void resource_table_init(void **table_ptr, int *length);

#endif

