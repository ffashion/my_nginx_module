#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>
#include <stdio.h>
#include <unistd.h>




static ngx_int_t 
ngx_http_upstreamtest_ctx_postconfigration(ngx_conf_t *cf);

static ngx_int_t
ngx_http_upstreamtest_headers_filter(ngx_http_request_t *r);


static ngx_int_t
ngx_http_upstreamtest_body_filter(ngx_http_request_t *r, ngx_chain_t *in);


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

ngx_http_module_t ngx_http_upstreamtest_module_ctx = {
      NULL, //preconfiguration
      ngx_http_upstreamtest_ctx_postconfigration, //postconfiguration
      NULL, //create_main_conf
      NULL, //init_main_conf
      NULL, //create_srv_conf
      NULL, //merge_srv_conf
      NULL, //create_loc_conf
      NULL //merge_loc_conf
};
ngx_module_t ngx_http_upstreamtest_module = {
    NGX_MODULE_V1,
    &ngx_http_upstreamtest_module_ctx,//ctx
    NULL, //commands
    NGX_HTTP_MODULE, //type
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static ngx_int_t 
ngx_http_upstreamtest_ctx_postconfigration(ngx_conf_t *cf){


    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_upstreamtest_headers_filter;


    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_upstreamtest_body_filter;

    return NGX_OK;
}

static ngx_int_t
ngx_http_upstreamtest_headers_filter(ngx_http_request_t *r){


    return ngx_http_next_header_filter(r);
}

static ngx_int_t
ngx_http_upstreamtest_body_filter(ngx_http_request_t *r, ngx_chain_t *in){

    

    return ngx_http_next_body_filter(r, in);
}