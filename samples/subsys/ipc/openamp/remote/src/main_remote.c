/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ipm.h>
#include <openamp/open_amp.h>
#include "platform.h"
#include "resource_table.h"
#include "metal/device.h"

#if 1
#define DEBUG_TRACE printk
#else
#define DEBUG_TRACE(...)
#endif

#define RPMSG_CHAN_NAME "rpmsg-tty-channel"

#define APP_TASK_STACK_SIZE (256)

K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);

static struct k_thread thread_data;

static K_SEM_DEFINE(message_rcv, 0, 1);

static char rcv_data[256];
static volatile unsigned int rcv_len;
static struct rpmsg_endpoint *tty_ept;
static struct rpmsg_endpoint *rcv_ept;
//static struct rpmsg_virtio_device *rvdev;
static struct virtqueue vq[2];
static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct st_resource_table *rsc_table;
static int rsc_size;
static struct device *ipm_handle = NULL;

extern int platform_kick(struct remoteproc *rproc, uint32_t id);
extern int platform_poll(struct virtio_device *vdev);
extern void platform_ipm_callback(void *context, u32_t id, volatile void *data);

static void rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data,
				size_t len, uint32_t src, void *priv)
{
	printk("%s:\n", __func__);
	rcv_ept = ept;
	memcpy(rcv_data, data, len);
	rcv_len = len;
	k_sem_give(&message_rcv);
}

static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.align = VRING_ALIGNMENT,
	},
	[1] = {
		.align = VRING_ALIGNMENT,
	},
};

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDRESS };
struct metal_device shm_device = {
 	.name = SHM_DEVICE_NAME,
 	.bus = "generic",
 	.num_regions = 1,
 	.regions = {
 		{.name  = "shm"},
 	},
 	.node = { NULL },
	.irq_num = 0,
 	.irq_info = NULL
};

static char *receive_message(struct rpmsg_virtio_device *rvdev)
{
	printk("%s:\n", __func__);
	while (k_sem_take(&message_rcv, K_NO_WAIT) != 0)
		platform_poll(rvdev->vdev);
	return rcv_data;
}

static int send_message(char *message)
{
	printk("%s:\n", __func__);
	return rpmsg_send(rcv_ept, message, rcv_len);
}

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	printk("%s:\n", __func__);
	return rsc_table->vdev.status;
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return rsc_table->vdev.dfeatures;
}

static void virtio_notify(struct virtqueue *vq)
{
	uint32_t dummy_data = 0x12345678; /* Some data must be provided */

	printk(" NAME %s\n", vq->vq_name);

	ipm_send(ipm_handle, 0, vq->vq_queue_index, &dummy_data,
		 sizeof(dummy_data));
}

virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
	.notify = virtio_notify,
};

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	struct metal_device *device;
	void* rsc_tab_addr;
	unsigned char *msg;

	printk("\r\nOpenAMP demo started\r\n");

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	resource_table_init(&rsc_tab_addr, &rsc_size);
	rsc_table = (struct st_resource_table *)rsc_tab_addr;
	
	metal_init(&metal_params);

	printk("metal init\n");

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		printk("metal_register_generic_device(): could not register shared memory device: error code %d\n", status);
	}

	printk("metal_register\n");

        status = metal_device_open("generic", SHM_DEVICE_NAME, &device);

	printk("device open %d\n", status);

        if (status != 0) {
                printk("metal_device_open failed %d\n", status);
        }

 	metal_io_init(&device->regions[0], 
		      (void *)SHM_START_ADDRESS,
		      shm_physmap, SHM_SIZE, -1, 0, NULL);
	io = metal_device_io_region(device, 0);

	printk("set io %p\n", io);

	/* setup IPM */
	ipm_handle = device_get_binding("MAILBOX_0");

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	ipm_set_enabled(ipm_handle, 1);

	/* setup vdev */
	memset(vq, 0, VRING_COUNT* sizeof(&vq[0]));
	
	vdev.role = RPMSG_REMOTE;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	rvrings[0].io = io;
	rvrings[0].num_descs = VRING_SIZE;
	rvrings[0].align = VRING_ALIGNMENT;
	rvrings[0].vq = &vq[0];

	rvrings[1].io = io;
	rvrings[1].num_descs = VRING_SIZE;
	rvrings[1].align = VRING_ALIGNMENT;
	rvrings[1].vq = &vq[1];

	vdev.vrings_info = &rvrings[0];
	vdev.index = rsc_table->vdev.notifyid;
	/* setup rvdev */
	rvdev.vdev = &vdev;

	/* wait for master ready */
	while( !(rpmsg_virtio_get_status(&rvdev) & 
		 VIRTIO_CONFIG_STATUS_DRIVER_OK));
	
	/* get the vring address set by master */
	rvrings[0].va = (void *)rsc_table->vring0.da;
	rvrings[1].va = (void *)rsc_table->vring1.da;

	printk("rpmsg_init_vdev\n");
	rpmsg_init_vdev(&rvdev, &vdev, io, (void *)SHM_START_ADDRESS, SHM_SIZE);

	printk("rpmsg_create_ept\n");
	tty_ept = rpmsg_create_ept(&rvdev, RPMSG_CHAN_NAME, RPMSG_ADDR_ANY,
				   RPMSG_ADDR_ANY, rpmsg_recv_callback, NULL);
	printk("wait receive\n");
	while(10)
	{
		msg = receive_message(&rvdev);
		send_message(msg);
	}

	metal_finish();

	printk("OpenAMP demo ended.\n");
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
