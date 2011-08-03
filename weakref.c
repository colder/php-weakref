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
#include "weakref.h"

static void weakref_store_init(TSRMLS_D) {
	weakref_store *store = emalloc(sizeof(weakref_store));
	store->objs = emalloc(sizeof(weakref_store_data));
	store->size = 1;

	WEAKREF_G(store) = store;
}

static void weakref_store_destroy(TSRMLS_D) {
	weakref_store *store = WEAKREF_G(store);

	if (store->objs != NULL) {
		efree(store->objs);
	}

	efree(store);

	WEAKREF_G(store) = NULL;
}

static void weakref_store_dtor(void *object, zend_object_handle ref_handle TSRMLS_DC) /* {{{ */
{
	weakref_store         *store         = WEAKREF_G(store);
	zend_objects_store_dtor_t  orig_dtor = store->objs[ref_handle].orig_dtor;
	weakref_store_data    data           = store->objs[ref_handle];
	weakref_ref_list     *list_entry     = data.wrefs_head;

	EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor = data.orig_dtor;

	orig_dtor(object, ref_handle TSRMLS_CC);

	while (list_entry != NULL) {
		weakref_ref_list *next = list_entry->next;
		list_entry->wref->valid = 0;
		list_entry->wref->ref = NULL;
		efree(list_entry);
		list_entry = next;
	}
}
/* }}} */

static void weakref_store_attach(weakref_object *intern, zval *ref TSRMLS_DC) /* {{{ */
{
	weakref_store      *store      = WEAKREF_G(store);
	zend_object_handle  ref_handle = Z_OBJ_HANDLE_P(ref);
	weakref_store_data *data       = NULL;

	while (ref_handle >= store->size) {
		store->size <<= 2;
		store->objs = erealloc(store->objs, store->size * sizeof(weakref_store_data));
	}

	data = &store->objs[ref_handle];

	if (EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor == weakref_store_dtor) {
		weakref_ref_list *list_entry = data->wrefs_head;
		weakref_ref_list *next       = emalloc(sizeof(weakref_ref_list)); 

		next->wref = intern;
		next->next = NULL;

		while (list_entry->next != NULL) {
			list_entry = list_entry->next;
		}

		list_entry->next = next;
	} else {
		data->orig_dtor = EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor;
		EG(objects_store).object_buckets[ref_handle].bucket.obj.dtor = weakref_store_dtor;

        data->wrefs_head = emalloc(sizeof(weakref_ref_list));
		data->wrefs_head->wref = intern;
		data->wrefs_head->next = NULL;
	}
}
/* }}} */


static void weakref_object_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	weakref_object *intern     = (weakref_object *)object;

	if (intern->valid) {
		zend_object_handle  ref_handle = Z_OBJ_HANDLE_P(intern->ref);
		weakref_store      *store      = WEAKREF_G(store);

		weakref_store_data *data       = &store->objs[ref_handle];
		weakref_ref_list   *prev       = NULL;
		weakref_ref_list   *cur        = data->wrefs_head;

		while (cur && cur->wref != intern) {
			prev = cur;
			cur = cur->next;
		}

		assert(cur != NULL);

		if (prev) {
			prev->next = cur->next;
		} else {
			data->wrefs_head = cur->next;
		}

		efree(cur);
	}

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	efree(object);
}
/* }}} */


static zend_object_value weakref_object_new_ex(zend_class_entry *class_type, weakref_object **obj, zval *orig, int clone_orig TSRMLS_DC) /* {{{ */
{
	zend_object_value  retval;
	weakref_object    *intern;
	zval              *tmp;

	intern = ecalloc(1, sizeof(weakref_object));

	*obj = intern;

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));


	if (clone_orig && orig) {
		weakref_object *other = (weakref_object *)zend_object_store_get_object(orig TSRMLS_CC);
		if (other->valid) {
			intern->valid = other->valid;
			intern->ref   = other->ref;
			weakref_store_attach(intern, other->ref TSRMLS_CC);
		} else {
			intern->valid = 0;
			intern->ref   = NULL;
		}
	} else {
		intern->valid = 0;
		intern->ref   = NULL;
	}

	retval.handle   = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, weakref_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = &weakref_handler_WeakRef;

	return retval;
}
/* }}} */

static zend_object_value weakref_object_clone(zval *zobject TSRMLS_DC) /* {{{ */
{
	zend_object_value      new_obj_val;
	zend_object           *old_object;
	zend_object           *new_object;
	zend_object_handle     handle = Z_OBJ_HANDLE_P(zobject);
	weakref_object        *intern;

	old_object  = zend_objects_get_address(zobject TSRMLS_CC);
	new_obj_val = weakref_object_new_ex(old_object->ce, &intern, zobject, 1 TSRMLS_CC);
	new_object  = &intern->std;

	zend_objects_clone_members(new_object, new_obj_val, old_object, handle TSRMLS_CC);

	return new_obj_val;
}
/* }}} */

static zend_object_value weakref_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
    weakref_object *tmp;
	return weakref_object_new_ex(class_type, &tmp, NULL, 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto int WeakRef::get()
 Return the reference, or null. */
PHP_METHOD(WeakRef, get)
{
	zval *object = getThis();
	weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->valid) {
		RETURN_ZVAL(intern->ref, 1, 0);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto int WeakRef::valid()
 Return whether the reference still valid. */
PHP_METHOD(WeakRef, valid)
{
	zval *object = getThis();
	weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_BOOL(intern->valid);
}
/* }}} */

/* {{{ proto void WeakRef::__construct(object ref)
*/
PHP_METHOD(WeakRef, __construct)
{
	zval *ref, *object = getThis();
	weakref_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &ref)) {
		return;
	}

	intern = (weakref_object *)zend_object_store_get_object(object TSRMLS_CC);

	intern->ref   = ref;

	weakref_store_attach(intern, ref TSRMLS_CC);

	intern->valid = 1;
}
/* }}} */

/*  Function/Class/Method definitions */
ZEND_BEGIN_ARG_INFO(arginfo_weakref_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_weakref_obj, 0)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

static const zend_function_entry weakref_funcs_WeakRef[] = {
	PHP_ME(WeakRef, __construct,     arginfo_weakref_obj,             ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, valid,           arginfo_weakref_void,            ZEND_ACC_PUBLIC)
	PHP_ME(WeakRef, get,             arginfo_weakref_void,            ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(weakref) /* {{{ */
{
	zend_class_entry weakref_ce;

	INIT_CLASS_ENTRY(weakref_ce, "WeakRef", weakref_funcs_WeakRef);

	weakref_ce_WeakRef = zend_register_internal_class(&weakref_ce TSRMLS_CC);

	weakref_ce_WeakRef->ce_flags      |= ZEND_ACC_FINAL_CLASS;
	weakref_ce_WeakRef->create_object  = weakref_object_new;

	memcpy(&weakref_handler_WeakRef, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	weakref_handler_WeakRef.clone_obj = weakref_object_clone;

	return SUCCESS;
}

PHP_RINIT_FUNCTION(weakref) /* {{{ */
{
	weakref_store_init(TSRMLS_C);
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(weakref) /* {{{ */
{
	weakref_store_destroy(TSRMLS_C);

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
