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
#include "wr_weakmap.h"
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

void wr_store_dtor(void *ref_object, zend_object_handle ref_handle TSRMLS_DC) /* {{{ */
{
	wr_store                  *store      = WR_G(store);
	zend_objects_store_dtor_t  orig_dtor  = store->objs[ref_handle].orig_dtor;
	wr_store_data              data       = store->objs[ref_handle];
	wr_ref_list               *list_entry;

	EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor = data.orig_dtor;

	orig_dtor(ref_object, ref_handle TSRMLS_CC);

	/* data might have changed if the destructor freed weakrefs, we reload from store */
	list_entry = store->objs[ref_handle].wrefs_head;

	while (list_entry != NULL) {
		wr_ref_list *next = list_entry->next;
		list_entry->dtor(ref_object, ref_handle, list_entry->obj TSRMLS_CC);
		efree(list_entry);
		list_entry = next;
	}
}
/* }}} */

void wr_store_attach(zend_object *intern, wr_ref_dtor dtor, zval *ref TSRMLS_DC) /* {{{ */
{
	wr_store           *store      = WR_G(store);
	zend_object_handle  ref_handle = Z_OBJ_HANDLE_P(ref);
	wr_store_data      *data       = NULL;

	while (ref_handle >= store->size) {
		store->size <<= 2;
		store->objs = erealloc(store->objs, store->size * sizeof(wr_store_data));
	}

	data = &store->objs[ref_handle];

	if (EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor == wr_store_dtor) {
		wr_ref_list *next = emalloc(sizeof(wr_ref_list));
		next->obj  = intern;
		next->dtor = dtor;
		next->next = NULL;

		if (data->wrefs_head) {
			wr_ref_list *list_entry = data->wrefs_head;

			while (list_entry->next != NULL) {
				list_entry = list_entry->next;
			}

			list_entry->next = next;
		} else {
			data->wrefs_head = next;
		}
	} else {
		data->orig_dtor = EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor;
		EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor = wr_store_dtor;

        data->wrefs_head = emalloc(sizeof(wr_ref_list));
		data->wrefs_head->obj  = intern;
		data->wrefs_head->dtor = dtor;
		data->wrefs_head->next = NULL;
	}
}
/* }}} */

void wr_store_detach(zend_object *intern, zend_object_handle ref_handle TSRMLS_DC) /* {{{ */
{
	wr_store      *store           = WR_G(store);

	if (!store) {
		// detach() can be called after the store has already been cleaned up,
		// depending on the shutdown sequence (i.e. in case of a fatal).
		// See tests/weakref_007.phpt
		return;
	} else {
		wr_store_data *data            = &store->objs[ref_handle];
		wr_ref_list   *prev            = NULL;
		wr_ref_list   *cur             = data->wrefs_head;

		while (cur && cur->obj != intern) {
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

PHP_MINIT_FUNCTION(weakref) /* {{{ */
{
	PHP_MINIT(wr_weakref)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(wr_weakmap)(INIT_FUNC_ARGS_PASSTHRU);
}

PHP_RINIT_FUNCTION(weakref) /* {{{ */
{
	wr_store_init(TSRMLS_C);
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(weakref) /* {{{ */
{
	wr_store_destroy(TSRMLS_C);

	return SUCCESS;
}
/* }}} */

static PHP_GINIT_FUNCTION(weakref) /* {{{ */
{
	weakref_globals->store = NULL;
}
/* }}} */

PHP_MINFO_FUNCTION(weakref) /* {{{ */
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Weak References support", "enabled");
	php_info_print_table_row(2, "Version", PHP_WEAKREF_VERSION);
	php_info_print_table_end();
}
/* }}} */

zend_module_entry weakref_module_entry = { /* {{{ */
	STANDARD_MODULE_HEADER_EX, NULL,
	NULL,
	"Weakref",
	NULL,
	PHP_MINIT(weakref),
	NULL,
	PHP_RINIT(weakref),
	PHP_RSHUTDOWN(weakref),
	PHP_MINFO(weakref),
	PHP_WEAKREF_VERSION,
	PHP_MODULE_GLOBALS(weakref),
	PHP_GINIT(weakref),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_WEAKREF
ZEND_GET_MODULE(weakref);
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
