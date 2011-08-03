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

#ifndef WEAKREF_H
#define WEAKREF_H

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

extern WEAKREF_API zend_class_entry *weakref_ce_WeakRef;

PHP_MINFO_FUNCTION(weakref);

PHP_MINIT_FUNCTION(weakref);
PHP_RINIT_FUNCTION(weakref);
PHP_RSHUTDOWN_FUNCTION(weakref);

zend_object_handlers weakref_handler_WeakRef;
WEAKREF_API zend_class_entry  *weakref_ce_WeakRef;

typedef struct _weakref_object {
	zend_object            std;
	zval                  *ref;
	zend_bool              valid;
} weakref_object;

typedef struct _weakref_ref_list {
	weakref_object           *wref;
	struct _weakref_ref_list *next;
} weakref_ref_list;

typedef struct _weakref_store_data {
	zend_objects_store_dtor_t  orig_dtor;
	weakref_ref_list          *wrefs_head;
} weakref_store_data;

typedef struct _weakref_store {
	weakref_store_data *objs;
	uint size;
} weakref_store;

ZEND_BEGIN_MODULE_GLOBALS(weakref)
    weakref_store *store;
ZEND_END_MODULE_GLOBALS(weakref)

#ifdef ZTS
#define WEAKREF_G(v) TSRMG(weakref_globals_id, zend_weakref_globals *, v)
int weakref_globals_id;
#else
#define WEAKREF_G(v) (weakref_globals.v)
zend_weakref_globals weakref_globals;
#endif

#endif /* WEAKREF_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
