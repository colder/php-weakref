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

#ifndef PHP_WEAKREF_H
#define PHP_WEAKREF_H

#include <php.h>

#define PHP_WEAKREF_VERSION "0.0.1-alpha"

#ifdef PHP_WIN32
#define WEAKREF_API __declspec(dllexport)
#else
#define WEAKREF_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "wr_weakref.h"

extern zend_module_entry weakref_module_entry;
#define phpext_weakref_ptr &weakref_module_entry

PHP_MINFO_FUNCTION(weakref);

PHP_MINIT_FUNCTION(weakref);
PHP_RINIT_FUNCTION(weakref);
PHP_RSHUTDOWN_FUNCTION(weakref);

typedef void (*wr_ref_dtor)(void *ref_object, zend_object_handle ref_handle, zend_object *wref_obj TSRMLS_DC);

typedef struct _wr_ref_list {
	zend_object              *obj;
	wr_ref_dtor          dtor;
	struct _wr_ref_list *next;
} wr_ref_list;

typedef struct _wr_store_data {
	zend_objects_store_dtor_t  orig_dtor;
	wr_ref_list          *wrefs_head;
} wr_store_data;

typedef struct _wr_store {
	wr_store_data *objs;
	uint size;
} wr_store;

ZEND_BEGIN_MODULE_GLOBALS(weakref)
    wr_store *store;
ZEND_END_MODULE_GLOBALS(weakref)

void wr_store_init(TSRMLS_D);
void wr_store_destroy(TSRMLS_D);
void wr_store_dtor(void *object, zend_object_handle ref_handle TSRMLS_DC);
void wr_store_attach(zend_object *intern, wr_ref_dtor dtor, zval *ref TSRMLS_DC);

#ifdef ZTS
#define WR_G(v) TSRMG(weakref_globals_id, zend_weakref_globals *, v)
int weakref_globals_id;
#else
#define WR_G(v) (weakref_globals.v)
zend_weakref_globals weakref_globals;
#endif

#endif /* PHP_WEAKREF_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
