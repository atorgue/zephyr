/*
 * Copyright (c) 2018, STMICROLECTRONICS
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
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

#define LPRINTF(format, ...) DEBUG_TRACE(format, ##__VA_ARGS__)
#define LPERROR(format, ...) LPRINTF("ERROR: " format, ##__VA_ARGS__)

#define RPMSG_CHAN_NAME "rpmsg-client-sample"

#define APP_TASK_STACK_SIZE (256)

K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);

static struct k_thread thread_data;

static K_SEM_DEFINE(message_rcv, 0, 1);

static char rcv_msg[256];
static volatile unsigned int rcv_len;
static struct rpmsg_endpoint local_ept;
static struct rpmsg_endpoint *rcv_ept;

static int rpmsg_recv_callback(struct rpmsg_endpoint *ept, void *data,
			       size_t len, uint32_t src, void *priv)
{
	LPRINTF("%s:\n", __func__);
	rcv_ept = ept;
	memcpy(rcv_msg, data, len);
	rcv_len = len;
	k_sem_give(&message_rcv);

	return 0;
}

static char *receive_message(struct virtio_device *vdev)
{
	LPRINTF("%s:\n", __func__);
	while (k_sem_take(&message_rcv, K_NO_WAIT) != 0)
		platform_poll(vdev);
	return rcv_msg;
}

static int send_message(char *message, unsigned int len)
{
	LPRINTF("%s:\n", __func__);
	return rpmsg_send(rcv_ept, message, len);
}

static void new_service_cb(struct rpmsg_device *rdev, const char *name,
			   uint32_t src)
{
	LPERROR("%s: unexpected ns service receive for name %s\n",
		__func__, name);
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	void *platform;
	int ret = 0;
	unsigned char *msg;
	unsigned char s_msg[32];
	uint32_t count = 0;
	struct rpmsg_device *rpdev;
	struct platform_info *pdata;

	LPRINTF("\r\nOpenAMP demo started\r\n");

	/* Initialize platform */
	ret = platform_init(0, NULL, &platform);
	if (ret) {
		LPERROR("Failed to initialize platform.\n");
		ret = -1;
		goto task_end;
	}
	pdata = (struct platform_info *)platform;

	rpdev = platform_create_rpmsg_vdev(platform, 0,
					   VIRTIO_DEV_SLAVE, NULL,
					   new_service_cb);
	if (!rpdev) {
		LPERROR("Failed to create rpmsg virtio device.\n");
		ret = -1;
		goto task_end;
	}

	LPRINTF("rpmsg_create_ept\n");
	ret = rpmsg_create_ept(&local_ept, rpdev, RPMSG_CHAN_NAME,
			       RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			       rpmsg_recv_callback, NULL);
	if (ret != 0)
		LPERROR("error while creating endpoint(%d)\n", ret);

	LPRINTF("wait receive\n");
	while (1) {
		msg = receive_message(pdata->rvdev.vdev);
		LPRINTF("message %d: %s\n", count, msg);

		if (++count < 100) {
			sprintf(s_msg, "hello world! %02d", count);
			send_message(s_msg, strlen(s_msg) + 1);
		} else {
			sprintf(s_msg, "goodbye!");
			send_message(s_msg, strlen(s_msg) + 1);
			break;
		}
	}

task_end:
	metal_finish();

	LPRINTF("OpenAMP demo ended.\n");
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
