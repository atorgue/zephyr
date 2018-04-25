/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_H__
#define PLATFORM_H__

#include <openamp/open_amp.h>

#if defined(CPU_LPC54114J256BD64_cm4)
#define SHM_START_ADDRESS       0x04000400
#define SHM_SIZE                0x7c00
#define SHM_DEVICE_NAME         "sramx.shm"


#define VRING_COUNT             2
#define VRING_RX_ADDRESS        0x04000400
#define VRING_TX_ADDRESS        0x04000800
#define VRING_ALIGNMENT         4
#define VRING_SIZE              32

#elif defined(CONFIG_BOARD_ST)

#define SHM_START_ADDRESS       0x10020000
#define SHM_SIZE                0x40000
#define SHM_DEVICE_NAME         "mcusram.shm"

#define VRING_COUNT             2
#define VRING_RX_ADDRESS        0XDEADBEAF
#define VRING_TX_ADDRESS        -1
#define VRING_ALIGNMENT         0x1000
#define VRING_SIZE              32

#define SHM_IO_REGION_ID 	0
#define RSC_IO_REGION_ID 	1

#endif

struct remoteproc *platform_create_proc(int role);

struct  rpmsg_virtio_device *
platform_create_rpmsg_vdev(struct remoteproc *rproc, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev));

#endif

