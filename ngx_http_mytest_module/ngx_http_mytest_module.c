#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>
#include <stdio.h>
#include <unistd.h>
//12345677

static ngx_int_t ngx_http_mytest_var_index;

ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r,
    void *data, ngx_int_t rc);
static void 
ngx_http_mytest_variables_set_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t 
ngx_http_mytest_variables_get_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);

char *ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t
ngx_http_mytest_add_variables(ngx_conf_t *cf);

static void *
ngx_http_mytest_create_loc_conf(ngx_conf_t *cf);

static ngx_int_t 
ngx_http_mytest_handler(ngx_http_request_t *r);

ngx_int_t ngx_http_mytest_init_modules(ngx_cycle_t *cycle);

typedef struct {
    ngx_str_t my_str;
    ngx_int_t my_num;
    ngx_flag_t my_flag;
    size_t my_size;
    ngx_array_t *my_str_array;
    ngx_array_t *my_keyval;
    off_t  my_off;
    ngx_msec_t my_msec;
    time_t my_sec;
    ngx_bufs_t my_bufs;
    ngx_uint_t my_enum_seq;
    ngx_uint_t my_bitmask;
    ngx_uint_t my_access;
    ngx_path_t *my_path;
    ngx_int_t var_index;
} ngx_http_mytest_conf_t;

typedef struct {
    ngx_str_t my_config_str;
    ngx_int_t my_config_num;
} ngx_http_mytest_config_conf_t;

static ngx_http_variable_t ngx_http_mytest_vaiable_var = {
    ngx_string("mytest_var"), //name /* must be first to build the hash */
    ngx_http_mytest_variables_set_handler, //set handler
    ngx_http_mytest_variables_get_handler, // get_handler
    0, //data
   NGX_HTTP_VAR_NOCACHEABLE,//flags
    0//index
};

//只要配置文件中有出现test 命令 才会执行set方法
static ngx_command_t ngx_http_mytest_cmds[]={
    {
        ngx_string("test"),
    //定义这个配置项出现的位置 以及
        NGX_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
    //检测到配置文件中 有test时 执行这个函数;
    //master 进程执行
        ngx_http_mytest,//set函数  处理配置项参数 或者给自己的子模块设置配置项目参数等
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_mytest_module_ctx = {
    ngx_http_mytest_add_variables, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_mytest_create_loc_conf, /* create location configuration */
    NULL /* merge location configuration */
};



ngx_module_t ngx_http_mytest_module = {
    NGX_MODULE_V1,
    &ngx_http_mytest_module_ctx,
    ngx_http_mytest_cmds,
    NGX_HTTP_MODULE,
    NULL, //init_master
    ngx_http_mytest_init_modules,//init_module
    NULL, //
    NULL, 
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};



//第二个是需要设置或者获取的变量值
//第三个是初始化时的回调指针
//flags 
// NGX_HTTP_VAR_CHANGEABLE表示这个变量是可变的.Nginx有很多内置变量是不可变的，比如arg_xxx这类变量，如果你使用set指令来修改，那么Nginx就会报错.
// NGX_HTTP_VAR_NOCACHEABLE表示这个变量每次都要去取值，而不是直接返回上次cache的值(配合对应的接口).
// NGX_HTTP_VAR_INDEXED表示这个变量是用索引读取的.
// NGX_HTTP_VAR_NOHASH表示这个变量不需要被hash.
static void ngx_http_mytest_variables_set_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data){

}

//当我们在get_handle中设置变量值的时候，只需要将对应的值放入到data中就可以了
//v 如果你需要自定义数据的话 那么v->data你需要自己 palloc一个空间 v本身是已经创建好了的
//data可以给变量设置除了r以外的值 也有可能是初始化的时候使用的
static ngx_int_t ngx_http_mytest_variables_get_handler(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data){

        v->data = ngx_palloc(r->pool,100);
        ngx_sprintf(v->data,"hello world");
        v->len = sizeof("hello world");
        v->valid = 1;
        v->not_found = 0;

    return NGX_OK;
}


void body_handler(ngx_http_request_t *r){


    //由于返回类型是void 所以需要主动结束请求
    ngx_http_finalize_request(r,0);
}

ngx_int_t ngx_http_mytest_init_modules(ngx_cycle_t *cycle){
    return 0;
}

static void *ngx_http_mytest_create_loc_conf(ngx_conf_t *cf){
    ngx_http_mytest_conf_t *mycf;
    mycf = (ngx_http_mytest_conf_t *)ngx_palloc(cf->pool,sizeof(ngx_http_mytest_conf_t));
    return mycf;
}

//
static ngx_int_t
ngx_http_mytest_add_variables(ngx_conf_t *cf){
   
    ngx_http_variable_t *var = NULL, *v = &ngx_http_mytest_vaiable_var;

    var = ngx_http_add_variable(cf,&v->name,v->flags);
    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = v->get_handler;
    var->data = v->data;

    //获取变量索引 方式1
    ngx_http_mytest_var_index = ngx_http_get_variable_index(cf,&v->name);

    return NGX_OK;

}



//worker进程执行
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r){
    // int fd = open("log1", O_RDWR | O_CREAT |O_APPEND);
    // if(write(fd,"start...\n",strlen("start...\n")) !=0){
    // }

    // if(r->method == NGX_HTTP_GET){
    //     write(fd,"GET\n",strlen("GET\n"));
    // }
    
    // write(fd,r->method_name.data,r->method_name.len);
    // write(fd,"\n",strlen("\n"));
    
    // write(fd,r->uri.data,r->uri.len);
    // write(fd,"\n",strlen("\n"));
    // write(fd,r->unparsed_uri.data,r->unparsed_uri.len);
    // write(fd,"\n",strlen("\n"));
    // write(fd,r->args.data,r->args.len);

    // //未经解析的http头
    // //r->header_in
    // //r->headers_in 存储了解析过的http 头
    // write(fd,r->headers_in.host->value.data,r->headers_in.host->value.len);
    // //可以通过遍历这个获取非标准http头
    // // r->headers_in.headers

    
    // // int rc = ngx_http_read_client_request_body(r,body_handler);
    // // if(rc >= NGX_HTTP_SPECIAL_RESPONSE){
    // //     return rc;
    // // }
    // ngx_table_elt_t *h = ngx_list_push(&r->headers_out.headers);
    // if(h == NULL){
    //     return NGX_ERROR;
    // }
    // h->hash = 1;
    // h->key.len = sizeof("TestHead") -1;
    // h->key.data = (u_char *)"TestHead";
    // h->value.len = sizeof("TestValue") -1;
    // h->value.data = (u_char *)"TestValue";

    // ngx_int_t rc = ngx_http_send_header(r);
    // if(rc == NGX_ERROR || rc == NGX_OK || r->header_only){
    //     return rc;
    // }
    ngx_http_post_subrequest_t *psr = ngx_palloc(r->pool,sizeof(ngx_http_post_subrequest_t));
    ngx_uint_t rc = -1;
    //子请求
    ngx_http_request_t *sr;

    if (psr == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    //子请求结束时会被调用
    psr->handler = mytest_subrequest_post_handler;

    //可以将其设置为module ctx
    psr->data = NULL;

    //构建一个全新的子请求
    //返回NGX_OK表示是合法的子请求 或者返回NGX_ERROR表示错误
    //NGX_HTTP_SUBREQUEST_IN_MEMORY 表示子请求的响应报文存储在r->upstream->buffer
    rc = ngx_http_subrequest(r,"/list/",NULL,sr,psr,NGX_HTTP_SUBREQUEST_IN_MEMORY);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    //必须返回NGX_DONE
    return NGX_DONE;
}
//
static ngx_int_t 
ngx_http_mytest_upstream(ngx_http_request_t *r) {
    ngx_http_mytest_conf_t *mycf;
    ngx_http_variable_value_t *v;
    mycf = ngx_http_get_module_loc_conf(r,ngx_http_mytest_module);
    
    // v = ngx_http_get_indexed_variable(r,mycf->var_index);
    // if(v && v->not_found == 1){
    //     return NGX_ERROR;
    // }

    //启动upstream机制
    ngx_http_upstream_init(r);

    //必须返回NGX_DONE 要求nginx框架不按照阶段继续向下处理请求了
    //请求停留在当前阶段，等待某个模块主动这个请求
    return NGX_DONE;
}


//master 进程执行
//set 函数
char *ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_http_core_loc_conf_t *clcf;
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_core_srv_conf_t  *cscf;
    ngx_http_mytest_conf_t *mycf;
    ngx_str_t name;


    clcf = ngx_http_conf_get_module_loc_conf(cf,ngx_http_core_module);
    mycf = ngx_http_conf_get_module_loc_conf(cf,ngx_http_mytest_module);

    // clcf->handler = ngx_http_mytest_upstream;
    clcf->handler = NULL;
    if(cf->name != NULL){
        printf("%s\n",cf->name);
    }
    name.data = ngx_palloc(cf->pool,ngx_strlen("mytest_var"));
    name.len = ngx_strlen("mytest_var");

    ngx_memcpy(name.data,"mytest_var",ngx_strlen("mytest_var"));

    //获取变量索引
    mycf->var_index =  ngx_http_get_variable_index(cf,&name);

    if (mycf->var_index <= -1) {
        return NGX_ERROR;
    }

    return NGX_OK;


}

//当前处于子请求
ngx_int_t mytest_subrequest_post_handler(ngx_http_request_t *r,
    void *data, ngx_int_t rc) {
        ngx_http_request_t *pr = r->parent;
        if (r->headers_out.status == NGX_HTTP_OK) {
            //子请求访问成功

            //r->upstream->buffer 当中得到响应
        }

        // pr->write_event_handler = mytest_post_handler;


}

