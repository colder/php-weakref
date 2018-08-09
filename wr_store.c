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

void wr_store_init() /* {{{ */
{
	wr_store *store = emalloc(sizeof(wr_store));

	zend_hash_init(&store->old_handlers, 0, NULL, NULL, 0);
	zend_hash_init(&store->objs, 0, NULL, NULL, 0);

	WR_G(store) = store;
} /* }}} */


void wr_store_restore_handlers(zend_object *object, zend_object_handlers* handlers) {
    efree((zend_object_handlers*)object->handlers);
    object->handlers = handlers;
}

void wr_store_destroy() /* {{{ */
{
	wr_store *store = WR_G(store);
    zend_object_handlers* orig_handlers;
	ulong key;

	ZEND_HASH_FOREACH_NUM_KEY_PTR(&store->old_handlers, key, orig_handlers) {
        wr_store_restore_handlers((zend_object *)key, orig_handlers);
	} ZEND_HASH_FOREACH_END();

	zend_hash_destroy(&store->old_handlers);
	zend_hash_destroy(&store->objs);

	efree(store);

	WR_G(store) = NULL;
} /* }}} */

/* This function is invoked whenever an object tracked by a weak ref/map is
 * destroyed */
void wr_store_tracked_object_dtor(zend_object *ref_obj) /* {{{ */
{
	wr_store                  *store      = WR_G(store);
	ulong handlers_key = (ulong)ref_obj;
    zend_object_handlers   *orig_handlers = zend_hash_index_find_ptr(&store->old_handlers, handlers_key);
	ulong                      handle_key = ref_obj->handle;
	wr_ref_list               *list_entry;

	/* Original dtor has been called, we invalidate the necessary weakrefs: */
    orig_handlers->dtor_obj(ref_obj);

    wr_store_restore_handlers((zend_object *)handlers_key, orig_handlers);
	zend_hash_index_del(&store->old_handlers, handlers_key);

	if ((list_entry = zend_hash_index_find_ptr(&store->objs, handle_key)) != NULL) {
		/* Invalidate wrefs_head while dtoring, to prevent detach on same wr */
		zend_hash_index_del(&store->objs, handle_key);

		do {
			wr_ref_list *next = list_entry->next;
			list_entry->dtor(list_entry->wref_obj, ref_obj);
			efree(list_entry);
			list_entry = next;
		} while (list_entry);
	}
}
/* }}} */

/**
 * This function is called when a given weak-ref/map starts tracking the 'ref'
 * object.
 */
void wr_store_track(zend_object *wref_obj, wr_ref_dtor dtor, zend_object *ref_obj) /* {{{ */
{
	wr_store *store        = WR_G(store);
	ulong     handlers_key = (ulong)ref_obj;
	ulong     handle_key   = ref_obj->handle;

	if (zend_hash_index_find_ptr(&store->old_handlers, handlers_key) == NULL) {
		size_t size = sizeof(zend_object_handlers);
		zend_hash_index_update_ptr(&store->old_handlers, handlers_key, ref_obj->handlers);
		zend_object_handlers* handlers = emalloc(size);
		memcpy(handlers, ref_obj->handlers, size);
		handlers->dtor_obj = wr_store_tracked_object_dtor;
		ref_obj->handlers = handlers;
	}

	wr_ref_list *tail = zend_hash_index_find_ptr(&store->objs, handle_key);

	/* We now put the weakref 'wref' in the list of weak references that need
	 * to be invalidated when 'ref' is destroyed */
	wr_ref_list *head = emalloc(sizeof(wr_ref_list));
	head->wref_obj = wref_obj;
	head->dtor     = dtor;
	head->next     = tail;

	zend_hash_index_update_ptr(&store->objs, handle_key, head);
}
/* }}} */

/**
 * This function is called when a given weak-ref/map stops tracking the 'ref'
 * object.
 */
void wr_store_untrack(zend_object *wref_obj, zend_object *ref_obj) /* {{{ */
{
	wr_store      *store           = WR_G(store);

	if (!store) {
		// detach() can be called after the store has already been cleaned up,
		// depending on the shutdown sequence (i.e. in case of a fatal).
		// See tests/weakref_007.phpt
		return;
	} else {
		wr_ref_list   *cur = zend_hash_index_find_ptr(&store->objs, ref_obj->handle);
		wr_ref_list   *prev  = NULL;

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
		} else if (cur->next) {
			zend_hash_index_update_ptr(&store->objs, ref_obj->handle, cur->next);
		} else {
			zend_hash_index_del(&store->objs, ref_obj->handle);
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
