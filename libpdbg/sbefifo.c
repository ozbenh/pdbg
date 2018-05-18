/* Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "operations.h"
#include "bitutils.h"
#include "target.h"

static struct sbefifo *sbe_fifo;

int sbefifo_initialize(void)
{
	struct pdbg_target *fifo_target;

	if (sbe_fifo)
		return 0;
	pdbg_for_each_class_target("sbefifo", fifo_target) {
		if (pdbg_target_probe(fifo_target) != PDBG_TARGET_ENABLED)
			continue;
		sbe_fifo = target_to_sbefifo(fifo_target);
		return 0;
	}
	return -ENOENT;
}

int sbefifo_submit(const void *msg, size_t msg_size,
		   void *resp, size_t *resp_size)
{
	u32 *tmp_resp, dh;
	size_t tmp_size;
	int rc;

	tmp_size = *resp_size + 0x2000;
	tmp_resp = malloc(tmp_size);
	memset(tmp_resp, 0xff, tmp_size);
	//	printf("tmp_size = %d\n", tmp_size);
	rc = sbe_fifo->message(sbe_fifo, msg, msg_size, tmp_resp, &tmp_size);
	if (rc)
		return rc;
	if (tmp_size & 3) {
		printf("Odd SBE response size: %d\n", tmp_size);
		return -EIO;
	}
	tmp_size >>= 2;
	dh = ntohl(tmp_resp[tmp_size - 1]);
	(void)dh;
	// XXX check
	memcpy(resp, tmp_resp, *resp_size);
	return 0;
}

int sbefifo_getmem(bool use_pba, u64 addr, size_t len, void *out_data)
{
	int align;
	u32 msg[6];
	size_t data_len;
	int rc;

	align = use_pba ? 128 : 8;
	align--;
	if ((addr | len) & align) {
		printf("%s access must be aligned to %d bytes\n",
		       use_pba ? "PBA" : "ADU", align + 1);
		return -EINVAL;
	}
	msg[0] = htonl(6);
	msg[1] = htonl(0xa401);
	msg[2] = htonl(use_pba ? 0x0002 : 0x0001); // 0x20 FAST MODE ?
	msg[3] = htonl(addr >> 32);
	msg[4] = htonl(addr & 0xffffffff);
	msg[5] = htonl(len);
	data_len = len;
	rc = sbefifo_submit(msg, sizeof(msg), out_data, &data_len);
	if (rc)
		return rc;
	return data_len;
}
