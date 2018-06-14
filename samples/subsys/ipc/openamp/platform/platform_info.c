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

#if 1
#define DEBUG_TRACE printk
#else
#define DEBUG_TRACE(...)
#endif

static K_SEM_DEFINE(data_sem, 0, 1);
static unsigned int vring_notifyid;
static int rsc_size;
static struct st_resource_table *rsc_table;
static struct metal_io_region *shm_io;
static struct metal_io_region *rsc_io;
static struct device *ipm_handle = NULL;
static struct rpmsg_virtio_shm_pool shpool;
static struct platform_info platform_data;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDRESS };
struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.num_regions = 2,
	.regions = {
		{.virt = NULL}, /* shared memory */
		{.virt = NULL}, /* rsc_table memory */
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	vring_notifyid |=  1 << id;
	if (k_sem_count_get(&data_sem) < 2)
		k_sem_give(&data_sem);
}

static int platform_notify(void *priv, uint32_t id)
{
	uint32_t dummy_data = 0x12345678; /* Some data must be provided */
	(void)priv;

	ipm_send(ipm_handle, 0, id, &dummy_data,
		 sizeof(dummy_data));
	return 0;
}

int platform_poll(struct virtio_device *vdev)
{
	int ret;
	unsigned int id_mask;

	ret = k_sem_take(&data_sem, K_FOREVER);
	if (!ret && vring_notifyid) {
		id_mask = vring_notifyid;
		vring_notifyid = 0;

		if (id_mask & (1 << VRING_TX_ID)) {
			rproc_virtio_notified(vdev, VRING_TX_ID);
		}
		if (id_mask &  (1 << VRING_RX_ID)) {
			rproc_virtio_notified(vdev, VRING_RX_ID);
		}
	}

	return ret;
}

/* Main hw machinery initialization entry point, called from main()*/
/* return 0 on success */
int init_system(void)
{
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	/* Low level abstraction layer for openamp initialization */
	metal_init(&metal_params);

	/* setup IPM */
	ipm_handle = device_get_binding("MAILBOX_0");

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	ipm_set_enabled(ipm_handle, 1);

	return 0;
}

struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_unbound_service_cb ns_cb)
{
	struct fw_rsc_vdev_vring *vring_rsc;
	struct platform_info *pdata = platform;
	struct rpmsg_virtio_device *rvdev = &pdata->rvdev;
	struct virtio_device *vdev;
	int ret;

	vdev = rproc_virtio_create_vdev(role, VDEV_ID, &rsc_table->vdev,
					rsc_io, NULL, platform_notify, NULL);

	if (!vdev) {
		DEBUG_TRACE("failed to create vdev\r\n");
		return NULL;
	}

	vring_rsc = &rsc_table->vring0;
	ret = rproc_virtio_init_vring(vdev, 0, vring_rsc->notifyid,
				      (void *)vring_rsc->da,
				      rsc_io, vring_rsc->num, vring_rsc->align);
	if (ret) {
		DEBUG_TRACE("failed to init vring 0\r\n");
		rproc_virtio_remove_vdev(vdev);
	}

	vring_rsc = &rsc_table->vring1;
	ret = rproc_virtio_init_vring(vdev, 1, vring_rsc->notifyid,
				      (void *)vring_rsc->da,
				      rsc_io, vring_rsc->num, vring_rsc->align);
	if (ret) {
		DEBUG_TRACE("failed to init vring 1\r\n");
		rproc_virtio_remove_vdev(vdev);
	}

	rpmsg_virtio_init_shm_pool(&shpool, (void *)SHM_START_ADDRESS,
				   (size_t)SHM_SIZE);
	ret =  rpmsg_init_vdev(rvdev, vdev, ns_cb, shm_io, &shpool);

	if (ret) {
		DEBUG_TRACE("failed rpmsg_init_vdev\r\n");
		rproc_virtio_remove_vdev(vdev);
	}

	return rpmsg_virtio_get_rpmsg_device(rvdev);
}

int platform_init(int argc, char *argv[], void **platform)
{
	void *rsc_tab_addr;
	struct metal_device *device;
	int ret;

	init_system();

	resource_table_init(&rsc_tab_addr, &rsc_size);
	rsc_table = (struct st_resource_table *)rsc_tab_addr;

	ret = metal_register_generic_device(&shm_device);
	if (ret != 0) {
		DEBUG_TRACE(" could not register shared memory device: %d\n",
		       ret);
		return -1;
	}

	ret = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (ret != 0) {
		DEBUG_TRACE("metal_device_open failed %d\n", ret);
		return -1;
	}

	metal_io_init(&device->regions[0],
		      (void *)SHM_START_ADDRESS,
		      shm_physmap, SHM_SIZE, -1, 0, NULL);

	shm_io = metal_device_io_region(device, 0);

	DEBUG_TRACE("set shm_io %p\n", shm_io);

	metal_io_init(&device->regions[RSC_IO_REGION_ID], rsc_table,
		      (metal_phys_addr_t *)rsc_table, rsc_size, -1, 0, NULL);

	rsc_io = metal_device_io_region(device, 1);

	*platform = &platform_data;

	return 0;
}
