/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform.h"
#include "resource_table.h"


#if defined(CPU_LPC54114J256BD64_cm4)
struct lpc_resource_table *rsc_table_ptr = (void *) RSC_TABLE_ADDRESS;

static const struct lpc_resource_table resource_table = {
	.ver = 1,
	.num = 2,
	.offset = {
		offsetof(struct lpc_resource_table, mem),
		offsetof(struct lpc_resource_table, vdev),
	},
	.mem = { RSC_RPROC_MEM, SHM_START_ADDRESS, SHM_START_ADDRESS, SHM_SIZE, 0 },
	.vdev = { RSC_VDEV, VIRTIO_ID_RPMSG, 0, 1 << VIRTIO_RPMSG_F_NS, 0, 0, 0, VRING_COUNT, { 0, 0 } },
	.vring0 = { VRING_TX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, VRING_TX_ID, 0 },
	.vring1 = { VRING_RX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, VRING_RX_ID, 0 },
};
#elif defined(CONFIG_BOARD_ST)

extern char ram_console[];

#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)
static const struct st_resource_table __resource resource_table = { \
	.ver = 1,
	.num = 2,
	.offset = {
		offsetof(struct st_resource_table, vdev),
		offsetof(struct st_resource_table, cm_trace), \
	},
	.vdev = { 
		  RSC_VDEV, VIRTIO_ID_RPMSG, VDEV_ID, 1 << VIRTIO_RPMSG_F_NS, 0, 0, 0,
		  VRING_COUNT, { 0, 0 }, 
	},

	.vring0 = { VRING_TX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 1, 0 },
	.vring1 = { VRING_RX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 2, 0 },
	.cm_trace = {
		RSC_TRACE,
		(uint32_t)ram_console, CONFIG_RAM_CONSOLE_BUFFER_SIZE + 1, 0, "cm4_log",
	},
};
#endif

void resource_table_init(void **table_ptr, int *length)
{
#if defined(CPU_LPC54114J256BD64_cm4)
	/* Master: copy the resource table to shared memory. */
	memcpy(rsc_table_ptr, &resource_table, sizeof(resource_table));
	*table_ptr = rsc_table_ptr;
#elif defined(CONFIG_BOARD_ST)
	*table_ptr = &resource_table;
#endif
	
	*length = sizeof(resource_table);
}

