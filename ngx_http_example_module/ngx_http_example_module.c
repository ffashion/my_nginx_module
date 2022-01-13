#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>
#include <stdio.h>
#include <unistd.h>




ngx_http_module_t ngx_http_example_module_ctx = {
      NULL, //preconfiguration
      NULL, //postconfiguration
      NULL, //create_main_conf
      NULL, //init_main_conf
      NULL, //create_srv_conf
      NULL, //merge_srv_conf
      NULL, //create_loc_conf
      NULL //merge_loc_conf
};
ngx_module_t ngx_http_example_module = {
    NGX_MODULE_V1,
    &ngx_http_example_module_ctx,//ctx
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