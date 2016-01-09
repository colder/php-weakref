/*
   +----------------------------------------------------------------------+
   | Weakref                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2011 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Etienne Kneuss <colder@php.net>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "wr_weakref.h"
#include "php_weakref.h"

void wr_store_init(TSRMLS_D) /* {{{ */
{
	wr_store *store = emalloc(sizeof(wr_store));
	store->objs = emalloc(sizeof(wr_store_data));
	store->size = 1;

	WR_G(store) = store;
} /* }}} */

void wr_store_destroy(TSRMLS_D) /* {{{ */
{
	wr_store *store = WR_G(store);

	if (store->objs != NULL) {
		efree(store->objs);
	}

	efree(store);

	WR_G(store) = NULL;
} /* }}} */

/* This function is invoked whenever an object tracked by a weak ref/map is
 * destroyed */
void wr_store_tracked_object_dtor(zend_object *ref_obj) /* {{{ */
{
	wr_store                  *store      = WR_G(store);
	uint32_t                   ref_handle = ref_obj->handle;
	zend_object_dtor_obj_t     orig_dtor  = store->objs[ref_handle].orig_dtor;

	
	// Not sure if it is really needed, but let's restore the original object
	// dtor before calling it.
	((zend_object_handlers *) ref_obj->handlers)->dtor_obj = orig_dtor;

	orig_dtor(ref_obj);

	/* Original dtor has been called, we invalidate the necessary weakrefs: */
	wr_ref_list *list_entry = store->objs[ref_handle].wrefs_head;

	/* Invalidate wrefs_head while dtoring, to prevent detach on same wr */
	store->objs[ref_handle].wrefs_head = NULL;

	while (list_entry != NULL) {
		wr_ref_list *next = list_entry->next;
		list_entry->dtor(list_entry->wref_obj, ref_obj TSRMLS_CC);
		efree(list_entry);
		list_entry = next;
	}
}
/* }}} */

/**
 * This function is called when a given weak-ref/map starts tracking the 'ref'
 * object.
 */
void wr_store_track(zend_object *wref_obj, wr_ref_dtor dtor, zend_object *ref_obj TSRMLS_DC) /* {{{ */
{
	wr_store *store      = WR_G(store);

	/* Grow store if needed */
	while (ref_obj->handle >= store->size) {
		store->size *= 2;
		store->objs = erealloc(store->objs, store->size * sizeof(wr_store_data));
	}

	wr_store_data *data = &store->objs[ref_obj->handle];

	/* The object in 'ref' needs to have its dtor replaced by
	 * 'wr_store_tracked_object_dtor' (if it's not there already) */
	if (ref_obj->handlers->dtor_obj != wr_store_tracked_object_dtor) {
		// FIXME
		zend_object_handlers *handlers = (zend_object_handlers *)ref_obj->handlers;
		data->orig_dtor = handlers->dtor_obj;
		handlers->dtor_obj = wr_store_tracked_object_dtor;
		data->wrefs_head = NULL;
	}

	/* We now put the weakref 'wref' in the list of weak references that need
	 * to be invalidated when 'ref' is destroyed */
	wr_ref_list *new_head = emalloc(sizeof(wr_ref_list));
	new_head->wref_obj = wref_obj;
	new_head->dtor	   = dtor;
	new_head->next	   = data->wrefs_head;

	data->wrefs_head = new_head;
}
/* }}} */

/**
 * This function is called when a given weak-ref/map stops tracking the 'ref'
 * object.
 */
void wr_store_untrack(zend_object *wref_obj, zend_object *ref_obj TSRMLS_DC) /* {{{ */
{
	wr_store      *store           = WR_G(store);

	if (!store) {
		// detach() can be called after the store has already been cleaned up,
		// depending on the shutdown sequence (i.e. in case of a fatal).
		// See tests/weakref_007.phpt
		return;
	} else {
		wr_store_data *data  = &store->objs[ref_obj->handle];
		wr_ref_list   *prev  = NULL;
		wr_ref_list   *cur   = data->wrefs_head;

		if (!cur) {
			// We are detaching from a wr that is being dtored, skip
			return;
		}

		while (cur && cur->wref_obj != wref_obj) {
			prev = cur;
			cur  = cur->next;
		}

		assert(cur != NULL);

		if (prev) {
			prev->next = cur->next;
		} else {
			data->wrefs_head = cur->next;
		}

		efree(cur);
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
