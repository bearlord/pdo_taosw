#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* {{{ _pdo_taosw_error */
int _pdo_taosw_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, int errcode, const char *sqlstate, const char *msg, const char *file, int line)
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;
    pdo_error_type *pdo_err = stmt ? &stmt->error_code : &dbh->error_code;
    pdo_taosw_error_info *einfo = &H->einfo;
    char *errmsg;
    pdo_taosw_stmt *S = NULL;

    if (stmt) {
        S = (pdo_taosw_stmt *) stmt->driver_data;
        pdo_err = &stmt->error_code;
        einfo = &S->einfo;
    } else {
        pdo_err = &dbh->error_code;
        einfo = &H->einfo;
    }
    pdo_err = &dbh->error_code;
    einfo = &H->einfo;

    einfo->errcode = errcode;
    einfo->file = file;
    einfo->line = line;

    if (einfo->errmsg) {
        pefree(einfo->errmsg, dbh->is_persistent);
        einfo->errmsg = NULL;
    }

    if (sqlstate == NULL || strlen(sqlstate) >= sizeof(pdo_error_type)) {
        strcpy(*pdo_err, "HY000");
    } else {
        strcpy(*pdo_err, sqlstate);
    }

    if (msg) {
        einfo->errmsg = estrdup(msg);
    }

    zend_throw_exception_ex(php_pdo_get_exception(), einfo->errcode, "SQLSTATE[%s] [0x%04x] %s",
                            *pdo_err, einfo->errcode, einfo->errmsg);

    return einfo->errcode;
}
/* }}} */

/* {{{ */
static int pdo_taosw_fetch_error_func(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info)
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;
    pdo_taosw_error_info *einfo = &H->einfo;

    if (stmt) {
        pdo_taosw_stmt *S = (pdo_taosw_stmt *) stmt->driver_data;
        einfo = &S->einfo;
    } else {
        einfo = &H->einfo;
    }

    if (einfo->errcode) {
        add_next_index_long(info, einfo->errcode);
        add_next_index_string(info, einfo->errmsg);
    }

    return 1;
}
/* }}} */

/* {{{ taosw_handle_closer */
static int taosw_handle_closer(pdo_dbh_t *dbh) /* {{{ */
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;
    if (H) {
        if (H->server) {
            taos_close(H->server);
            H->server = NULL;
        }
        if (H->einfo.errmsg) {
            pefree(H->einfo.errmsg, dbh->is_persistent);
            H->einfo.errmsg = NULL;
        }
        pefree(H, dbh->is_persistent);
        dbh->driver_data = NULL;
    }
    return 0;
}
/* }}} */

/* {{{ taosw_handle_preparer */
static int
taosw_handle_preparer(pdo_dbh_t *dbh, const char *sql, size_t sql_len, pdo_stmt_t *stmt, zval *driver_options)
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;
    pdo_taosw_stmt *S = ecalloc(1, sizeof(pdo_taosw_stmt));
    int ret;
    char *nsql = NULL;
    size_t nsql_len = 0;
    int emulate = 0;
    int execute_only = 0;
    int num_params = 0;

    S->H = H;
    stmt->driver_data = S;
    stmt->methods = &taosw_stmt_methods;

    stmt->supports_placeholders = PDO_PLACEHOLDER_POSITIONAL;
    ret = pdo_parse_params(stmt, (char *) sql, sql_len, &nsql, &nsql_len);

    if (ret == 1) {
        /* query was rewritten */
        sql = nsql;
        sql_len = nsql_len;
    } else if (ret == -1) {
        /* failed to parse */
        strcpy(dbh->error_code, stmt->error_code);
        return 0;
    }

    if (!(S->stmt = taos_stmt_init(H->server))) {
        pdo_taosw_error(dbh);
        if (nsql) {
            efree(nsql);
        }
        return 0;
    }

    int code = taos_stmt_prepare(S->stmt, sql, 0);
    if (code != 0) {
        pdo_taosw_error_stmt_msg(stmt, pdo_taosw_convert_errno(code), taos_stmt_errstr(S->stmt));
        if (nsql) {
            efree(nsql);
        }
        return 0;
    }

    taos_stmt_num_params(S->stmt, &num_params);
    S->num_params = num_params;
    if (S->num_params) {
        S->params_given = 0;
        S->params = ecalloc(S->num_params, sizeof(TAOS_MULTI_BIND));
        S->in_null = ecalloc(S->num_params, sizeof(zend_bool));
        S->in_length = ecalloc(S->num_params, sizeof(zend_ulong));
    }

    dbh->alloc_own_columns = 1;
    S->max_length = pdo_attr_lval(driver_options, PDO_ATTR_MAX_COLUMN_LEN, 0);

    return 1;
}
/* }}} */

/* {{{ taosw_handle_doer */
static zend_long taosw_handle_doer(pdo_dbh_t *dbh, const char *sql, size_t sql_len)
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;
    TAOS_RES *res;
    zend_long ret = 1;
    int errno;

    res = taos_query(H->server, sql);
    errno = taos_errno(res);
    if (errno != 0) {
        pdo_taosw_error_msg(dbh, pdo_taosw_convert_errno(errno), taos_errstr(res));
        return -1;
    }

    int c = taos_affected_rows(res);
    if (c == -1) {
        pdo_taosw_error_msg(dbh, pdo_taosw_convert_errno(errno), taos_errstr(res));
        return -1;
    }

    ret = Z_L(0);
    taos_free_result(res);

    return ret;
}
/* }}} */

/* {{{ taosw_handle_quoter */
static int
taosw_handle_quoter(pdo_dbh_t *dbh, const char *unquoted, size_t unquotedlen, char **quoted, size_t *quotedlen, enum pdo_param_type paramtype)
{
    int qcount = 0;
    char const *cu, *l, *r;
    char *c;

    if (!unquotedlen) {
        *quotedlen = 2;
        *quoted = emalloc(*quotedlen + 1);
        strcpy(*quoted, "''");
        return 1;
    }

    /* count single quotes */
    for (cu = unquoted; (cu = strchr(cu, '\'')); qcount++, cu++); /* empty loop */

    *quotedlen = unquotedlen + qcount + 2;
    *quoted = c = emalloc(*quotedlen + 1);
    *c++ = '\'';

    /* foreach (chunk that ends in a quote) */
    for (l = unquoted; (r = strchr(l, '\'')); l = r + 1) {
        strncpy(c, l, r - l + 1);
        c += (r - l + 1);
        *c++ = '\'';            /* add second quote */
    }

    /* Copy remainder and add enclosing quote */
    strncpy(c, l, *quotedlen - (c - *quoted) - 1);
    (*quoted)[*quotedlen - 1] = '\'';
    (*quoted)[*quotedlen] = '\0';

    return 1;
}
/* }}} */

/* {{{ pdo_taosw_last_insert_id */
static char *pdo_taosw_last_insert_id(pdo_dbh_t *dbh, const char *name, size_t *len)
{
    return NULL;
}
/* }}} */

/* {{{ pdo_taosw_get_attribute */
static int pdo_taosw_get_attribute(pdo_dbh_t *dbh, zend_long attr, zval *return_value)
{
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;

    switch (attr) {
        case PDO_ATTR_CLIENT_VERSION:
            ZVAL_STRING(return_value, (char *) taos_get_client_info());
            break;

        case PDO_ATTR_SERVER_VERSION:
            ZVAL_STRING(return_value, (char *) taos_get_server_info(H->server));
            break;

        default:
            return 0;
    }

    return 1;
}
/* }}} */

/* {{{ pdo_taosw_check_liveness */
static int pdo_taosw_check_liveness(pdo_dbh_t *dbh)
{
    return SUCCESS;
}
/* }}} */

/* {{{ taosw_handle_in_transaction */
static int taosw_handle_in_transaction(pdo_dbh_t *dbh)
{
    return 1;
}
/* }}} */

/* {{{ taosw_handle_begin */
static int taosw_handle_begin(pdo_dbh_t *dbh)
{
    return 1;
}
/* }}} */

/* {{{ taosw_handle_commit */
static int taosw_handle_commit(pdo_dbh_t *dbh)
{
    return 1;
}
/* }}} */

/* {{{ taosw_handle_rollback */
static int taosw_handle_rollback(pdo_dbh_t *dbh)
{
    return 1;
}
/* }}} */

/* {{{ dbh_methods */
static const zend_function_entry dbh_methods[] = {
        PHP_FE_END
};
/* }}} */

/* {{{ pdo_taosw_get_driver_methods */
static const zend_function_entry *pdo_taosw_get_driver_methods(pdo_dbh_t *dbh, int kind)
{
    switch (kind) {
        case PDO_DBH_DRIVER_METHOD_KIND_DBH:
            return dbh_methods;
        default:
            return NULL;
    }
}
/* }}} */

/* {{{ pdo_taosw_set_attr */
static int pdo_taosw_set_attr(pdo_dbh_t *dbh, zend_long attr, zval *val)
{
    zend_bool bval = zval_get_long(val) ? 1 : 0;
    pdo_taosw_db_handle *H = (pdo_taosw_db_handle *) dbh->driver_data;

    switch (attr) {
        case PDO_ATTR_EMULATE_PREPARES:
            H->emulate_prepares = bval;
            return 1;

        default:
            return 0;
    }
}
/* }}} */

/* {{{ taosw_methods */
static const struct pdo_dbh_methods taosw_methods = {
        taosw_handle_closer,
        taosw_handle_preparer,
        taosw_handle_doer,
        taosw_handle_quoter,
        NULL, /* handle begin */
        NULL, /* handle commit */
        NULL, /* handle rollback */
        pdo_taosw_set_attr,
        NULL, /* last_insert_id */
        pdo_taosw_fetch_error_func,
        pdo_taosw_get_attribute,
        NULL,    /* check_liveness */
        pdo_taosw_get_driver_methods,  /* get_driver_methods */
        NULL,
        taosw_handle_in_transaction, /* in_transaction */
};
/* {{{ */


/* {{{ pdo_taosw_handle_factory */
static int pdo_taosw_handle_factory(pdo_dbh_t *dbh, zval *driver_options) /* {{{ */
{
    pdo_taosw_db_handle *H;
    size_t i;
    int ret = 0;
    char *host = NULL;
    char *dbname = NULL;
    char *charset = NULL;
    char *timezone = NULL;
    uint16_t port = 6030;
    zend_long connect_timeout = 30;

    struct pdo_data_src_parser vars[] = {
            {"dbname",   "",          0},
            {"host",     "localhost", 0},
            {"port",     "6030",      0},
            {"user",     NULL,        0},
            {"password", NULL,        0},
            {"charset",  NULL,        0},
            {"timezone", NULL,        0}
    };

    php_pdo_parse_data_source(dbh->data_source, dbh->data_source_len, vars, 7);

    H = pecalloc(1, sizeof(pdo_taosw_db_handle), dbh->is_persistent);
    dbh->driver_data = H;

    H->einfo.errcode = 0;
    H->einfo.errmsg = NULL;

    if (driver_options) {
        connect_timeout = pdo_attr_lval(driver_options, PDO_ATTR_TIMEOUT, 30);
    }

    dbname = vars[0].optval;
    host = vars[1].optval;
    if (vars[2].optval) {
        port = atoi(vars[2].optval);
    }

    charset = vars[5].optval;
    if (charset) {
        taos_options(TSDB_OPTION_CHARSET, charset);
    }
    timezone = vars[6].optval;
    if (timezone) {
        taos_options(TSDB_OPTION_TIMEZONE, timezone);
    }

    swoole::coroutine::async([&]() {
        taos_init();
        H->server = taos_connect(host, dbh->username, dbh->password, dbname, port);

        if (H->server == NULL) {
            zend_throw_exception_ex(NULL, 0, "SQLSTATE[%s] [%s] %s", "HY000", "0x000B", "Unable to establish connection");
            for (i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
                if (vars[i].freeme) {
                    efree(vars[i].optval);
                }
            }
            taosw_handle_closer(dbh);
        } else {
            taosw_inited = 1;
            H->attached = 1;

            dbh->methods = &taosw_methods;
            dbh->alloc_own_columns = 1;
            dbh->max_escaped_char_length = 2;

            ret = 1;
        }
    });

    return ret;
}
/* }}} */

const pdo_driver_t pdo_taosw_driver = {
    PDO_DRIVER_HEADER(taosw),
    pdo_taosw_handle_factory
};

