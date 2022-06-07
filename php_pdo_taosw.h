/* pdo_taosw extension for PHP (c) 2022 bearlord 565364226@qq.com */

#ifndef PHP_PDO_TAOSW_H
# define PHP_PDO_TAOSW_H

extern zend_module_entry pdo_taosw_module_entry;
# define phpext_pdo_taosw_ptr &pdo_taosw_module_entry

extern int taosw_inited;

# define PHP_PDO_TAOSW_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_PDO_TAOSW)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

PHP_MINIT_FUNCTION(pdo_taosw);
PHP_MSHUTDOWN_FUNCTION(pdo_taosw);
PHP_RINIT_FUNCTION(pdo_taosw);
PHP_RSHUTDOWN_FUNCTION(pdo_taosw);
PHP_MINFO_FUNCTION(pdo_taosw);

#endif	/* PHP_PDO_TAOSW_H */
