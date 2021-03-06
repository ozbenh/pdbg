/* Copyright 2017 IBM Corp.
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
#include <inttypes.h>

#include <target.h>

enum backend { FSI, I2C, KERNEL, FAKE, HOST };

static inline bool target_is_disabled(struct pdbg_target *target)
{
	return pdbg_target_status(target) == PDBG_TARGET_DISABLED ||
		pdbg_target_status(target) == PDBG_TARGET_NONEXISTENT;
}

void target_select(struct pdbg_target *target);
void target_unselect(struct pdbg_target *target);
bool target_selected(struct pdbg_target *target);

/* Returns the sum of return codes. This can be used to count how many targets the callback was run on. */
int for_each_child_target(char *class, struct pdbg_target *parent,
				 int (*cb)(struct pdbg_target *, uint32_t, uint64_t *, uint64_t *),
				 uint64_t *arg1, uint64_t *arg2);

int for_each_target(char *class, int (*cb)(struct pdbg_target *, uint32_t, uint64_t *, uint64_t *), uint64_t *arg1, uint64_t *arg2);
void for_each_target_release(char *class);
