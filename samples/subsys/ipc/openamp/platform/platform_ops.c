/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ipm.h>

#include <openamp/open_amp.h>
#include <openamp/remoteproc_virtio.h>
#include "platform.h"
#include "resource_table.h"
#include "metal/device.h"

#if 0
#define DEBUG_TRACE(...) printk(...)
#else
#define DEBUG_TRACE(...)
#endif

static K_SEM_DEFINE(data_sem, 0, 1);
static struct device *ipm_handle = NULL;
extern struct metal_device shm_device;
static unsigned int vring_notifyid;

static void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	vring_notifyid = id ? VRING_RX_ID : VRING_TX_ID;
	k_sem_give(&data_sem);
}

int platform_kick(struct remoteproc *rproc, uint32_t id)
{
	uint32_t dummy_data = 0x12345678; /* Some data must be provided */

	if (!rproc) {
		DEBUG_TRACE("null intr_info\r\n");
		return -EINVAL;
	}

	DEBUG_TRACE("->%s() for vring %d\n", __func__, id);
	ipm_send(ipm_handle, 0, id, &dummy_data, sizeof(dummy_data));

	return 0;
}

struct remoteproc platform_rproc;

static struct remoteproc *platform_init(struct remoteproc_ops *ops, void *priv)
{
	printk(" %s enter\n", __func__);
	ipm_handle = device_get_binding("MAILBOX_0");
	if (!ipm_handle) {
		return NULL;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);
	platform_rproc.priv = priv;
	
	return &platform_rproc;
}

void *platform_mmap (struct remoteproc *rproc,
		      metal_phys_addr_t *pa, metal_phys_addr_t *da,
		      size_t size, unsigned int attribute,
		      struct metal_io_region **io)
{
	(void) rproc;
	(void) size;
	(void) attribute;
	
	printk(" %s enter\n", __func__);
 	*io =  &shm_device.regions[RSC_IO_REGION_ID]; 
	*pa = *da;
	return (void *)*da;
}

struct remoteproc_ops platform_rproc_ops = {
	.init = platform_init,
	.notify = platform_kick,
	.mmap = platform_mmap,
};

struct remoteproc platform_rproc = {
	.ops = &platform_rproc_ops,
};


int platform_poll(struct virtio_device *vdev)
{
	int status ;

	status = k_sem_take(&data_sem, K_FOREVER);
	if (status == 0) {
		rproc_virtio_notified(vdev, vring_notifyid);
	}

	return status;
}
