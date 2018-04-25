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

#include <openamp/open_amp.h>
#include "platform.h"
#include "resource_table.h"

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
static struct rpmsg_virtio_device *rvdev;

extern int platform_kick(struct remoteproc *rproc, uint32_t id);
extern int platform_poll(struct virtio_device *vdev);

static void rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data,
				size_t len, uint32_t src, void *priv)
{
	printk("%s:\n", __func__);
	rcv_ept = ept;
	memcpy(rcv_data, data, len);
	rcv_len = len;
	k_sem_give(&message_rcv);
}

static char *receive_message(struct rpmsg_virtio_device *rvdev)
{
	DEBUG_TRACE("%s:\n", __func__);
	while (k_sem_take(&message_rcv, K_NO_WAIT) != 0)
		platform_poll(rvdev->vdev);
	return rcv_data;
}

static int send_message(char *message)
{
	DEBUG_TRACE("%s:\n", __func__);
	return rpmsg_send(rcv_ept, message, rcv_len);
}

struct remoteproc_ops rproc_ops = {
	.notify = platform_kick,
};

struct remoteproc rproc = {
	.ops = &rproc_ops,
};


void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	unsigned char *msg;

	struct remoteproc *rproc;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	
	metal_init(&metal_params);
	
	rproc = platform_create_proc(0);
	
	rvdev = platform_create_rpmsg_vdev(rproc, 0, VIRTIO_DEV_SLAVE, NULL);
	if(!rvdev) {
		printk("platform_create_rpmsg_vdev error !\n");
		goto err_rvdev;
	}
	printk("rpmsg_create_ept\n");
	tty_ept = rpmsg_create_ept(rvdev, RPMSG_CHAN_NAME, RPMSG_ADDR_ANY,
				   RPMSG_ADDR_ANY, rpmsg_recv_callback, NULL);
	printk("wait receive\n");
	while(10)
	{
		msg = receive_message(rvdev);
		send_message(msg);
	}

err_rvdev:
	
	metal_finish();
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
