/*-------------------------------------------------------------------------
 *
 * gtm_conn.h
 *
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Portions Copyright (c) 2010-2011 Nippon Telegraph and Telephone Corporation
 *
 * src/include/gtm/gtm_conn.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef GTM_CONN_H
#define GTM_CONN_H

#include "gtm/libpq-be.h"

struct GTM_ThreadInfo;

typedef struct GTM_ConnectionInfo
{
	/* Port contains all the vital information about this connection */
	Port					*con_port;
	struct GTM_ThreadInfo	*con_thrinfo;
	bool					con_authenticated;
} GTM_ConnectionInfo;

typedef struct GTM_Connections
{
	uint32				gc_conn_count;
	uint32				gc_array_size;
	GTM_ConnectionInfo	*gc_connections;
	GTM_RWLock			gc_lock;
} GTM_Connections;


#endif
