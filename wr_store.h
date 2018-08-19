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

#ifndef WR_STORE_H
#define WR_STORE_H

#include <php.h>

#ifdef ZTS
#include "TSRM.h"
#endif

#include "wr_weakref.h"

typedef void (*wr_ref_dtor)(zend_object *wref_obj, zend_object *ref_obj);

typedef struct _wr_ref_list {
	zend_object         *wref_obj;
	wr_ref_dtor          dtor;
	struct _wr_ref_list *next;
} wr_ref_list;

typedef struct _wr_store {
	HashTable objs;
	HashTable old_handlers;
} wr_store;

void wr_store_init(TSRMLS_D);
void wr_store_destroy(TSRMLS_D);
void wr_store_tracked_object_dtor(zend_object *ref_obj);
void wr_store_track(zend_object *wref_obj, wr_ref_dtor dtor, zend_object *ref_obj);
void wr_store_untrack(zend_object *wref_obj, zend_object *ref_obj);
void wr_store_restore_handlers(zend_object *object, zend_object_handlers* orig_handlers);

#endif /* PHP_WEAKREF_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
