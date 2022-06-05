/* pdo_taosw extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

extern "C" {
    #include "php.h"
    #include "php_ini.h"
    #include "ext/standard/info.h"
    #include "pdo/php_pdo.h"
    #include "pdo/php_pdo_driver.h"
    #include "php_pdo_taosw.h"
    #include "php_pdo_taosw_int.h"
}

#include "swoole_coroutine.h"

int taos_inited = 0;

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pdo_taosw)
{
    php_pdo_register_driver(&pdo_taosw_driver);

    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_NULL",  (zend_long)(TSDB_DATA_TYPE_NULL + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_BOOL",  (zend_long)(TSDB_DATA_TYPE_BOOL + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_TINYINT",  (zend_long)(TSDB_DATA_TYPE_TINYINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_SMALLINT",  (zend_long)(TSDB_DATA_TYPE_SMALLINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_INT",  (zend_long)(TSDB_DATA_TYPE_INT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_BIGINT",  (zend_long)(TSDB_DATA_TYPE_BIGINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_FLOAT",  (zend_long)(TSDB_DATA_TYPE_FLOAT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_DOUBLE",  (zend_long)(TSDB_DATA_TYPE_DOUBLE + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_BINARY",  (zend_long)(TSDB_DATA_TYPE_BINARY + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_TIMESTAMP",  (zend_long)(TSDB_DATA_TYPE_TIMESTAMP + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_NCHAR",  (zend_long)(TSDB_DATA_TYPE_NCHAR + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_UTINYINT",  (zend_long)(TSDB_DATA_TYPE_UTINYINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_USMALLINT",  (zend_long)(TSDB_DATA_TYPE_USMALLINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_UINT",  (zend_long)(TSDB_DATA_TYPE_UINT + 6000));
    REGISTER_PDO_CLASS_CONST_LONG("PARAM_TAOSW_UBIGINT",  (zend_long)(TSDB_DATA_TYPE_UBIGINT + 6000));

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pdo_taosw)
{
    if (taos_inited) {
        taos_cleanup();
        taos_inited = 0;
    }
    php_pdo_unregister_driver(&pdo_taosw_driver);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pdo_taosw)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "pdo_taosw support", "enabled");

    php_info_print_table_row(2, "Module version", pdo_taosw_module_entry.version);
	php_info_print_table_end();
}
/* }}} */

/* {{{ pdo_taosw_functions[]
 */
static const zend_function_entry pdo_taosw_functions[] = {
	PHP_FE_END
};
/* }}} */

/* {{{ pdo_taosw_deps
 */
static zend_module_dep pdo_taosw_deps[] = {
    ZEND_MOD_REQUIRED("pdo")
        ZEND_MOD_END
};
/* }}} */


/* {{{ pdo_taosw_module_entry */
zend_module_entry pdo_taosw_module_entry = {
    STANDARD_MODULE_HEADER_EX,
    NULL,
    pdo_taosw_deps,
    "pdo_taosw",
    pdo_taosw_functions,
    PHP_MINIT(pdo_taosw),
    PHP_MSHUTDOWN(pdo_taosw),
    NULL,
    NULL,
    PHP_MINFO(pdo_taosw),
    PHP_PDO_TAOSW_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */


#ifdef COMPILE_DL_PDO_TAOSW
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(pdo_taosw)
#endif
