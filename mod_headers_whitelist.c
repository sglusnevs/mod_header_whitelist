#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"
#include "apr_strings.h"

/* Forward declaration of module */
module AP_MODULE_DECLARE_DATA headers_whitelist_module;

/* Module configuration per server/vhost */
typedef struct {
    apr_array_header_t *whitelist;      /* headers allowed */
    apr_array_header_t *sensitive;      /* headers to hide in logs */
} whitelist_cfg;

/* Create per-server/vhost configuration */
static void *create_whitelist_server_config(apr_pool_t *p, server_rec *s) {
    whitelist_cfg *cfg = apr_pcalloc(p, sizeof(*cfg));
    cfg->whitelist = apr_array_make(p, 4, sizeof(const char *));
    cfg->sensitive = apr_array_make(p, 4, sizeof(const char *));
    return cfg;
}

/* Directive handler: HeaderWhitelist */
static const char *set_whitelist(cmd_parms *cmd, void *dummy, const char *arg) {
    whitelist_cfg *cfg = ap_get_module_config(cmd->server->module_config,
                                              &headers_whitelist_module);
    const char **new = (const char **)apr_array_push(cfg->whitelist);
    *new = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

/* Directive handler: SensitiveHeaders */
static const char *set_sensitive(cmd_parms *cmd, void *dummy, const char *arg) {
    whitelist_cfg *cfg = ap_get_module_config(cmd->server->module_config,
                                              &headers_whitelist_module);
    const char **new = (const char **)apr_array_push(cfg->sensitive);
    *new = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

/* Apache directive table */
static const command_rec whitelist_cmds[] = {
    AP_INIT_ITERATE("HeadersClientWhitelist",
                    set_whitelist,
                    NULL,
                    RSRC_CONF,
                    "List of client headers to allow through (case-insensitive)"),
    AP_INIT_ITERATE("HeadersClientSensitive",
                    set_sensitive,
                    NULL,
                    RSRC_CONF,
                    "List of client headers whose values should be hidden from logs"),
    {NULL}
};

/* Check if a header is in a given array */
static int is_in_array(apr_array_header_t *arr, const char *name) {
    if (!arr || arr->nelts == 0) return 0;
    const char **elts = (const char **)arr->elts;
    for (int i = 0; i < arr->nelts; i++) {
        if (apr_strnatcasecmp(name, elts[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Hook: strip disallowed headers, log placeholders for sensitive headers */
static int whitelist_fixups(request_rec *r) {
    /* Skip subrequests */
    if (r->main) return DECLINED;

    whitelist_cfg *cfg = ap_get_module_config(r->server->module_config,
                                              &headers_whitelist_module);

    if (!cfg || !cfg->whitelist || cfg->whitelist->nelts == 0) {
        return DECLINED;
    }

    const apr_array_header_t *arr = apr_table_elts(r->headers_in);
    apr_table_entry_t *elts = (apr_table_entry_t *)arr->elts;
    apr_table_t *new_headers = apr_table_make(r->pool, arr->nelts);

    char *placeholder;

    for (int i = 0; i < arr->nelts; i++) {
        if (!elts[i].key) continue;

        /* Determine placeholder */
	    if (is_in_array(cfg->sensitive, elts[i].key)) {
		    placeholder = "<hidden>";
	    } else {
		    placeholder = elts[i].val;
	    }

        if (is_in_array(cfg->whitelist, elts[i].key)) {
            apr_table_add(new_headers, elts[i].key, elts[i].val);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "whitelist: allowed header: %s: %s",
                          elts[i].key, placeholder);
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                          "whitelist: stripped header: %s: %s",
                          elts[i].key, placeholder);
        }
    }

    r->headers_in = new_headers;
    return DECLINED;
}

/* Register hooks */
static void register_hooks(apr_pool_t *p) {
    ap_hook_fixups(whitelist_fixups, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Module declaration */
module AP_MODULE_DECLARE_DATA headers_whitelist_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                       /* per-dir config */
    NULL,                       /* merge per-dir config */
    create_whitelist_server_config, /* per-server config */
    NULL,                       /* merge per-server config */
    whitelist_cmds,             /* directives */
    register_hooks
};

