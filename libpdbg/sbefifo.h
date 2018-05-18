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
#ifndef __SBEFIFO_H
#define __SBEFIFO_H

#include <stdint.h>
#include "compiler.h"
#include "device.h"
#include "libpdbg.h"

int sbefifo_initialize(void);
int sbefifo_submit(const void *msg, size_t msg_size,
		   void *resp, size_t *resp_size);
int sbefifo_getmem(bool use_pba, u64 addr, size_t len, void *out_data);

#endif /* __SBEFIFO_H */
