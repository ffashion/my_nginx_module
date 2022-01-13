/**
 * @file ngx_http_debug_module.c
 * @author your name (you@domain.com)
 * @brief a debug module 
 * @version 0.1
 * @date 2021-07-24
 * 
 * @copyright Copyright (c) 2021
 * 
 * 因为要记录时间戳 那么是不是要在11个所有阶段 都注册一遍
 * 
 * 服务器响应错误显示 
 *  
 */



#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>
#include "ngx_http_debug_module.h"

typedef struct{
    ngx_open_file_t *log_file;
    ngx_int_t log_level;
}ngx_http_debug_log_t;

typedef struct {
    ngx_int_t off; //是否关闭这个模块 的标志位
    ngx_http_debug_log_t *log;
    ngx_http_request_t *r ;//现在未使用 未来重构使用,用于减少debug_print 的参数
}ngx_http_debug_srv_conf_t;

typedef struct {
    ngx_str_t *rid; //请求id
    ngx_str_t *response_body;
}ngx_http_debug_request_ctx_t;

//专门用于字符串判断
static ngx_str_t err_levels[] = {
    ngx_null_string,
    ngx_string("emerg"),
    ngx_string("alert"),
    ngx_string("crit"),
    ngx_string("error"),
    ngx_string("warn"),
    ngx_string("notice"),
    ngx_string("info"),
    ngx_string("debug")
};

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static void *
ngx_http_debug_create_loc_conf(ngx_conf_t *cf);
static void *
ngx_http_debug_create_srv_conf(ngx_conf_t *cf);
static char *
ngx_http_debug_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t 
ngx_http_debug_ctx_postconfigration(ngx_conf_t *cf);
static ngx_int_t 
ngx_http_debug_post_read_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_post_server_rewrite_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_http_rewrite_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_http_server_rewrite_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_log_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_preaccess_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_access_phase_handler(ngx_http_request_t *r);
static ngx_int_t 
ngx_http_debug_content_phase_handler(ngx_http_request_t *r);

static ngx_int_t
ngx_http_debug_headers_filter(ngx_http_request_t *r);
static ngx_int_t
ngx_http_debug_body_filter(ngx_http_request_t *r, ngx_chain_t *in);


static char *
ngx_http_debug_set_log(ngx_conf_t *cf,ngx_command_t *cmd,void *conf);
static char *
ngx_http_debug_set_level(ngx_conf_t *cf,ngx_int_t *log_level);
static inline void 
ngx_http_debug_print(ngx_int_t log_level,ngx_http_debug_log_t *log,const char *fmt,...);
void get_request_body_cb(ngx_http_request_t *r);
ngx_str_t *
get_host_info(ngx_http_request_t *r);
ngx_str_t *
get_request_id(ngx_str_t *rid);
ngx_str_t *
get_http_cookie(ngx_http_request_t *r);
ngx_str_t *
get_request_method(ngx_http_request_t *r);
ngx_str_t *
get_request_body(ngx_http_request_t *r);
ngx_str_t * 
get_user_agent(ngx_http_request_t *r);
ngx_str_t * 
get_request_line(ngx_http_request_t *r);
ngx_str_t * 
get_request_headers(ngx_http_request_t *r);
ngx_str_t * 
get_response_headers(ngx_http_request_t *r);
ngx_str_t * 
get_response_body(ngx_http_request_t *r, ngx_chain_t *in);

ngx_command_t ngx_http_debug_module_cmds[]= {
    {
        ngx_string("debug_log"),
        //type
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE12,
        ngx_http_debug_set_log,//set method
        //conf  CONF类型决定了寻址方式 虽然除了NGX_MAIN_CONF 和 NGX_DIRECT_CONF 都使用第三种寻址方式
        NGX_HTTP_SRV_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_http_module_t ngx_http_debug_module_ctx = {
      NULL, //preconfiguration
      ngx_http_debug_ctx_postconfigration, //postconfiguration
      NULL, //create_main_conf
      NULL, //init_main_conf
      ngx_http_debug_create_srv_conf, //create_srv_conf
      NULL, //merge_srv_conf
      NULL, //create_loc_conf
      NULL //merge_loc_conf
};

ngx_module_t ngx_http_debug_module = {
    NGX_MODULE_V1,
    &ngx_http_debug_module_ctx,
    ngx_http_debug_module_cmds,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_debug_create_srv_conf(ngx_conf_t *cf){
    ngx_http_debug_srv_conf_t *dscf;

    dscf = ngx_palloc(cf->pool,sizeof(ngx_http_debug_srv_conf_t));
    if(dscf == NULL){
        return NGX_CONF_ERROR;
    }

    dscf->log =  ngx_palloc(cf->pool,sizeof(ngx_http_debug_log_t));
    if (dscf->log == NULL){
        return NGX_CONF_ERROR;
    }

    dscf->log->log_file = ngx_palloc(cf->pool,sizeof(ngx_open_file_t));
    if (dscf->log->log_file == NULL){
        return NGX_CONF_ERROR;
    }
    dscf->off = 0;
    dscf->log->log_file->fd = -1; //
    dscf->log->log_level = -1; //为了判断是否有重复指令 所以置-1

    return dscf;
}


static char *
ngx_http_debug_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child){

}
void 
get_request_body_cb(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf = NULL;
    ngx_str_t *rid = NULL,*p = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    rid = drct->rid;
    
    //现在r->request_body已经有值了
    p = get_request_body(r);
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V get_request_body_cb\n",rid);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_body: %V\n",rid,p);
    } else {
         ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_body: null\n",rid);
    }
    // ngx_http_finalize_request(r,NGX_OK);
    r->main->count--;
    return;
}

static void event_handler(ngx_event_t *ev){
    printf("hello world handler\n");
}
static ngx_event_t _event = {0};
static ngx_connection_t dumb_con = {0};
static ngx_int_t 
ngx_http_debug_ctx_postconfigration(ngx_conf_t *cf){
    ngx_http_core_main_conf_t  *cmcf;
    ngx_http_handler_pt        *h;
    cmcf = ngx_http_conf_get_module_main_conf(cf,ngx_http_core_module);

    //服务端已经完整接收到了客户端的请求头
    //永远是挂到后面
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    
    if( h == NULL) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_post_read_phase_handler;
    
        
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_SERVER_REWRITE_PHASE].handlers);
    if ( h == NULL ) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_http_server_rewrite_phase_handler;

    
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if ( h == NULL ) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_http_rewrite_phase_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PREACCESS_PHASE].handlers);
    if ( h == NULL ) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_preaccess_phase_handler;


    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if ( h == NULL ) {
        return NGX_ERROR;
    }
    
    *h = ngx_http_debug_access_phase_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
    if (h == NULL ) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_content_phase_handler;


    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if ( h == NULL ) {
        return NGX_ERROR;
    }
    *h = ngx_http_debug_log_phase_handler;


    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_debug_headers_filter;


    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_debug_body_filter;
    
   
    // ngx_event_add_timer(&_event,timer);

    return NGX_OK;

}
//刚读取完头之后
static ngx_int_t 
ngx_http_debug_post_read_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *rid = NULL,*p = NULL;
    ngx_int_t rc = 0;

    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }

    drct = ngx_palloc(r->pool,sizeof(ngx_http_debug_request_ctx_t));
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    drct->rid = ngx_palloc(r->pool,sizeof(ngx_str_t));
    if (drct->rid == NULL){
        return NGX_DECLINED;
    }

    drct->rid->data = ngx_palloc(r->pool,32);
    drct->rid->len = 32;
    if (drct->rid->data == NULL) {
        return NGX_DECLINED;
    }
    ngx_http_set_ctx(r,drct,ngx_http_debug_module);

    
    //请求id 是随机的 每次获取一个请求id 都是不一样的 所以要保存在一个地方
    rid = drct->rid;
    (void)get_request_id(rid); //post read阶段执行一次 以后不需要执行
    
    if (!rid) {
        return NGX_DECLINED;
    }
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%v post_read_phase\n",rid);

    p =  get_host_info(r);
    //ngx_http_debug_print 其实是ngx_vslprintf 不能判断p为null
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V %V",rid,p);
    }

    p = get_request_method(r);
    if (p){
       ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_method: %V\n",rid,p);
    }

    if(r->uri.len > 0){
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V uri: %V\n",rid,&r->uri);
    }else {
         ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V uri: null\n",rid);
    }

    if(r->args.len > 0) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_args: %V\n",rid,&r->args);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_args: null\n",rid);
    }

    p = get_http_cookie(r);    
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V http_cookie: %V\n",rid,p);
    } else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V http_cookie: null\n",rid,p);
    }

    p = get_request_line(r);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_line: %V\n",rid,p);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_line: null%V\n",rid);
    }

    p = get_request_headers(r);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V all_headers: %V\n",rid,p);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V all_headers: null%V\n",rid);
    }
    //同步读取请求头，后面改为异步的
    //get_request_body_cb 请求体读完会被调用
    //如果conetn_len<0 也会被调用。丢弃也会被调用。
    //如果你不在get_request_body_cb里面结束请求的话需要将r->main->count-- 否则日志阶段就不执行了
    //结束请求本身会执行一次r->main->count--
    //get_request_body_cb 为模块的逻辑处理函数
    rc = ngx_http_read_client_request_body(r,get_request_body_cb);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    
    _event.handler = event_handler;
    _event.data = &dumb_con;
    dumb_con.fd = (ngx_socket_t) -1;
    ngx_msec_t timer = 5;
    
    ngx_add_timer(&_event,timer);

    //继续下一个post read
    return NGX_DECLINED;
}


static ngx_int_t 
ngx_http_debug_http_server_rewrite_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);

    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;

    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V server_rewrite_phase\n",rid);
    
    //当前处理完毕 下一个rewrite模块执行
    return NGX_DECLINED;
}

//conf_phase 
static ngx_int_t 
ngx_http_debug_http_rewrite_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }

    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;

    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V rewrite_phase \n",rid);
    
    //当前处理完毕 下一个rewrite模块执行
    return NGX_DECLINED;
}

static ngx_int_t 
ngx_http_debug_preaccess_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V preaccess_phase\n",rid);
    return NGX_DECLINED;
}

static ngx_int_t 
ngx_http_debug_access_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }
    //这个阶段会进来2次 第二次进来  drct为NULL
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;

    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V access_phase\n",rid);
    return NGX_DECLINED;
}

//反代之后这个不会生效 因为反代的content checker直接返回NGX_OK了
static ngx_int_t 
ngx_http_debug_content_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf= NULL;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V ngx_http_debug_content_phase_handler\n",rid);
    
    return NGX_DECLINED;
}

//log 不执行和r->main->count 有关 
static ngx_int_t 
ngx_http_debug_log_phase_handler(ngx_http_request_t *r){
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf = NULL;
    ngx_str_t *p = NULL,*rid = NULL;
    //根据module的索引字段（ctx_index），找到 request 所请求的 location 配置
    //如果加上了rewrite 则dscf 会被跳转到其他location
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);//drct == NULL
    // int i = ngx_http_debug_module.ctx_index;//0x23
    if (dscf->off || dscf->log->log_file->fd < 0){
        return NGX_DECLINED;
    }
    void *modsec_ctx = r->ctx[0x28];//有的。
    
    //我的日志是执行了的 但是drct为空 明明我这个先执行的但是我这个ctx没有值 modsec的ctx就有值
    if (drct == NULL) {
        return NGX_DECLINED;
    }
    rid = drct->rid;
    
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V log_phase start \n",rid);
    
    //这里肯定有问题
    // p = get_user_agent(r);
    // if (p) {
    //     ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V user_agent: %V\n",rid,p);
    // }else {
    //     //这里出现了问题
    //     ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V user_agent: null%V\n",rid);
    // }

    p = get_request_line(r);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_line: %V\n",rid,p);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_line: null%V\n",rid);//有问题
    }

    p = get_request_headers(r);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V all_headers: %V\n",rid,p);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V all_headers: null%V\n",rid);
    }

    p = get_request_body(r);
    if (p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_body: %V\n",rid,p);
    } else {
         ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V request_body: null\n",rid);
    }
    

    

    return NGX_DECLINED;
}


//处理响应头
static ngx_int_t
ngx_http_debug_headers_filter(ngx_http_request_t *r){
    //modsecurity 是在gzip之前执行的 
    // r->headers_out.headers
    //r->headers_out.content_type
    // r->headers_out.status //响应状态
    // r->headers_out.status_line;
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf= NULL;
    ngx_str_t *p = NULL,*rid = NULL;
    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);
    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    if (dscf->off || dscf->log->log_file->fd < 0){
        return ngx_http_next_header_filter(r);
    }
    if(drct == NULL){
        return ngx_http_next_header_filter(r);
    }
    rid = drct->rid;
    if(!rid) {
        return ngx_http_next_header_filter(r);
    }

    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V headers_filter\n",rid);
    p = get_response_headers(r);
    if(p) {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V response_headers %V\n",rid,p);
    }else {
        ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V response_headers null\n",rid);
    }

    // ngx_http_post_request()

    return ngx_http_next_header_filter(r);
}
//处理响应体
//响应体以块的形式交付 所以body filter可能会被调用多次
static ngx_int_t
ngx_http_debug_body_filter(ngx_http_request_t *r, ngx_chain_t *in){
    ngx_chain_t *chain = NULL;
    ngx_http_debug_request_ctx_t *drct= NULL;
    ngx_http_debug_srv_conf_t *dscf= NULL;
    ngx_str_t *p = NULL,*rid = NULL;

    if(in == NULL){
        return ngx_http_next_body_filter(r,in);
    }

    dscf = ngx_http_get_module_srv_conf(r,ngx_http_debug_module);

    drct = ngx_http_get_module_ctx(r,ngx_http_debug_module);
    
    if (dscf->off || dscf->log->log_file->fd < 0 || !drct){
        return ngx_http_next_body_filter(r,in);
    }

    rid = drct->rid;
    if(!drct->rid) {
        return ngx_http_next_body_filter(r, in);
    }
    
    ngx_http_debug_print(NGX_HTTP_DEBUG_DEBUG,dscf->log,"%V body_filter start\n",rid);

    // for(chain = in;chain; chain = chain->next){
    //     u_char *pos = chain->buf->pos;
    //     size_t n = chain->buf->last - chain->buf->pos;
    //     ngx_write_fd(dscf->log->log_file->fd,pos,n);
    // }
    
    //这个链表的入口函数是ngx_http_output_filter
    return ngx_http_next_body_filter(r, in);
}

static char *
ngx_http_debug_set_log(ngx_conf_t *cf,ngx_command_t *cmd,void *conf){
    ngx_http_debug_srv_conf_t *dscf;
    ngx_str_t *value;

    dscf = conf;
    
    if (dscf->log->log_level >= 0){
        return "is duplicate";
    }
    
    //判断重复
    dscf->log->log_level = 0;

    value = cf->args->elts;
    //第一个参数 off or path
    //指令必须至少带一个参数 所以不需要判断 这是设置的
    if(0 == ngx_strncmp(value[1].data,"off",value[1].len)){
        dscf->off = 1;
        if(cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }else{
            return NGX_CONF_ERROR;
        }
    }
    //ngx_conf_open_file 只是把文件挂到全局变量上 等后面一次性打开
    dscf->log->log_file = ngx_conf_open_file(cf->cycle,&value[1]);

    if (dscf->log->log_file == NULL){
        return NGX_CONF_ERROR;
    }

    if (ngx_http_debug_set_level(cf,&dscf->log->log_level) != NGX_CONF_OK){
        return NGX_CONF_ERROR;
    }
    return NGX_CONF_OK;
}

static char *
ngx_http_debug_set_level(ngx_conf_t *cf,ngx_int_t *log_level){
    ngx_str_t *value;
    ngx_int_t i;

    if (cf->args->nelts == 2){
        *log_level = NGX_HTTP_DEBUG_DEBUG;
        return NGX_CONF_OK;
    }

    value = cf->args->elts;

    if(cf->args->nelts == 3) {

        for(i = 1;i <= NGX_HTTP_DEBUG_DEBUG; i++){
            if (ngx_strcmp(value[2].data,err_levels[i].data) == 0){
                *log_level = i ;
                return NGX_CONF_OK;
            }
        }
        return NGX_CONF_ERROR;
    }
    return NGX_CONF_ERROR;
}




static inline void 
ngx_http_debug_print(ngx_int_t log_level,ngx_http_debug_log_t *log,const char *fmt,...){
    va_list args;
    ngx_uint_t n;
    u_char *p, *last;
    
    //单条日志最大字节数目
    u_char errstr[NGX_MAX_ERROR_STR];
    ngx_time_t  *tp;
    
    if (log->log_level < 0){
        return ;
    }
    if (log->log_level < log_level){
        return;
    }
    
    last = errstr + NGX_MAX_ERROR_STR;

    //更新时间
    ngx_time_update();
    p = ngx_cpymem(errstr,ngx_cached_err_log_time.data,
    ngx_cached_err_log_time.len);

    
    tp = ngx_timeofday();
    p = ngx_slprintf(p,last,":%03M ",tp->msec);
    
    //日志等级
    p = ngx_slprintf(p, last, " [%V] ", &err_levels[log_level]);
    
    //pid
    p = ngx_slprintf(p, last, "%P#" NGX_TID_T_FMT ": ",
                    ngx_log_pid, ngx_log_tid);


    va_start(args,fmt);
//从p 开始最大为last 
    p = ngx_vslprintf(p, last, fmt, args);
//返回的p 为有空间的位置
    va_end(args);

    n = ngx_write_fd(log->log_file->fd, errstr, p - errstr);
}


