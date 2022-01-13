#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>
#include "ngx_http_debug_module.h"

#define NGX_MAX_HEADER_STR 20480
static ngx_inline ngx_str_t *
v2ns(ngx_http_request_t *r,ngx_http_variable_value_t *v);

static ngx_int_t
ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_request_id(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_header(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data);
static ngx_int_t
ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_argument(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data);

static ngx_int_t
ngx_http_variable_request(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data);
static ngx_int_t
ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep);
static ngx_int_t
ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
ngx_str_t *
get_request_body(ngx_http_request_t *r);


static ngx_int_t
ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);



static ngx_int_t
ngx_http_variable_server_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  port;

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ngx_connection_local_sockaddr(r->connection, NULL, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    port = ngx_inet_get_port(r->connection->local_sockaddr);

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_variable_server_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_str_t  s;
    u_char     addr[NGX_SOCKADDR_STRLEN];

    s.len = NGX_SOCKADDR_STRLEN;
    s.data = addr;

    if (ngx_connection_local_sockaddr(r->connection, &s, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    s.data = ngx_pnalloc(r->pool, s.len);
    if (s.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(s.data, addr, s.len);

    v->len = s.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s.data;

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_remote_addr(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    v->len = r->connection->addr_text.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = r->connection->addr_text.data;

    return NGX_OK;
}
static ngx_int_t
ngx_http_variable_remote_port(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_uint_t  port;

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    v->data = ngx_pnalloc(r->pool, sizeof("65535") - 1);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    port = ngx_inet_get_port(r->connection->sockaddr);

    if (port > 0 && port < 65536) {
        v->len = ngx_sprintf(v->data, "%ui", port) - v->data;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_variable_request_line(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *p, *s;

    s = r->request_line.data;

    if (s == NULL) {
        s = r->request_start;

        if (s == NULL) {
            v->not_found = 1;
            return NGX_OK;
        }

        for (p = s; p < r->header_in->last; p++) {
            if (*p == CR || *p == LF) {
                break;
            }
        }

        r->request_line.len = p - s;
        r->request_line.data = s;
    }

    v->len = r->request_line.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = s;
    return NGX_OK;
}


//获取某个头
static ngx_int_t
ngx_http_variable_header(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_table_elt_t  *h;

    h = *(ngx_table_elt_t **) ((char *) r + data);

    if (h) {
        v->len = h->value.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = h->value.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_variable_request(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    ngx_str_t  *s;
    
    s = (ngx_str_t *) ((char *) r + data);

    if (s->data) {
        v->len = s->len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = s->data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}




static ngx_int_t
ngx_http_variable_cookies(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_headers_internal(r, v, data, ';');
}

static ngx_int_t
ngx_http_variable_headers_internal(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data, u_char sep)
{
    size_t             len;
    u_char            *p, *end;
    ngx_uint_t         i, n;
    ngx_array_t       *a;
    ngx_table_elt_t  **h;

    a = (ngx_array_t *) ((char *) r + data);

    n = a->nelts;
    h = a->elts;

    len = 0;

    for (i = 0; i < n; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        len += h[i]->value.len + 2;
    }

    if (len == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len -= 2;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (n == 1) {
        v->len = (*h)->value.len;
        v->data = (*h)->value.data;

        return NGX_OK;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->len = len;
    v->data = p;

    end = p + len;

    for (i = 0; /* void */ ; i++) {

        if (h[i]->hash == 0) {
            continue;
        }

        p = ngx_copy(p, h[i]->value.data, h[i]->value.len);

        if (p == end) {
            break;
        }

        *p++ = sep; *p++ = ' ';
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_variable_request_method(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    if (r->main->method_name.data) {
        v->len = r->main->method_name.len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = r->main->method_name.data;

    } else {
        v->not_found = 1;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_variable_msec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 4);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T.%03M", tp->sec, tp->msec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}

static ngx_int_t
ngx_http_variable_request_body(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char       *p;
    size_t        len;
    ngx_buf_t    *buf;
    ngx_chain_t  *cl;
    //r->request_body==NULL
    if (r->request_body == NULL
        || r->request_body->bufs == NULL
        || r->request_body->temp_file)
    {
        v->not_found = 1;

        return NGX_OK;
    }

    cl = r->request_body->bufs;
    buf = cl->buf;

    if (cl->next == NULL) {
        v->len = buf->last - buf->pos;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;
        v->data = buf->pos;

        return NGX_OK;
    }

    len = buf->last - buf->pos;
    cl = cl->next;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        len += buf->last - buf->pos;
    }

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;
    cl = r->request_body->bufs;

    for ( /* void */ ; cl; cl = cl->next) {
        buf = cl->buf;
        p = ngx_cpymem(p, buf->pos, buf->last - buf->pos);
    }

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}



static ngx_inline ngx_str_t *
v2ns(ngx_http_request_t *r,ngx_http_variable_value_t *v){
        ngx_str_t *ret = NULL;
        ret = ngx_palloc(r->pool,sizeof(ngx_str_t));
        if (ret == NULL){
            return NULL;
        }
        if (v == NULL || v->data == NULL || v->len == 0){
            return NULL;
        }
        ret->data = v->data;
        ret->len = v->len;
        return ret;
}



//host info  cookie信息 request id / request method
ngx_str_t *
get_host_info(ngx_http_request_t *r){
    ngx_http_variable_value_t **v;
    ngx_str_t *p;
    u_char *last;
    u_char *next;
    ngx_int_t i;
    ngx_uint_t len;

    p = ngx_palloc(r->pool,sizeof(ngx_str_t));
    if(p == NULL) {
        return NULL;
    }

    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t *) * 4);
    if ( v == NULL) {
        return NULL;
    }
    
    for(i=0; i <=3 ;i++){
        v[i] = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
        if (v[i] == NULL){
            return NULL;
        }
    }

    if (ngx_http_variable_server_addr(r,v[0],0) != NGX_OK) {
        return NULL;
    }
    
    if ( ngx_http_variable_server_port(r,v[1],0) != NGX_OK){
        return NULL;
    }

    if ( ngx_http_variable_remote_addr(r,v[2],0) != NGX_OK){
        return NULL;
    }

    if ( ngx_http_variable_remote_port(r,v[3],0) != NGX_OK){
        return NULL;
    }


    for (i=0; i<=3; i++ ) {
        if (v[i]->not_found == 1) {
            return NULL;
        }
    }

    len = sizeof("client: : ") -1 + v[0]->len + v[1]->len + v[2]->len + v[3]->len + sizeof("server: :") -1 + sizeof("\n") - 1;

    p->data = ngx_palloc(r->pool,len);

    if (p->data == NULL) {
        return NULL;
    }
    p->len = len;

    last = p->data + len;

    next = ngx_slprintf(p->data,last,"server: %V:%V ",v2ns(r,v[0]), v2ns(r,v[1]));

    next = ngx_slprintf(next,last,"client: %V:%V\n",v2ns(r,v[2]), v2ns(r,v[3]));

    return p;
}

ngx_str_t *
get_http_cookie(ngx_http_request_t *r){
    ngx_http_variable_value_t *v;

    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
    if (v == NULL) {
        return NULL;
    }
    if ( ngx_http_variable_cookies(r,v,offsetof(ngx_http_request_t, headers_in.cookies)) != NGX_OK) {
        return NULL;
    }
    return v->not_found ? NULL :v2ns(r,v);
}


ngx_str_t *
get_request_id(ngx_str_t *rid){
    
    if (rid == NULL || rid->data == NULL) {
        return NULL;
    }

    u_char  *id = rid->data;
    ngx_sprintf(id, "%08xD%08xD%08xD%08xD",
                (uint32_t) ngx_random(), (uint32_t) ngx_random(),
                (uint32_t) ngx_random(), (uint32_t) ngx_random());
    return rid;
}

ngx_str_t *
get_request_method(ngx_http_request_t *r){
    ngx_http_variable_value_t *v = NULL;
    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
    if (v == NULL) {
        return NULL;
    }
    if ( ngx_http_variable_request_method(r,v,0) != NGX_OK) {
        return NULL;
    }

    if (v->not_found == 1) {
        return NULL;
    }
    
    return v->not_found ? NULL :v2ns(r,v);

}


ngx_str_t * 
get_user_agent(ngx_http_request_t *r){
    ngx_http_variable_value_t *v = NULL;
    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
    if (v == NULL) {
        return NULL;
    }
    if (ngx_http_variable_header(r,v, offsetof(ngx_http_request_t, headers_in.user_agent)) != NGX_OK) {
        return NULL;
    }
    return v->not_found ? NULL :v2ns(r,v);
}
ngx_str_t * 
get_request_line(ngx_http_request_t *r){
    ngx_http_variable_value_t *v = NULL;
    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
    if (v == NULL) {
        return NULL;
    }
    if (ngx_http_variable_request_line(r,v,0) != NGX_OK){
        return NULL;
    }

    return v->not_found ? NULL :v2ns(r,v);
}

ngx_str_t * 
get_request_headers(ngx_http_request_t *r){
    //存储着所有的请求头
    // r->headers_in.headers
    ngx_str_t *ret = NULL;
    u_char *p = NULL, *last = NULL;
    ngx_list_part_t *part = NULL;
    ngx_table_elt_t *headers = NULL;
    ngx_uint_t i = 0;
    ret = ngx_palloc(r->pool,sizeof(ngx_str_t));
    ret->data = ngx_palloc(r->pool,NGX_MAX_HEADER_STR);
    part = &r->headers_in.headers.part;
    headers = part->elts; //某些header
    p = ret->data;//初始化p 用于循环
    for(i=0;/*void*/;i++){
        if(i >= part->nelts){
            if (!part->next){
                break;
            }
            //替换part
            part = part->next;
            headers = part->elts;
            i = 0;
        }

        if(headers[i].hash == 0){
            continue;
        }

        p = ngx_cpymem(p,"\"",sizeof("\""));
        p = ngx_cpymem(p,headers[i].key.data,headers[i].key.len);
        p = ngx_cpymem(p,": ",sizeof(": "));
        p = ngx_cpymem(p,headers[i].value.data,headers[i].value.len);
        p = ngx_cpymem(p,"\"",sizeof("\""));
        p = ngx_cpymem(p," ",sizeof(" "));
    }
    ret->len = p - ret->data;
    if(ret->len  == 0){
        return NULL;
    }

    return ret;
}



ngx_str_t *
get_request_body(ngx_http_request_t *r) {
    ngx_http_variable_value_t *v = NULL;
    v = ngx_palloc(r->pool,sizeof(ngx_http_variable_value_t));
    if (v == NULL) {
        return NULL;
    }
    if ( ngx_http_variable_request_body(r,v,0) != NGX_OK) {
        return NULL;
    }
    return v->not_found ? NULL :v2ns(r,v);

}

ngx_str_t * 
get_response_headers(ngx_http_request_t *r){
    ngx_str_t *ret = NULL;
    u_char *p = NULL, *last = NULL;
    ngx_list_part_t *part = NULL;
    ngx_table_elt_t *headers = NULL;
    ngx_uint_t i = 0;
    ret = ngx_palloc(r->pool,sizeof(ngx_str_t));
    ret->data = ngx_palloc(r->pool,NGX_MAX_HEADER_STR);
    part = &r->headers_out.headers.part;
    headers = part->elts; //某些header
    p = ret->data;//初始化p 用于循环 存储结果
    for(i=0;/*void*/;i++){
        if(i >= part->nelts){
            if (!part->next){
                break;
            }
            //替换part
            part = part->next;
            headers = part->elts;
            i = 0;
        }

        if(headers[i].hash == 0){
            continue;
        }

        p = ngx_cpymem(p,"\"",sizeof("\""));
        p = ngx_cpymem(p,headers[i].key.data,headers[i].key.len);
        p = ngx_cpymem(p,": ",sizeof(": "));
        p = ngx_cpymem(p,headers[i].value.data,headers[i].value.len);
        p = ngx_cpymem(p,"\"",sizeof("\""));
        p = ngx_cpymem(p," ",sizeof(" "));
    }
    ret->len = p - ret->data;
    if(ret->len  == 0){
        return NULL;
    }

    return ret;
}
//in 一定有值
ngx_str_t * 
get_response_body(ngx_http_request_t *r, ngx_chain_t *in){
    ngx_str_t *ret = NULL;
    ret = ngx_palloc(r->pool,sizeof(ngx_str_t));
    if(in->buf->in_file){
        return NULL;
    }
    

    ret->data = in->buf->pos;
    ret->len = in->buf->last - in->buf->pos;


    if(!ret->len){
        return NULL;
    }

    return ret;
}