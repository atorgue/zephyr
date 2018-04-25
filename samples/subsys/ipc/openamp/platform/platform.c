/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "platform.h"
#include "resource_table.h"
#include "openamp/remoteproc.h"
#include "metal/device.h"

#if 0
#define DEBUG_TRACE(...) printk(...)
#else
#define DEBUG_TRACE(...)
#endif

#define SHM_IO_REGION_ID 0
#define _IO_REGION_ID 0

extern struct remoteproc_ops platform_rproc_ops;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDRESS };
struct metal_device shm_device = {
 	.name = SHM_DEVICE_NAME,
 	.bus = "generic",
 	.num_regions = 2,
 	.regions = {
 		{.name  = "shm"},
 		{.name  = "rsc table"},
 	},
 	.node = { NULL },
	.irq_num = 0,
 	.irq_info = NULL
};

struct remoteproc *platform_create_proc(int role)
{
	ARG_UNUSED(role);
	void *rsc_table;
	int rsc_size;
	struct metal_device *device;
	struct remoteproc *rproc;
	int ret = 0;

	ret = metal_register_generic_device(&shm_device);
	if (ret != 0) {
		printk("could not register shmem device: error code %d\n", ret);
	}

        ret = metal_device_open("generic", SHM_DEVICE_NAME, &device);

        if (ret != 0) {
                printk("metal_device_open failed %d\n", ret);
        }
 
 	metal_io_init(&device->regions[SHM_IO_REGION_ID], 
		      (void *)SHM_START_ADDRESS,
		      shm_physmap, SHM_SIZE, -1, 0, NULL);

	/* Initialize remoteproc instance */
	rproc = remoteproc_init(&platform_rproc_ops, NULL);
	if (!rproc)
		return NULL;

	resource_table_init(&rsc_table, &rsc_size);

	metal_io_init(&device->regions[RSC_IO_REGION_ID], rsc_table,
		      (metal_phys_addr_t *)rsc_table, rsc_size, -1, 0, NULL);
	
	rproc->rsc_io = &device->regions[RSC_IO_REGION_ID];
	rproc->rsc_table = rsc_table;
	rproc->rsc_len = rsc_size;
	rproc->rsc_io = &device->regions[RSC_IO_REGION_ID];
	
	ret = remoteproc_parse_rsc_table(rproc, rsc_table, rsc_size);
	if (ret) {
		printk(" failed to parse resource table\n");
		remoteproc_remove(rproc);
		return NULL;
	}

return rproc;
}


struct  rpmsg_virtio_device *
platform_create_rpmsg_vdev(struct remoteproc *rproc, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev))
{
	struct rpmsg_virtio_device *rpmsg_vdev;
	struct virtio_device *vdev;
	struct metal_device *device;
	int ret;

 	printk(" %s enter\n", __func__);
        ret = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	printk("device open %d\n", ret);
        if (ret != 0) {
                printk("metal_device_open failed %d\n", ret);
		return NULL;
        }
 
	rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
	if (!rpmsg_vdev) 
		return NULL;

	/* TODO: can we have a wrapper for the following two functions? */
	vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
	if (!vdev) {
		printk("remoteproc_create_virtio failed %d\n", ret);
		goto err1;
	}

	ret = rpmsg_init_vdev(rpmsg_vdev, vdev,
			       &device->regions[SHM_IO_REGION_ID],
			       (void *)SHM_START_ADDRESS, SHM_SIZE);
	if (ret) {
		printk("rpmsg_init_vdev failed %d\n", ret);
		remoteproc_remove_virtio(rproc, vdev);
	} else {
		printk("rpmsg_init_vdev Ok %d\n", ret);
		return rpmsg_vdev;
	}
err1:
	metal_free_memory(rpmsg_vdev);
	return NULL;
}
