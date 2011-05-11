/*****************************************************************************\
 *  slurm_notify.c - implementation-independent notifier logging
 *  functions
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

#if HAVE_CONFIG_H
#  include "config.h"
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/macros.h"
#include "src/common/plugin.h"
#include "src/common/plugrack.h"
#include "src/common/slurm_notify.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/xmalloc.h"
#include "src/common/xassert.h"
#include "src/common/xstring.h"
#include "src/slurmctld/slurmctld.h"

/*
 * WARNING:  Do not change the order of these fields or add additional
 * fields at the beginning of the structure.  If you do, job completion
 * logging plugins will stop working.  If you need to add fields, add them
 * at the end of the structure.
 */
typedef struct slurm_notify_ops {
	int          (*log) (uint16_t state, char *payload);
} slurm_notify_ops_t;

/*
 * A global job completion context.  "Global" in the sense that there's
 * only one, with static bindings.  We don't export it.
 */
struct slurm_notify_context {
	char *			notify_type;
	plugrack_t		plugin_list;
	plugin_handle_t		cur_plugin;
	slurm_notify_ops_t	ops;
};

static slurm_notify_context_t  g_context = NULL;
static pthread_mutex_t      context_lock = PTHREAD_MUTEX_INITIALIZER;

static slurm_notify_context_t
_slurm_notify_context_create( const char *notify_type )
{
	slurm_notify_context_t c;

	if ( notify_type == NULL ) {
		debug3( "_slurm_notify_context_create: no notify type" );
		return NULL;
	}

	c = xmalloc( sizeof( struct slurm_notify_context ) );

	/* Copy the job completion job completion type. */
	c->notify_type = xstrdup( notify_type );
	if ( c->notify_type == NULL ) {
		debug3( "can't make local copy of notify type" );
		xfree( c );
		return NULL;
	}

	/* Plugin rack is demand-loaded on first reference. */
	c->plugin_list = NULL;
	c->cur_plugin = PLUGIN_INVALID_HANDLE;

	return c;
}

static int
_slurm_notify_context_destroy( slurm_notify_context_t c )
{
	int rc = SLURM_SUCCESS;
	/*
	 * Must check return code here because plugins might still
	 * be loaded and active.
	 */
	if ( c->plugin_list ) {
		if ( plugrack_destroy( c->plugin_list ) != SLURM_SUCCESS ) {
			rc = SLURM_ERROR;
		}
	} else {
		plugin_unload(c->cur_plugin);
	}

	xfree( c->notify_type );
	xfree( c );

	return rc;
}

/*
 * Resolve the operations from the plugin.
 */
static slurm_notify_ops_t *
_slurm_notify_get_ops( slurm_notify_context_t c )
{
	/*
         * These strings must be kept in the same order as the fields
         * declared for slurm_notify_ops_t.
         */
	static const char *syms[] = {
		"slurm_notify_log"
	};
        int n_syms = sizeof( syms ) / sizeof( char * );

	/* Find the correct plugin. */
        c->cur_plugin = plugin_load_and_link(c->notify_type, n_syms, syms,
					     (void **) &c->ops);
        if ( c->cur_plugin != PLUGIN_INVALID_HANDLE )
        	return &c->ops;

	if(errno != EPLUGIN_NOTFOUND) {
		error("Couldn't load specified plugin name for %s: %s",
		      c->notify_type, plugin_strerror(errno));
		return NULL;
	}

	error("Couldn't find the specified plugin name for %s "
	      "looking at all files", c->notify_type);

	/* Get the plugin list, if needed. */
        if ( c->plugin_list == NULL ) {
		char *plugin_dir;
                c->plugin_list = plugrack_create();
                if ( c->plugin_list == NULL ) {
                        error( "Unable to create a plugin manager" );
                        return NULL;
                }

                plugrack_set_major_type( c->plugin_list, "notify" );
                plugrack_set_paranoia( c->plugin_list,
				       PLUGRACK_PARANOIA_NONE,
				       0 );
		plugin_dir = slurm_get_plugin_dir();
                plugrack_read_dir( c->plugin_list, plugin_dir );
		xfree(plugin_dir);
        }

        /* Find the correct plugin. */
        c->cur_plugin =
		plugrack_use_by_type( c->plugin_list, c->notify_type );
        if ( c->cur_plugin == PLUGIN_INVALID_HANDLE ) {
                error( "can't find a plugin for type %s", c->notify_type );
                return NULL;
        }

        /* Dereference the API. */
        if ( plugin_get_syms( c->cur_plugin,
                              n_syms,
                              syms,
                              (void **) &c->ops ) < n_syms ) {
                error( "incomplete notify plugin detected" );
                return NULL;
        }

        return &c->ops;
}

extern int
notify_init( void )
{
	int retval = SLURM_SUCCESS;
	char *notify_type;

	slurm_mutex_lock( &context_lock );

	if ( g_context )
		_slurm_notify_context_destroy(g_context);

	notify_type = slurm_get_notify_type();
	g_context = _slurm_notify_context_create( notify_type );
	if ( g_context == NULL ) {
		error( "cannot create a context for %s", notify_type );
		xfree(notify_type);
		retval = SLURM_ERROR;
		goto done;
	}
	xfree(notify_type);

	if ( _slurm_notify_get_ops( g_context ) == NULL ) {
		error( "cannot resolve job completion plugin operations" );
		_slurm_notify_context_destroy( g_context );
		g_context = NULL;
		retval = SLURM_ERROR;
	}

done:
	slurm_mutex_unlock( &context_lock );
	return retval;
}

extern int
notify_fini(void)
{
	slurm_mutex_lock( &context_lock );

	if ( !g_context)
		goto done;

	_slurm_notify_context_destroy ( g_context );
	g_context = NULL;

done:
	slurm_mutex_unlock( &context_lock );
	return SLURM_SUCCESS;
}

extern int
notify_log(uint16_t state, char *payload)
{
	int retval = SLURM_SUCCESS;

	slurm_mutex_lock( &context_lock );
	if ( g_context )
		retval = (*(g_context->ops.log))(state, payload);
	else {
		error ("slurm_notify plugin context not initialized");
		retval = ENOENT;
	}
	slurm_mutex_unlock( &context_lock );
	return retval;
}
