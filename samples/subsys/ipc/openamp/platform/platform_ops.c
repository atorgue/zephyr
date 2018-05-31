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
	vring_notifyid |=  1 << id;
	if (k_sem_count_get(&data_sem) < 2);
		k_sem_give(&data_sem);
}

int platform_poll(struct virtio_device *vdev)
{
	int status ;
	unsigned int id_mask;
	
	status = k_sem_take(&data_sem, K_FOREVER);
	if (!status && vring_notifyid) {
		id_mask = vring_notifyid;
		vring_notifyid = 0;

		if (id_mask & (1 << VRING_TX_ID)) {
			rproc_virtio_notified(vdev, VRING_TX_ID);
		}
		if (id_mask &  (1 << VRING_RX_ID)) {
			rproc_virtio_notified(vdev, VRING_RX_ID);
		}
	}

	return status;
}
