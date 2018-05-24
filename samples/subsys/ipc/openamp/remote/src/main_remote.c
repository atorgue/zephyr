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

static char rcv_msg[256];
static volatile unsigned int rcv_len;
static struct rpmsg_endpoint tty_ept;
static struct rpmsg_endpoint *rcv_ept;

static struct virtqueue vq[2];
static struct virtio_device *vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *shm_io;
static struct metal_io_region *rsc_io;
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
	memcpy(rcv_msg, data, len);
	rcv_len = len;
	k_sem_give(&message_rcv);
}

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDRESS };
struct metal_device shm_device = {
 	.name = SHM_DEVICE_NAME,
 	.bus = "generic",
 	.num_regions = 2,
 	.regions = {
 		{.virt = NULL}, /* shared memory */
		{.virt = NULL}, /* rsc_table memory */
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
	return rcv_msg;
}

static int send_message(char *message)
{
	printk("%s:\n", __func__);
	return rpmsg_send(rcv_ept, message, rcv_len);
}

static int virtio_notify(void *priv, uint32_t id)
{
	uint32_t dummy_data = 0x12345678; /* Some data must be provided */
	(void)priv;
	
	ipm_send(ipm_handle, 0, id, &dummy_data,
		 sizeof(dummy_data));
	return 0;
}
static void new_endpoint_cb(struct rpmsg_device *rdev, const char *name,
			    uint32_t src)
{
	printk("%s: unexpected ns service receive for name %s\n",__func__, name);
#if 0
	rpmsg_create_ept(&tty_ept2, &rvdev.rdev, name, 
				   RPMSG_ADDR_ANY, src,
			           rpmsg_recv_callback, NULL);
#endif
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	struct metal_device *device;
	struct fw_rsc_vdev_vring *vring_rsc;
	void* rsc_tab_addr;
	unsigned char *msg;
//	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	
	printk("\r\nOpenAMP demo started\r\n");

	resource_table_init(&rsc_tab_addr, &rsc_size);
	rsc_table = (struct st_resource_table *)rsc_tab_addr;
	
//	metal_init(&metal_params);
	metal_init();

	status = metal_register_generic_device(&shm_device);
	if (status != 0) {
		printk("metal_register_generic_device(): could not register shared memory device: error code %d\n", status);
	}

        status = metal_device_open("generic", SHM_DEVICE_NAME, &device);
        if (status != 0) {
                printk("metal_device_open failed %d\n", status);
        }

 	metal_io_init(&device->regions[0], 
		      (void *)SHM_START_ADDRESS,
		      shm_physmap, SHM_SIZE, -1, 0, NULL);

	shm_io = metal_device_io_region(device, 0);

	printk("set shm_io %p\n", shm_io);

	metal_io_init(&device->regions[RSC_IO_REGION_ID], rsc_table,
		      (metal_phys_addr_t *)rsc_table, rsc_size, -1, 0, NULL);
	
	rsc_io = metal_device_io_region(device, 1);

	printk("set rsc_io %p\n", rsc_io);

	/* setup IPM */
	ipm_handle = device_get_binding("MAILBOX_0");

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	ipm_set_enabled(ipm_handle, 1);

	/* setup vdev */
	printk("rproc_virtio_create_vdev\n");

	vdev = rproc_virtio_create_vdev(RPMSG_REMOTE, VDEV_ID, &rsc_table->vdev,
					rsc_io, NULL, virtio_notify, NULL);

	vring_rsc = &rsc_table->vring0;
	status = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid,
					      (void *)vring_rsc->da,
					      rsc_io, vring_rsc->num, 
						vring_rsc->align);
	vring_rsc = &rsc_table->vring1;
	status = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid,
					     (void *)vring_rsc->da,
					      rsc_io, vring_rsc->num, 
						vring_rsc->align);
	printk("rpmsg_init_vdev\n");
	rpmsg_init_vdev(&rvdev, vdev, &new_endpoint_cb,
			shm_io, (void *)SHM_START_ADDRESS, SHM_SIZE);

	k_sleep(500);
	printk("rpmsg_create_ept\n");
	status = rpmsg_create_ept(&tty_ept, &rvdev.rdev, RPMSG_CHAN_NAME, 
				   RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			           rpmsg_recv_callback, NULL);
	if( status != 0 )
		printk("error while creating endpoint(%x)\n", status);
		
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
