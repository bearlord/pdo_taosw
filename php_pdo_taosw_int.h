#ifndef PHP_PDO_TAOSW_INT_H
#define PHP_PDO_TAOSW_INT_H

#include <taos.h>
#include <taoserror.h>
#include <php.h>

#define PDO_TAOSW_PARAM_BIND TAOS_BIND
#define PHP_PDO_TAOSW_CONNECTION_FAILURE_SQLSTATE "08006"

typedef struct {
    const char *file;
    int line;
    unsigned int errcode;
    char *errmsg;
} pdo_taosw_error_info;

/* stuff we use in a taos database handle */
typedef struct {
    TAOS *server;
    unsigned attached: 1;
    unsigned _reserved: 31;
    pdo_taosw_error_info einfo;
    unsigned int stmt_counter;
    /* The following two variables have the same purpose. Unfortunately we need
       to keep track of two different attributes having the same effect. */
    zend_bool emulate_prepares;
    zend_bool disable_native_prepares; /* deprecated since 5.6 */
    zend_bool disable_prepares;
    zend_ulong max_buffer_size;
    unsigned fetch_table_names: 1;
} pdo_taosw_db_handle;

typedef struct {
    TAOS_FIELD *def;
} pdo_taosw_column;

typedef struct {
    pdo_taosw_db_handle *H;
    TAOS_RES *result;
    TAOS_STMT *stmt;
    TAOS_FIELD *fields;
    TAOS_BIND *params;
    TAOS_BIND *bound_result;
    TAOS_ROW current_data;
    int *current_lengths;
    int num_params;
    int *in_null;
    zend_ulong *in_length;
    unsigned int params_given;
    unsigned max_length: 1;
    pdo_taosw_error_info einfo;
    int current_row;
    pdo_taosw_column *cols;
    char *query;
    char **param_values;
    int *param_lengths;
    int *param_formats;
    zend_bool is_prepared;
    int *out_null;
    zend_ulong *out_length;
} pdo_taosw_stmt;

#if PHP_VERSION_ID < 70300
extern pdo_driver_t pdo_taosw_driver;
extern struct pdo_stmt_methods taosw_stmt_methods;
#else
extern const pdo_driver_t pdo_taosw_driver;
extern const struct pdo_stmt_methods taosw_stmt_methods;
#endif

extern int
_pdo_taosw_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, int errcode, const char *sqlstate, const char *msg, const char *file,
                int line);

#define pdo_taosw_error(s) _pdo_taosw_error(s, NULL, 0, NULL, NULL, __FILE__, __LINE__)
#define pdo_taosw_error_msg(s, e, m)    _pdo_taosw_error(s, NULL, e, NULL, m, __FILE__, __LINE__)
#define pdo_taosw_error_stmt(s, e, z)    _pdo_taosw_error(s->dbh, s, e, z, NULL, __FILE__, __LINE__)
#define pdo_taosw_error_stmt_msg(s, e, m)    _pdo_taosw_error(s->dbh, s, e, NULL, m, __FILE__, __LINE__)
#define pdo_taosw_convert_errno(errno) ((int32_t) (errno > 0 ? errno : errno + 0x80000000))

#endif /* PHP_PDO_TAOSW_INT_H */