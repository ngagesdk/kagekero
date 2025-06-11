/** @file pfs.h
 *
 *  Packed file system implementation.
 *  Adapted version, originally from The Mistral Report:
 *  https://montyontherun.itch.io/the-mistral-report
 *
 *  Copyright (c) 2019, Daniel Monteiro. All rights reserved.
 *  SPDX-License-Identifier: BSD-2-Clause
 *
 **/

#ifndef PFS_H
#define PFS_H

#include <stdint.h>

void init_file_reader(void);
size_t size_of_file(const char *path);
uint8_t *load_binary_file_from_path(const char *path);

#endif // PFS_H
