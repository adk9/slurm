/*****************************************************************************\
 *  notify_ftb.c - FTB notifier plugin.
 *****************************************************************************
 *  Copyright (C) 2004-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if HAVE_STDINT_H
#  include <stdint.h>
#endif
#if HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#ifdef HAVE_FTB
#  include <ftb.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "slurm/slurm.h"
#include "slurm/slurm_errno.h"

#include "src/common/slurm_notify.h"
#include "src/slurmctld/slurmctld.h"

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  SLURM uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "notify" for SLURM notify) and <method>
 * is a description of how this plugin satisfies that application.  SLURM will
 * only load notify plugins if the plugin_type string has a
 * prefix of "notify/".
 *
 * plugin_version - an unsigned 32-bit integer giving the version number
 * of the plugin.  If major and minor revisions are desired, the major
 * version number may be multiplied by a suitable magnitude constant such
 * as 100 or 1000.  Various SLURM versions will likely require a certain
 * minimum version for their plugins as the notify API matures.
 */
const char plugin_name[]       	= "Notify FTB plugin";
const char plugin_type[]       	= "notify/ftb";
const uint32_t plugin_version	= 100;

const FTB_event_info_t ftb_events[] = {
	{"MON_NODES_UNREACHABLE","ERROR"},
	{"MON_NODES_ALIVE", 	 "INFO"},
	{"MON_NODES_IDLE",       "INFO"},
	{"RM_NODES_REMOVED",     "INFO"},
	{"MON_NODES_POWER_SAVE", "INFO"},
};

static FTB_client_handle_t chandle;
static FTB_client_t client;

/*
 * init() is called when the plugin is loaded, before any other functions
 * are called.  Put global initialization here.
 */
extern int init ( void )
{
	int ret = 0;
	strcpy(client.event_space, "FTB.SLURM");
	strcpy(client.client_subscription_style, "FTB_SUBSCRIPTION_NONE");

	ret = FTB_Connect(&client, &chandle);
	if (ret != FTB_SUCCESS)
		return error("FTB_Connect() failed (error %d)", ret);

	ret = FTB_Declare_publishable_events(chandle, 0, ftb_events, 1);
	if (ret != FTB_SUCCESS)
		return error("FTB_Declare_publishable_events() failed (error %d)", ret);

	return SLURM_SUCCESS;
}

extern int fini ( void )
{
	return FTB_Disconnect(chandle);
}

static char *node_event (uint16_t state)
{
	switch(state) {
	case NODE_STATE_DOWN:
	case NODE_STATE_FAIL:
		return ftb_events[0].event_name;
	case NODE_STATE_POWER_UP:
	case NODE_RESUME:
		return ftb_events[1].event_name;
	case NODE_STATE_IDLE:
		return ftb_events[2].event_name;
	case NODE_STATE_ALLOCATED:
		return ftb_events[3].event_name;
	case NODE_STATE_POWER_SAVE:
		return ftb_events[4].event_name;
	default:
		return NULL;
	}
}

/*
 * The remainder of this file implements the standard SLURM notify API.
 */
extern int slurm_notify_log (uint16_t state, char *payload)
{
	int rc;
	FTB_event_handle_t evt;
	FTB_event_properties_t prop;
	char *event;

	prop.event_type = 1;
	snprintf(prop.event_payload, FTB_MAX_PAYLOAD_DATA, "%s",
		 (payload != NULL) ? payload : "");
	event = node_event(state);
	if (event) {
		rc = FTB_Publish(chandle, node_event(state), &prop, &evt);
		if (rc != FTB_SUCCESS)
			return error("FTB_Publish failed (error %d)", rc);
	}

	return SLURM_SUCCESS;
}
