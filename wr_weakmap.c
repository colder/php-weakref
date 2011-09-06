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
#include "wr_weakmap.h"
#include "php_weakref.h"


static void wr_weakmap_ref_dtor(void *object, zend_object *wref_obj TSRMLS_DC) { /* {{{ */
	/*
	wr_weakref_object *wref = (wr_weakref_object *)wref_obj;
	wref->valid = 0;
	wref->ref = NULL;
	*/
}
/* }}} */

static void wr_weakmap_object_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	/*
	wr_weakref_object *intern     = (wr_weakref_object *)object;

	if (intern->valid) {
		zend_object_handle  ref_handle = Z_OBJ_HANDLE_P(intern->ref);
		wr_store      *store           = WR_G(store);

		wr_store_data *data            = &store->objs[ref_handle];
		wr_ref_list   *prev            = NULL;
		wr_ref_list   *cur             = data->wrefs_head;

		while (cur && cur->obj != (zend_object *)intern) {
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

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	efree(object);
	*/
}
/* }}} */

static zend_object_value wr_weakmap_object_new_ex(zend_class_entry *class_type, wr_weakmap_object **obj, zval *orig, int clone_orig TSRMLS_DC) /* {{{ */
{
	zend_object_value  retval;
	wr_weakmap_object *intern;

#if PHP_VERSION_ID < 50399
	zval              *tmp;
#endif

	intern = ecalloc(1, sizeof(wr_weakmap_object));

	*obj = intern;

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);

#if PHP_VERSION_ID >= 50399
	object_properties_init(&intern->std, class_type);
#else
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#endif

	if (clone_orig && orig) {
		wr_weakmap_object *other = (wr_weakmap_object *)zend_object_store_get_object(orig TSRMLS_CC);
		//TODO
	} else {
		//TODO
	}

	retval.handle   = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, wr_weakmap_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = &wr_handler_WeakMap;

	return retval;
}
/* }}} */

static zend_object_value wr_weakmap_object_clone(zval *zobject TSRMLS_DC) /* {{{ */
{
	zend_object_value      new_obj_val;
	zend_object           *old_object;
	zend_object           *new_object;
	zend_object_handle     handle = Z_OBJ_HANDLE_P(zobject);
	wr_weakmap_object     *intern;

	old_object  = zend_objects_get_address(zobject TSRMLS_CC);
	new_obj_val = wr_weakmap_object_new_ex(old_object->ce, &intern, zobject, 1 TSRMLS_CC);
	new_object  = &intern->std;

	zend_objects_clone_members(new_object, new_obj_val, old_object, handle TSRMLS_CC);

	return new_obj_val;
}
/* }}} */

static zend_object_value wr_weakmap_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
    wr_weakmap_object *tmp;
	return wr_weakmap_object_new_ex(class_type, &tmp, NULL, 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto void WeakMap::__construct()
*/
PHP_METHOD(WeakMap, __construct)
{
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}
}
/* }}} */

/*  Function/Class/Method definitions */
ZEND_BEGIN_ARG_INFO(arginfo_wr_weakmap_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wr_weakmap_obj, 0)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_wr_weakmap_obj_val, 0)
	ZEND_ARG_INFO(0, object)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static const zend_function_entry wr_funcs_WeakMap[] = {
	PHP_ME(WeakMap, __construct,     arginfo_wr_weakmap_void,             ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(wr_weakmap) /* {{{ */
{
	zend_class_entry weakmap_ce;

	INIT_CLASS_ENTRY(weakmap_ce, "WeakMap", wr_funcs_WeakMap);

	wr_ce_WeakMap = zend_register_internal_class(&weakmap_ce TSRMLS_CC);

	wr_ce_WeakMap->ce_flags      |= ZEND_ACC_FINAL_CLASS;
	wr_ce_WeakMap->create_object  = wr_weakmap_object_new;

	memcpy(&wr_handler_WeakMap, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	wr_handler_WeakMap.clone_obj = wr_weakmap_object_clone;

	return SUCCESS;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
