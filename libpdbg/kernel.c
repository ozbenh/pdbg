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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <err.h>
#include <inttypes.h>
#include <endian.h>

#include "bitutils.h"
#include "operations.h"
#include "target.h"

#define FSI_SCAN_PATH "/sys/bus/platform/devices/gpio-fsi/fsi0/rescan"
#define FSI_CFAM_PATH "/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/raw"

int fsi_fd;

static int kernel_fsi_getcfam(struct fsi *fsi, uint32_t addr64, uint32_t *value)
{
	int rc;
	uint32_t tmp, addr = (addr64 & 0x7ffc00) | ((addr64 & 0x3ff) << 2);

	rc = lseek(fsi_fd, addr, SEEK_SET);
	if (rc < 0) {
		warn("Failed to seek %s", FSI_CFAM_PATH);
		return errno;
	}

	rc = read(fsi_fd, &tmp, 4);
	if (rc < 0) {
		if ((addr64 & 0xfff) != 0xc09)
			/* We expect reads of 0xc09 to occasionally
			 * fail as the probing code uses it to see
			 * if anything is present on the link. */
			warn("Failed to read from 0x%08" PRIx32 " (%016" PRIx32 ")", (uint32_t) addr, addr64);
		return errno;
	}
	*value = be32toh(tmp);

	return 0;
}

static int kernel_fsi_putcfam(struct fsi *fsi, uint32_t addr64, uint32_t data)
{
	int rc;
	uint32_t tmp, addr = (addr64 & 0x7ffc00) | ((addr64 & 0x3ff) << 2);

	rc = lseek(fsi_fd, addr, SEEK_SET);
	if (rc < 0) {
		warn("Failed to seek %s", FSI_CFAM_PATH);
		return errno;
	}

	tmp = htobe32(data);
	rc = write(fsi_fd, &tmp, 4);
	if (rc < 0) {
		warn("Failed to write to 0x%08" PRIx32 " (%016" PRIx32 ")", addr, addr64);
		return errno;
	}

	return 0;
}

#if 0
/* TODO: At present we don't have a generic destroy method as there aren't many
 * use cases for it. So for the moment we can just let the OS close the file
 * descriptor on exit. */
static void kernel_fsi_destroy(struct pdbg_target *target)
{
	close(fsi_fd);
}
#endif

static void kernel_fsi_scan_devices(void)
{
	const char one = '1';
	int rc, fd;

	fd = open(FSI_SCAN_PATH, O_WRONLY | O_SYNC);
	if (fd < 0)
		err(errno, "Unable to open %s", FSI_SCAN_PATH);

	rc = write(fd, &one, sizeof(one));
	if (rc < 0)
		err(errno, "Unable to write to %s", FSI_SCAN_PATH);

	close(fd);
}

int kernel_fsi_probe(struct pdbg_target *target)
{
	if (!fsi_fd) {
		int tries = 5;

		while (tries) {
			/* Open first raw device */
			fsi_fd = open(FSI_CFAM_PATH, O_RDWR | O_SYNC);
			if (fsi_fd >= 0)
				return 0;
			tries--;

			/* Scan */
			kernel_fsi_scan_devices();
			sleep(1);
		}
		if (fsi_fd < 0) {
			err(errno, "Unable to open %s", FSI_CFAM_PATH);
			return -1;
		}

	}

	return -1;
}

static struct fsi kernel_fsi = {
	.target = {
		.name = "Kernel based FSI master",
		.compatible = "ibm,kernel-fsi",
		.class = "fsi",
		.probe = kernel_fsi_probe,
	},
	.read = kernel_fsi_getcfam,
	.write = kernel_fsi_putcfam,
};
DECLARE_HW_UNIT(kernel_fsi);

static int kernel_pib_getscom(struct pib *pib, uint64_t addr, uint64_t *value)
{
	int rc;

	rc = pread(pib->fd, value, 8, addr);
	if (rc < 0) {
		warn("Failed to read scom");
		return errno;
	}
	return 0;
}

static int kernel_pib_putscom(struct pib *pib, uint64_t addr, uint64_t value)
{
	int rc;

	rc = pwrite(pib->fd, &value, 8, addr);
	if (rc < 0) {
		warn("Failed to write scom");
		return errno;
	}
	return 0;
}

static int kernel_pib_probe(struct pdbg_target *target)
{
	struct pib *pib = target_to_pib(target);
	const char *scom_path;

	scom_path = dt_prop_get(target, "scom-path");
	pib->fd = open(scom_path, O_RDWR | O_SYNC);
	if (pib->fd < 0) {
		err(errno, "Unable to open %s\n", scom_path);
		return -1;
	}
	lseek(pib->fd, 0, SEEK_SET);
	return 0;

}

struct pib kernel_pib = {
	.target = {
		.name =	"Kernel based FSI SCOM",
		.compatible = "ibm,kernel-pib",
		.class = "pib",
		.probe = kernel_pib_probe,
	},
	.read = kernel_pib_getscom,
	.write = kernel_pib_putscom,
};
DECLARE_HW_UNIT(kernel_pib);

static int kernel_sbefifo_message(struct sbefifo *sf,
				  const void *in_msg, size_t in_size,
				  void *out_resp, size_t *out_resp_size)
{
	int rc;

	rc = write(sf->fd, in_msg, in_size);
	if (rc < 0) {
		warn("Failed to write message to SBE fifo");
		return -errno;
	}
	rc = read(sf->fd, out_resp, *out_resp_size);
	if (rc < 0) {
		warn("Failed to read response from SBE fifo");
		return -errno;
	}
	*out_resp_size = rc;
	return 0;
}

static int kernel_sbefifo_probe(struct pdbg_target *target)
{
	struct sbefifo *sf = target_to_sbefifo(target);

	sf->fd = open("/dev/sbefifo1", O_RDWR | O_SYNC);
	if (sf->fd < 0) {
		err(errno, "Unable to open driver\n");
		return -1;
	}
	return 0;

}

struct sbefifo kernel_sbefifo = {
	.target = {
		.name =	"Kernel based FSI SBE FIFO",
		.compatible = "ibm,kernel-sbefifo",
		.class = "sbefifo",
		.probe = kernel_sbefifo_probe,
	},
	.message = kernel_sbefifo_message,
};
DECLARE_HW_UNIT(kernel_sbefifo);
