/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "platform.h"
#include "resource_table.h"

#if 0
#define DEBUG_TRACE(...) printk(...)
#else
#define DEBUG_TRACE(...)
#endif

extern struct hil_platform_ops platform_ops;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDRESS };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	{
		{
			.virt       = (void *) SHM_START_ADDRESS,
			.physmap    = shm_physmap,
			.size       = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask  = 0xffffffff,
			.mem_flags  = 0,
			.ops        = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

struct hil_proc *platform_init(int role)
{
	int status;
	static uint32_t vring0_id = 0;
	static uint32_t vring1_id = 1;

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		DEBUG_TRACE("metal_register_generic_device(): could not register shared memory device: error code %d\n", status);
		return NULL;
	}

	printf("platform_create():\n");
	struct hil_proc *proc = hil_create_proc(&platform_ops, role != RPMSG_MASTER, NULL);
	if (proc == NULL) {
		DEBUG_TRACE("platform_create(): could not allocate hil_proc\n");
		return NULL;
	}

	/* Setup IPI info */
	DEBUG_TRACE("hil_set_vring_ipi\r\n");
	hil_set_vring_ipi(proc, 0, NULL, &vring0_id);
	DEBUG_TRACE("hil_set_vring_ipi\r\n");
	hil_set_vring_ipi(proc, 1, NULL, &vring1_id);

	hil_set_shm(proc, "generic", SHM_DEVICE_NAME, SHM_START_ADDRESS, SHM_SIZE);
	return proc;
}

