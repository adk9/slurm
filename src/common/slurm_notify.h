/*****************************************************************************\
 *  slurm_notify.h - implementation-independent notifier logging
 *  API definitions
 *****************************************************************************
 *  Copyright (C) 2011 	    The Trustees of Indiana University and Indiana
 *			    University Research and Technology.
 *			    Corporation.  All rights reserved.
 *  Written by Abhishek Kulkarni <adkulkar@osl.iu.edu>
 *  All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <https://computing.llnl.gov/linux/slurm/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef __SLURM_NOTIFY_H__
#define __SLURM_NOTIFY_H__

#if HAVE_CONFIG_H
#  include "config.h"
#endif
#if HAVE_STDINT_H
#  include <stdint.h>           /* for uint16_t, uint32_t definitions */
#endif
#if HAVE_INTTYPES_H
#  include <inttypes.h>         /* for uint16_t, uint32_t definitions */
#endif
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "src/slurmctld/slurmctld.h"

enum ftb_states {
	MON_NODES_UNREACHABLE,
	MON_NODES_ALIVE,
	RM_NODES_ADDED,
	RM_NODES_REMOVED
};

/* global notifier context */
typedef struct slurm_notify_context *slurm_notify_context_t;

/* initialization of the notifier */
extern int notify_init(void);

/* terminate pthreads and free, general clean-up for termination */
extern int notify_fini(void);

/* notify a node's state over the notifier  */
extern int notify_log(uint16_t state, void *arg);

#endif /*__SLURM_NOTIFY_H__*/
