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

#if 0
#define DEBUG_TRACE(...) printkprintk(...)
#else
#define DEBUG_TRACE(...)
#endif

#define RPMSG_CHAN_NAME              "rpmsg-tty-channel"

#define APP_TASK_STACK_SIZE (256)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static K_SEM_DEFINE(channel_created, 0, 1);

static K_SEM_DEFINE(message_rcv, 0, 1);
static char rcv_data[256];
static volatile unsigned int rcv_data_length;

static struct rsc_table_info rsc_info;
static struct hil_proc *proc;

static struct rpmsg_channel *rp_channel;
static struct rpmsg_endpoint *rp_endpoint;

static void rpmsg_recv_callback(struct rpmsg_channel *channel, void *data,
				int data_length, void *private, unsigned long src)
{
	DEBUG_TRACE("%s:\n", __func__);
	rp_channel = channel;
	memcpy(rcv_data, data, data_length);
	rcv_data_length = data_length;
	k_sem_give(&message_rcv);
}

static void rpmsg_channel_created(struct rpmsg_channel *channel)
{
	DEBUG_TRACE("%s:\n", __func__);
	rp_channel = channel;
	rp_endpoint = rpmsg_create_ept(rp_channel, rpmsg_recv_callback, RPMSG_NULL, RPMSG_ADDR_ANY);
	k_sem_give(&channel_created);
}

static void rpmsg_channel_deleted(struct rpmsg_channel *channel)
{
	rpmsg_destroy_ept(rp_endpoint);
}

static char *receive_message(void)
{
	DEBUG_TRACE("%s:\n", __func__);
	while (k_sem_take(&message_rcv, K_NO_WAIT) != 0)
		hil_poll(proc, 0);
	return rcv_data;
}

static int send_message(char *message)
{
	DEBUG_TRACE("%s:\n", __func__);
	return rpmsg_send(rp_channel, message, rcv_data_length);
}

inline int VIRT_UART_Init(struct remote_proc *rproc)
{
  /*
   * Create a rpmsg channel
   */

  return 0;
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	int status;
	
	resource_table_init((void **) &rsc_info.rsc_tab, &rsc_info.size);
	metal_init(&metal_params);

	proc = platform_init(RPMSG_MASTER);
	if (proc == NULL) {
		goto _cleanup;
	}

	resource_table_init((void **) &rsc_info.rsc_tab, &rsc_info.size);
	struct remote_proc *rproc_ptr = NULL;
	status = remoteproc_resource_init(&rsc_info, proc,
					      rpmsg_channel_created, rpmsg_channel_deleted,
					      rpmsg_recv_callback, &rproc_ptr, RPMSG_REMOTE);
	if (status != 0) {
		goto _cleanup;
	}

	rp_channel = rpmsg_create_channel(rproc_ptr->rdev, RPMSG_CHAN_NAME);
	if(!rp_channel)
		goto _cleanup;
	
	while (k_sem_take(&channel_created, K_NO_WAIT) != 0)
		hil_poll(proc, 0);

	char *message = NULL;
	while (strcmp(message, "stop")) {
		message = receive_message();
		status = send_message(message);
		if (status <= 0) {
			DEBUG_TRACE("error while sending message\n");
			goto _cleanup;
		}
	}

_cleanup:
	if (rproc_ptr) {
		remoteproc_resource_deinit(rproc_ptr);
	}
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
