#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_lua_api.h>


extern int ngx_ssl_ja3(ngx_connection_t *c);
extern int ngx_ssl_ja3_hash(ngx_connection_t *c);
extern int ngx_http2_fingerprint(ngx_connection_t *c, ngx_http_v2_connection_t *h2c);

static ngx_int_t ngx_http_ssl_fingerprint_init(ngx_conf_t *cf);

static ngx_http_module_t ngx_http_ssl_fingerprint_module_ctx = {
    NULL,                  /* preconfiguration */
    ngx_http_ssl_fingerprint_init, /* postconfiguration */
    NULL,                  /* create main configuration */
    NULL,                  /* init main configuration */
    NULL,                  /* create server configuration */
    NULL,                  /* merge server configuration */
    NULL,                  /* create location configuration */
    NULL                   /* merge location configuration */
};

ngx_module_t ngx_http_ssl_fingerprint_module = {
    NGX_MODULE_V1,
    &ngx_http_ssl_fingerprint_module_ctx, /* module context */
    NULL,                         /* module directives */
    NGX_HTTP_MODULE,              /* module type */
    NULL,                         /* init master */
    NULL,                         /* init module */
    NULL,                         /* init process */
    NULL,                         /* init thread */
    NULL,                         /* exit thread */
    NULL,                         /* exit process */
    NULL,                         /* exit master */
    NGX_MODULE_V1_PADDING};


static ngx_int_t
ngx_http_ssl_greased(ngx_http_request_t *r,
                 ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->connection == NULL)
    {
        return NGX_OK;
    }

    if (r->connection->ssl == NULL)
    {
        return NGX_OK;
    }

    if (ngx_ssl_ja3(r->connection) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    v->len = 1;
    v->data = (u_char*)(r->connection->ssl->fp_tls_greased ? "1" : "0");

    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}

static ngx_int_t
ngx_http_ssl_fingerprint(ngx_http_request_t *r,
                 ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->connection == NULL)
    {
        return NGX_OK;
    }

    if (r->connection->ssl == NULL)
    {
        return NGX_OK;
    }

    if (ngx_ssl_ja3(r->connection) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    v->data = r->connection->ssl->fp_ja3_str.data;
    v->len = r->connection->ssl->fp_ja3_str.len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}

static ngx_int_t
ngx_http_ssl_fingerprint_hash(ngx_http_request_t *r,
                 ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->connection == NULL)
    {
        return NGX_OK;
    }

    if (r->connection->ssl == NULL)
    {
        return NGX_OK;
    }

    if (ngx_ssl_ja3_hash(r->connection) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    v->data = r->connection->ssl->fp_ja3_hash.data;
    v->len = r->connection->ssl->fp_ja3_hash.len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}

int ngx_http_ssl_ffi_fingerprint(ngx_http_request_t *r,
                char *data, size_t len, size_t *len_out)

{
    ngx_http_lua_ssl_ctx_t *cctx;
    ngx_connection_t *c = r->connection;

    cctx = ngx_http_lua_ssl_get_ctx(c->ssl->connection);
    if (cctx == NULL) {
        c = cctx->connection;
    }

    if (c == NULL)
    {
        return NGX_OK;
    }

    if (c->ssl == NULL)
    {
        return NGX_OK;
    }

    if (ngx_ssl_ja3(c) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    if (len <= r->connection->ssl->fp_ja3_str.len)
    {
        return NGX_ERROR;
    }

    ngx_memcpy(data, r->connection->ssl->fp_ja3_str.data, r->connection->ssl->fp_ja3_str.len);
    data[r->connection->ssl->fp_ja3_str.len] = '\0';
    *len_out = r->connection->ssl->fp_ja3_str.len;

    return NGX_OK;
}

int ngx_http_ssl_ffi_fingerprint_hash(ngx_http_request_t *r,
                char *data, size_t len, size_t *len_out)
{
    ngx_http_lua_ssl_ctx_t *cctx;
    ngx_connection_t *c = r->connection;

    cctx = ngx_http_lua_ssl_get_ctx(c->ssl->connection);
    if (cctx == NULL) {
        c = cctx->connection;
    }

    if (c == NULL)
    {
        return NGX_OK;
    }

    if (r->connection->ssl == NULL)
    {
        return NGX_OK;
    }

    if (ngx_ssl_ja3_hash(c) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    if (len <= r->connection->ssl->fp_ja3_hash.len)
    {
        return NGX_ERROR;
    }

    ngx_memcpy(data, r->connection->ssl->fp_ja3_hash.data, r->connection->ssl->fp_ja3_hash.len);
    data[r->connection->ssl->fp_ja3_hash.len] = '\0';
    *len_out = r->connection->ssl->fp_ja3_hash.len;

    return NGX_OK;
}

static ngx_int_t
ngx_http_http2_fingerprint(ngx_http_request_t *r,
                 ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->connection == NULL)
    {
        return NGX_OK;
    }

    if (r->stream == NULL)
    {
        return NGX_OK;
    }

    if (r->stream->connection == NULL)
    {
        return NGX_OK;
    }

    if (ngx_http2_fingerprint(r->connection, r->stream->connection) == NGX_DECLINED)
    {
        return NGX_ERROR;
    }

    v->data = r->stream->connection->fp_str.data;
    v->len = r->stream->connection->fp_str.len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}

static ngx_http_variable_t ngx_http_ssl_fingerprint_variables_list[] = {
    {ngx_string("http_ssl_greased"),
     NULL,
     ngx_http_ssl_greased,
     0, 0, 0},
    {ngx_string("http_ssl_ja3"),
     NULL,
     ngx_http_ssl_fingerprint,
     0, 0, 0},
    {ngx_string("http_ssl_ja3_hash"),
     NULL,
     ngx_http_ssl_fingerprint_hash,
     0, 0, 0},
    {ngx_string("http2_fingerprint"),
     NULL,
     ngx_http_http2_fingerprint,
     0, 0, 0},
};

static ngx_int_t
ngx_http_ssl_fingerprint_init(ngx_conf_t *cf)
{

    ngx_http_variable_t *v;
    size_t l = 0;
    size_t vars_len;

    vars_len = (sizeof(ngx_http_ssl_fingerprint_variables_list) /
                sizeof(ngx_http_ssl_fingerprint_variables_list[0]));

    /* Register variables */
    for (l = 0; l < vars_len; ++l)
    {
        v = ngx_http_add_variable(cf,
                                  &ngx_http_ssl_fingerprint_variables_list[l].name,
                                  ngx_http_ssl_fingerprint_variables_list[l].flags);
        if (v == NULL)
        {
            continue;
        }
        *v = ngx_http_ssl_fingerprint_variables_list[l];
    }

    return NGX_OK;
}
