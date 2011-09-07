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
#include "ext/spl/spl_exceptions.h"
#include "ext/standard/info.h"
#include "wr_weakmap.h"
#include "php_weakref.h"


static void wr_weakmap_ref_dtor(void *ref_object, zend_object_handle ref_handle, zend_object *wref_obj TSRMLS_DC) { /* {{{ */
	wr_weakmap_object *intern = (wr_weakmap_object *)wref_obj;

	zend_hash_index_del(&intern->map, ref_handle);
}
/* }}} */

static void wr_weakmap_object_free_storage(void *object TSRMLS_DC) /* {{{ */
{

	wr_weakmap_object *intern     = (wr_weakmap_object *)object;

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	zend_hash_destroy(&intern->map);

	efree(intern);
}
/* }}} */

static zend_object_value wr_weakmap_object_new_ex(zend_class_entry *class_type, wr_weakmap_object **obj, zval *orig, int clone_orig TSRMLS_DC) /* {{{ */
{
	zend_object_value  retval;
	wr_weakmap_object *intern;
	zend_class_entry  *parent = class_type;

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

	zend_hash_init(&intern->map, 0, NULL, ZVAL_PTR_DTOR, 0);

	if (clone_orig && orig) {
		wr_weakmap_object *other = (wr_weakmap_object *)zend_object_store_get_object(orig TSRMLS_CC);
		//TODO
	} else {
		//TODO
	}

	if (class_type != wr_ce_WeakMap) {
		zend_hash_find(&class_type->function_table, "offsetget",    sizeof("offsetget"),    (void **) &intern->fptr_offset_get);
		if (intern->fptr_offset_get->common.scope == wr_ce_WeakMap) {
			intern->fptr_offset_get = NULL;
		}
		zend_hash_find(&class_type->function_table, "offsetset",    sizeof("offsetset"),    (void **) &intern->fptr_offset_set);
		if (intern->fptr_offset_set->common.scope == wr_ce_WeakMap) {
			intern->fptr_offset_set = NULL;
		}
		zend_hash_find(&class_type->function_table, "offsetexists", sizeof("offsetexists"), (void **) &intern->fptr_offset_has);
		if (intern->fptr_offset_has->common.scope == wr_ce_WeakMap) {
			intern->fptr_offset_has = NULL;
		}
		zend_hash_find(&class_type->function_table, "offsetunset",  sizeof("offsetunset"),  (void **) &intern->fptr_offset_del);
		if (intern->fptr_offset_del->common.scope == wr_ce_WeakMap) {
			intern->fptr_offset_del = NULL;
		}
		zend_hash_find(&class_type->function_table, "count",        sizeof("count"),        (void **) &intern->fptr_count);
		if (intern->fptr_count->common.scope == wr_ce_WeakMap) {
			intern->fptr_count = NULL;
		}
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

static inline zval *wr_weakmap_object_read_dimension_helper(wr_weakmap_object *intern, zval *offset TSRMLS_DC) /* {{{ */
{
	zval *retval;

	if (!offset || Z_TYPE_P(offset) != IS_OBJECT) {
		zend_throw_exception(spl_ce_RuntimeException, "Index invalid or out of range", 0 TSRMLS_CC);
		return NULL;
	}

	if (zend_hash_index_find(&intern->map, Z_OBJ_HANDLE_P(offset), (void**)&retval) == FAILURE) {
		retval = NULL;
	}

	return retval;
}
/* }}} */

static zval *wr_weakmap_object_read_dimension(zval *object, zval *offset, int type TSRMLS_DC) /* {{{ */
{
	wr_weakmap_object *intern;
	zval *retval;

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_offset_get) {
		zval *rv;
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(&object, intern->std.ce, &intern->fptr_offset_get, "offsetGet", &rv, offset);
		zval_ptr_dtor(&offset);
		if (rv) {
			Z_DELREF_P(rv);
			return rv;
		}
		return EG(uninitialized_zval_ptr);
	}

	retval = wr_weakmap_object_read_dimension_helper(intern, offset TSRMLS_CC);
	if (retval) {
		return retval;
	}
	return NULL;
}
/* }}} */

static inline void wr_weakmap_object_write_dimension_helper(wr_weakmap_object *intern, zval *offset, zval *value TSRMLS_DC) /* {{{ */
{
	if (!offset) {
		zend_throw_exception(spl_ce_RuntimeException, "WeakMap does not support [] (append)", 0 TSRMLS_CC);
		return;
	}

	if (Z_TYPE_P(offset) != IS_OBJECT) {
		zend_throw_exception(spl_ce_RuntimeException, "Index is not an object", 0 TSRMLS_CC);
		return;
	}

	if (!zend_hash_index_exists(&intern->map, Z_OBJ_HANDLE_P(offset))) {
		// Object not already in the weakmap, we attach it
		wr_store_attach((zend_object *)intern, wr_weakmap_ref_dtor, offset TSRMLS_CC);
	}

	Z_ADDREF_P(value);
	if (zend_hash_index_update(&intern->map, Z_OBJ_HANDLE_P(offset), &value, sizeof(zval *), NULL) == FAILURE) {
		zend_throw_exception(spl_ce_RuntimeException, "Failed to update the map", 0 TSRMLS_CC);
		zval_ptr_dtor(&value);
		return;
	}
}
/* }}} */

static void wr_weakmap_object_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC) /* {{{ */
{
	wr_weakmap_object *intern;

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_offset_set) {
		if (!offset) {
			ALLOC_INIT_ZVAL(offset);
		} else {
			SEPARATE_ARG_IF_REF(offset);
		}
		SEPARATE_ARG_IF_REF(value);
		zend_call_method_with_2_params(&object, intern->std.ce, &intern->fptr_offset_set, "offsetSet", NULL, offset, value);
		zval_ptr_dtor(&value);
		zval_ptr_dtor(&offset);
		return;
	}

	wr_weakmap_object_write_dimension_helper(intern, offset, value TSRMLS_CC);
}
/* }}} */

static inline void wr_weakmap_object_unset_dimension_helper(wr_weakmap_object *intern, zval *offset TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		zend_throw_exception(spl_ce_RuntimeException, "Index is not an object", 0 TSRMLS_CC);
		return;
	}

	zend_hash_index_del(&intern->map, Z_OBJ_HANDLE_P(offset));
}
/* }}} */

static void wr_weakmap_object_unset_dimension(zval *object, zval *offset TSRMLS_DC) /* {{{ */
{
	wr_weakmap_object *intern;

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_offset_del) {
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(&object, intern->std.ce, &intern->fptr_offset_del, "offsetUnset", NULL, offset);
		zval_ptr_dtor(&offset);
		return;
	}

	wr_weakmap_object_unset_dimension_helper(intern, offset TSRMLS_CC);

}
/* }}} */

static inline int wr_weakmap_object_has_dimension_helper(wr_weakmap_object *intern, zval *offset, int check_empty TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(offset) != IS_OBJECT) {
		zend_throw_exception(spl_ce_RuntimeException, "Index is not an object", 0 TSRMLS_CC);
		return 0;
	}

	return zend_hash_index_exists(&intern->map, Z_OBJ_HANDLE_P(offset));
}
/* }}} */

static int wr_weakmap_object_has_dimension(zval *object, zval *offset, int check_empty TSRMLS_DC) /* {{{ */
{
	wr_weakmap_object *intern;

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_offset_get) {
		zval *rv;
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(&object, intern->std.ce, &intern->fptr_offset_has, "offsetExists", &rv, offset);
		zval_ptr_dtor(&offset);
		if (rv) {
			int result = i_zend_is_true(rv);
			zval_ptr_dtor(&rv);
			return result;
		}
		return 0;
	}

	return wr_weakmap_object_has_dimension_helper(intern, offset, check_empty TSRMLS_CC);
}
/* }}} */

static int wr_weakmap_object_count_elements(zval *object, long *count TSRMLS_DC) /* {{{ */
{
	wr_weakmap_object *intern;

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_count) {
		zval *rv;
		zend_call_method_with_0_params(&object, intern->std.ce, &intern->fptr_count, "count", &rv);
		if (rv) {
			zval *retval;
			MAKE_STD_ZVAL(retval);
			ZVAL_ZVAL(retval, rv, 1, 1);
			convert_to_long(retval);
			*count = (long) Z_LVAL_P(retval);
			zval_ptr_dtor(&retval);
			return SUCCESS;
		}
	}

	*count = zend_hash_num_elements(&intern->map);
	return SUCCESS;
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

/* {{{ proto int WeakMap::count(void)
*/
PHP_METHOD(WeakMap, count)
{
	zval *object = getThis();
	wr_weakmap_object *intern;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")) {
		return;
	}

	intern = (wr_weakmap_object *)zend_object_store_get_object(object TSRMLS_CC);

	RETURN_LONG(0);
}
/* }}} */

/* {{{ proto bool WeakMap::offsetExists(mixed $index) U
 Returns whether the requested $index exists. */
PHP_METHOD(WeakMap, offsetExists)
{
	zval                  *zindex;
	wr_weakmap_object  *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE) {
		return;
	}

	intern = (wr_weakmap_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(wr_weakmap_object_has_dimension_helper(intern, zindex, 0 TSRMLS_CC));
} /* }}} */

/* {{{ proto mixed WeakMap::offsetGet(mixed $index) U
 Returns the value at the specified $index. */
PHP_METHOD(WeakMap, offsetGet)
{
	zval               *zindex, *value_p;
	wr_weakmap_object  *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE) {
		return;
	}

	intern   = (wr_weakmap_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	value_p  = wr_weakmap_object_read_dimension_helper(intern, zindex TSRMLS_CC);

	if (value_p) {
		RETURN_ZVAL(value_p, 1, 0);
	}
	RETURN_NULL();
} /* }}} */

/* {{{ proto void WeakMap::offsetSet(mixed $index, mixed $newval) U
 Sets the value at the specified $index to $newval. */
PHP_METHOD(WeakMap, offsetSet)
{
	zval                  *zindex, *value;
	wr_weakmap_object  *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &zindex, &value) == FAILURE) {
		return;
	}

	intern = (wr_weakmap_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	wr_weakmap_object_write_dimension_helper(intern, zindex, value TSRMLS_CC);

} /* }}} */

/* {{{ proto void WeakMap::offsetUnset(mixed $index) U
 Unsets the value at the specified $index. */
PHP_METHOD(WeakMap, offsetUnset)
{
	zval                  *zindex;
	wr_weakmap_object  *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE) {
		return;
	}

	intern = (wr_weakmap_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	wr_weakmap_object_unset_dimension_helper(intern, zindex TSRMLS_CC);

} /* }}} */

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
	PHP_ME(WeakMap, __construct,     arginfo_wr_weakmap_void,      ZEND_ACC_PUBLIC)
	PHP_ME(WeakMap, count,           arginfo_wr_weakmap_void,      ZEND_ACC_PUBLIC)
	PHP_ME(WeakMap, offsetExists,    arginfo_wr_weakmap_obj,       ZEND_ACC_PUBLIC)
	PHP_ME(WeakMap, offsetGet,       arginfo_wr_weakmap_obj,       ZEND_ACC_PUBLIC)
	PHP_ME(WeakMap, offsetSet,       arginfo_wr_weakmap_obj_val,   ZEND_ACC_PUBLIC)
	PHP_ME(WeakMap, offsetUnset,     arginfo_wr_weakmap_obj,       ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

PHP_MINIT_FUNCTION(wr_weakmap) /* {{{ */
{
	zend_class_entry weakmap_ce;

	INIT_CLASS_ENTRY(weakmap_ce, "WeakMap", wr_funcs_WeakMap);

	wr_ce_WeakMap = zend_register_internal_class(&weakmap_ce TSRMLS_CC);

	wr_ce_WeakMap->ce_flags        |= ZEND_ACC_FINAL_CLASS;
	wr_ce_WeakMap->create_object    = wr_weakmap_object_new;

	memcpy(&wr_handler_WeakMap, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	wr_handler_WeakMap.clone_obj       = wr_weakmap_object_clone;
	wr_handler_WeakMap.read_dimension  = wr_weakmap_object_read_dimension;
	wr_handler_WeakMap.write_dimension = wr_weakmap_object_write_dimension;
	wr_handler_WeakMap.unset_dimension = wr_weakmap_object_unset_dimension;
	wr_handler_WeakMap.has_dimension   = wr_weakmap_object_has_dimension;
	wr_handler_WeakMap.count_elements  = wr_weakmap_object_count_elements;

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
