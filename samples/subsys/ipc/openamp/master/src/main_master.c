/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ipm.h>
#include <misc/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openamp/open_amp.h>
#include <metal/device.h>

#define APP_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static struct device *ipm_handle = NULL;

#define SHM_START_ADDRESS       0x04000400
#define SHM_SIZE                0x7c00
#define SHM_DEVICE_NAME         "sramx.shm"

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

#define VRING_COUNT             2
#define VRING_RX_ADDRESS        0x04007800
#define VRING_TX_ADDRESS        0x04007C00
#define VRING_ALIGNMENT         4
#define VRING_SIZE              16

#define RSC_TABLE_ADDRESS       0x04000000


#if 0

static struct rpmsg_channel *rp_channel;
static struct rpmsg_endpoint *rp_endpoint;

static K_SEM_DEFINE(channel_created, 0, 1);

static K_SEM_DEFINE(message_received, 0, 1);
static volatile unsigned int received_data;

static struct rsc_table_info rsc_info;



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

struct lpc_resource_table *rsc_table_ptr = (void *) RSC_TABLE_ADDRESS;

#if defined(CPU_LPC54114J256BD64_cm4)
static const struct lpc_resource_table rsc_table = {
	.ver = 1,
	.num = 2,
	.offset = {
		offsetof(struct lpc_resource_table, mem),
		offsetof(struct lpc_resource_table, vdev),
	},
	.mem = { RSC_RPROC_MEM, SHM_START_ADDRESS, SHM_START_ADDRESS, SHM_SIZE, 0 },
	.vdev = { RSC_VDEV, VIRTIO_ID_RPMSG, 0, 1 << VIRTIO_RPMSG_F_NS, 0, 0, 0, VRING_COUNT, { 0, 0 } },
	.vring0 = { VRING_TX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 1, 0 },
	.vring1 = { VRING_RX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 2, 0 },
};
#endif

void resource_table_init(void **table_ptr, int *length)
{
#if defined(CPU_LPC54114J256BD64_cm4)
	/* Master: copy the resource table to shared memory. */
	memcpy(rsc_table_ptr, &rsc_table, sizeof(struct lpc_resource_table));
#endif

	*length = sizeof(struct lpc_resource_table);
	*table_ptr = rsc_table_ptr;
}

static void rpmsg_recv_callback(struct rpmsg_channel *channel, void *data,
				int data_length, void *private, unsigned long src)
{
	received_data = *((unsigned int *) data);
	k_sem_give(&message_received);
}

static void rpmsg_channel_created(struct rpmsg_channel *channel)
{
	rp_channel = channel;
	rp_endpoint = rpmsg_create_ept(rp_channel, rpmsg_recv_callback, RPMSG_NULL, RPMSG_ADDR_ANY);
	k_sem_give(&channel_created);
}

static void rpmsg_channel_deleted(struct rpmsg_channel *channel)
{
	rpmsg_destroy_ept(rp_endpoint);
}

static unsigned int receive_message(void)
{
	while (k_sem_take(&message_received, K_NO_WAIT) != 0) {
		;
		hil_poll(proc, 0);
	}
	return received_data;
}

static int send_message(unsigned int message)
{
	return rpmsg_send(rp_channel, &message, sizeof(message));
}
#endif


static struct virtio_vring_info rvrings[2] = {
	[0] = {
		.align = VRING_ALIGNMENT,
	},
	[1] = {
		.align = VRING_ALIGNMENT,
	},
};
static struct virtio_device vdev;
static struct rpmsg_virtio_device rvdev;
static struct metal_io_region *io;
static struct virtqueue * vq[2];

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return VIRTIO_CONFIG_STATUS_DRIVER_OK;
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	return ;
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	printk("in virtio_get_features\n");
	return 1 << VIRTIO_RPMSG_F_NS;
}

static void virtio_set_features(struct virtio_device *vdev,
                                      uint32_t features)
{
	return ;
}

static void virtio_notify(struct virtqueue *vq)
{
	uint32_t dummy_data = 0x12345678; /* Some data must be provided */
	printk("virtio_notify for vq %p\n", vq);

	printk(" NAME %s\n", vq->vq_name);

	ipm_send(ipm_handle, 0, 0, &dummy_data, sizeof(dummy_data));
}

virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

static void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	printk("platform_ipm_callback\n");
	virtqueue_notification(vq[0]);
	virtqueue_notification(vq[1]);
}

#if 0
static int enable_interrupt(struct proc_intr *intr)
{
	return ipm_set_enabled(ipm_handle, 1);
}
#endif

void endpoint_cb(struct rpmsg_endpoint *ept, void *data,
                             size_t len, uint32_t src, void *priv)
{
	printk("in endpoint_cb\n");
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int status = 0;
	struct metal_device *device;
	struct rpmsg_endpoint *ep;
	unsigned int *p = (void *)RSC_TABLE_ADDRESS;

	printk("\r\nOpenAMP demo started\r\n");

	/* Make sure resource table is setup by slave - HACK */
	while (*p != 1) {
		;
	}
	printk("%p %x %x\n", p, *p, *(p+1));

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
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

        io = metal_device_io_region(device, 0);

	printk("set io %p\n", io);

	/* setup IPM */
	ipm_handle = device_get_binding("MAILBOX_0");

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	ipm_set_enabled(ipm_handle, 1);

	/* setup vdev */
	vq[0] = virtqueue_allocate(VRING_SIZE, true);
	vq[1] = virtqueue_allocate(VRING_SIZE, true);

	vdev.role = RPMSG_MASTER;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	rvrings[0].io = io;
	rvrings[0].va = (void *)VRING_TX_ADDRESS;
	rvrings[0].num_descs = VRING_SIZE;
	rvrings[0].align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].va = (void *)VRING_RX_ADDRESS;
	rvrings[1].num_descs = VRING_SIZE;
	rvrings[1].align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.vrings_info = &rvrings[0];

	/* setup rvdev */
	rvdev.vdev = &vdev;

	rpmsg_init_vdev(&rvdev, &vdev, io, (void *)SHM_START_ADDRESS, SHM_SIZE);
	printk("rpmsg_init_vdev DONE\n");

	ep = rpmsg_create_ept(&rvdev, "kumar", 3, 4, &endpoint_cb, NULL);

	printk("created ep %p\n", ep);

	int data = 0xdeadbeef;
	status = rpmsg_send(ep, &data, sizeof(data));

	printk("sent data status %d\n", status);

	metal_finish();

	printk("OpenAMP demo ended.\n");
}

void main(void)
{
	printk("Starting application thread!\n");
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
