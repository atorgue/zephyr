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
static unsigned int vring_notifyid;

void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	vring_notifyid = id ? VRING_RX_ID : VRING_TX_ID;
	k_sem_give(&data_sem);
}

int platform_poll(struct virtio_device *vdev)
{
	int status ;

	printk("->%s(): vring_notifyid %d\n", __func__, vring_notifyid);
	status = k_sem_take(&data_sem, K_FOREVER);
	if (status == 0) {
		rproc_virtio_notified(vdev, vring_notifyid);
	}

	return status;
}
