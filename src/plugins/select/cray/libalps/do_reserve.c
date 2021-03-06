/*
 * Implements the Basil RESERVE method for creating partitions.
 *
 * Copyright (c) 2009-2011 Centro Svizzero di Calcolo Scientifico (CSCS)
 * Licensed under the GPLv2.
 */
#include "../basil_alps.h"

/* Internal helper functions */
extern void rsvn_free_param(struct basil_rsvn_param *p);
extern void free_rsvn(struct basil_reservation *r);

/**
 * rsvn_add_mem_param  -  Add memory allocation request to reservation.
 * @rp:     reservation to add to
 * @mem_mb: memory size in MB requested for @rp
 */
static int rsvn_add_mem_param(struct basil_rsvn_param *rp, uint32_t mem_mb)
{
	struct basil_memory_param *mp;

	if (mem_mb == 0)	/* 0 means 'use defaults' */
		return 0;

	mp = calloc(1, sizeof(*mp));
	if (mp == NULL)
		return -1;

	/* As of Basil 1.2/3.1, BMT_OS is still the only supported type. */
	mp->type    = BMT_OS;
	mp->size_mb = mem_mb;

	if (rp->memory)
		mp->next = rp->memory;
	rp->memory = mp;
	return 0;
}

/**
 * rsvn_add_params  -  Populate parameters for a RESERVE request
 * @resv:	the reservation to add to
 * @width:	mppwidth > 0
 * @depth:	mppdepth >= 0 (0 meaning 'use defaults')
 * @nppn:	mppnppn  >= 0 (0 meaning 'use defaults')
 * @mem_mb:	mppmem   >= 0 (0 meaning 'use defaults', else size in MB)
 * @mppnodes:	comma-separated nodelist (will be freed if not NULL)
 * @accel:	accelerator parameters (will be freed if not NULL)
 */
static int rsvn_add_params(struct basil_reservation *resv,
			   uint32_t width, uint32_t depth, uint32_t nppn,
			   uint32_t mem_mb, char *mppnodes,
			   struct basil_accel_param *accel)
{
	struct basil_rsvn_param *rp = calloc(1, sizeof(*rp));

	if (rp == NULL)
		return -1;

	rp->arch  = BNA_XT;	/* "XT" is the only supported architecture */
	rp->width = width;
	rp->depth = depth;
	rp->nppn  = nppn;
	rp->nodes = mppnodes;
	rp->accel = accel;

	if (mem_mb && rsvn_add_mem_param(rp, mem_mb) < 0) {
		rsvn_free_param(rp);
		return -1;
	}

	if (resv->params)
		rp->next = resv->params;
	resv->params = rp;

	return 0;
}

static struct basil_reservation *alloc_rsvn(uint32_t rsvn_id)
{
	struct basil_reservation *new = calloc(1, sizeof(*new));

	if (new != NULL)
		new->rsvn_id = rsvn_id;
	return new;
}

/**
 * rsvn_new  -  allocate new reservation with single 'ReserveParam' element
 * @user:	owner (user_name) of the reservation (mandatory)
 * @batch_id:	batch job ID associated with reservation or NULL (Basil 1.1 only)
 *
 * @width:	mppwidth > 0
 * @depth:	mppdepth >= 0 (0 meaning 'use default')
 * @nppn:	mppnppn  >= 0 (0 meaning 'use default')
 * @mem_mb:	mppmem   >= 0 (0 meaning 'use defaults', else size in MB)
 * @mppnodes:	comma-separated nodelist (will be freed if not NULL)
 * @accel:	accelerator parameters or NULL
 *
 * The reservation ID is initially 0, since 0 is an invalid reservation ID.
 */
static struct basil_reservation *rsvn_new(const char *user, const char *batch_id,
					  uint32_t width, uint32_t depth,
					  uint32_t nppn, uint32_t mem_mb,
					  char *mppnodes,
					  struct basil_accel_param *accel)
{
	struct basil_reservation *res;

	assert(user != NULL && *user != '\0');

	if (width <= 0 || depth < 0 || nppn < 0)
		return NULL;

	res = alloc_rsvn(0);
	if (res == NULL)
		return NULL;

	strncpy(res->user_name, user, sizeof(res->user_name));

	if (batch_id && *batch_id)
		strncpy(res->batch_id, batch_id, sizeof(res->batch_id));

	if (rsvn_add_params(res, width, depth, nppn, mem_mb, mppnodes, accel) < 0) {
		free_rsvn(res);
		return NULL;
	}

	return res;
}

/**
 * basil_reserve  -  wrapper around rsvn_new.
 * @user:       owner of the reservation
 * @batch_id:   (numeric) job ID
 * @width:      mppwidth (aprun -n)
 * @depth:      mppdepth (aprun -d)
 * @nppn:       mppnppn  (aprun -N)
 * @mem_mb:     mppmem   (aprun -m)
 * @ns_head:    list of requested mppnodes (will be freed if not NULL)
 * @accel_head: optional accelerator parameters
 * Returns reservation ID > 0 if ok, negative %enum basil_error on error.
 */
long basil_reserve(const char *user, const char *batch_id,
		   uint32_t width, uint32_t depth, uint32_t nppn,
		   uint32_t mem_mb, struct nodespec *ns_head,
		   struct basil_accel_param *accel_head)
{
	struct basil_reservation *rsvn;
	struct basil_parse_data bp = {0};
	char *mppnodes = ns_to_string(ns_head);
	long rc;

	free_nodespec(ns_head);
	rsvn = rsvn_new(user, batch_id, width, depth, nppn, mem_mb,
			mppnodes, accel_head);
	if (rsvn == NULL)
		return -BE_INTERNAL;

	bp.method    = BM_reserve;
	bp.mdata.res = rsvn;
	bp.version   = BV_1_0;
	/*
	 * Rule:
	 * - if *res->batch_id is set, we are using Basil 1.1
	 * - if *res->batch_id == '\0' we have to fall back to Basil 1.0
	 */
	if (batch_id && *batch_id)
		bp.version = get_basil_version();

	rc = basil_request(&bp);
	if (rc >= 0)
		rc = rsvn->rsvn_id;
	free_rsvn(rsvn);
	return rc;
}
