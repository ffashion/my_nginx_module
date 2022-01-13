#include <ngx_config.h>
#include <ngx_http.h>
#include <ngx_core.h>


ngx_int_t ngx_http_getconf_moudle_ctx_preconfiguration(ngx_conf_t *cf){
    static int i  = 1;
    printf("被调用%d 次\n",i);
    i++;
    return NGX_OK;
}
static void *
ngx_http_getconf_ctx_create_loc_conf(ngx_conf_t *cf);
static void *
ngx_http_getconf_ctx_create_main_conf(ngx_conf_t *cf);

static void *
ngx_http_getconf_ctx_create_srv_conf(ngx_conf_t *cf);

// static void *
// ngx_http_getconf_ctx_merge_loc_conf(ngx_conf_t *cf,void *prev, void *conf);

static ngx_int_t 
ngx_http_getconf_ctx_preconfigration(ngx_conf_t *cf);

static ngx_int_t 
ngx_http_getconf_ctx_postconfigration(ngx_conf_t *cf);

static ngx_int_t ngx_http_getconf_vars_get(ngx_http_request_t *r,
ngx_http_variable_value_t *v, uintptr_t data);

static void ngx_http_getconf_vars_set(ngx_http_request_t *r,
ngx_http_variable_value_t *v, uintptr_t data);

static ngx_int_t
ngx_http_debug_handler(ngx_http_request_t *r);

typedef struct {
    ngx_str_t name;
    ngx_log_t *log;
    ngx_file_t *log_file;
} ngx_http_getconf_module_loc_conf_t;


// #define NGX_HTTP_VAR_CHANGEABLE   1
// #define NGX_HTTP_VAR_NOCACHEABLE  2
// #define NGX_HTTP_VAR_INDEXED      4
// #define NGX_HTTP_VAR_NOHASH       8

//set and get handler 

// typedef void (*ngx_http_set_variable_pt) (ngx_http_request_t *r,
//     ngx_http_variable_value_t *v, uintptr_t data);
// typedef ngx_int_t (*ngx_http_get_variable_pt) (ngx_http_request_t *r,
//     ngx_http_variable_value_t *v, uintptr_t data);



static ngx_http_variable_t ngx_http_getconf_vars[] = {
    {
        ngx_string("hello"), //name
        ngx_http_getconf_vars_set, //set_handler
        ngx_http_getconf_vars_get, //get_handler
        0, //data
        NGX_HTTP_VAR_CHANGEABLE, //flags
        0 //index
    },
    ngx_http_null_variable
};



//各种回调函数
ngx_http_module_t ngx_http_getconf_module_ctx = {
      ngx_http_getconf_ctx_preconfigration, //preconfiguration
      ngx_http_getconf_ctx_postconfigration, //postconfiguration
      ngx_http_getconf_ctx_create_main_conf, //create_main_conf
      NULL, //init_main_conf
      ngx_http_getconf_ctx_create_srv_conf, //create_srv_conf
      NULL, //merge_srv_conf
      ngx_http_getconf_ctx_create_loc_conf, //create_loc_conf
      NULL //merge_loc_conf
};

//配置文件的所有出现的cmd和当前模块cmd一样会执行这个set
//set函数只需要为自己cmd 所需要的那个空间 赋值即可
char *ngx_http_getconf_module_cmds_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_getconf_module_loc_conf_t *glcf;
    glcf = conf;
    
    // int fd = open("log3",O_RDWR | O_CREAT |O_APPEND);

    // glcf->log->file


    //获取配置结构体 
    
    // ngx_http_cycle_get_module_main_conf(cf->cycle,ngx_http_getconf_module);

    // if(*(ngx_http_conf_ctx_t **)conf){
    //     return "is duplicate";
    // }
    
    if(conf == NULL){
        printf("conf is null\n");
    }
    printf("ngx_http_getconf_module_cmds_set :%x\n",conf);

    //用于11个阶段
    // ngx_http_get_module_main_conf(cf,ngx_core_module);

    //用于set函数
    cmcf = ngx_http_conf_get_module_main_conf(cf,ngx_http_core_module);
    


   
     //这个cmd_type是一个特定的type 这是因为这个type是从配置文件解析的 位置固定的
    // if(cf->cmd_type == NGX_HTTP_MAIN_CONF){
    //     printf("this is NGX_HTTP_MAIN_CONF\n");
    // }
    // ngx_str_t *name;
    // name = cf->args->elts;
    
    // if(name->data != NULL){
    //     printf("%s\n",name->data);
    // }
    // printf("%d\n",sizeof(*name));
    // printf("%d\n",cf->args->size);
    // name++;  
    // if(name->data != NULL){
    //     printf("%s\n",name->data);
    // }

    // printf("cf->args->nelts :%d\n",cf->args->nelts);

    // //这个cmd type是 各种type 或到一起的这是因为这是在源码中定义的
    // printf("%x\n",cmd->type);

    // if(cmd->name.data != NULL){
    //     printf("%s\n",cmd->name.data);
    // }
    

    // ngx_command_t *_cmd = NULL;
    // for(int i=0;cf->cycle->modules[i];i++){
    //     _cmd = cf->cycle->modules[i]->commands;

    //     if(write(STDOUT_FILENO,_cmd->name.data,_cmd->name.len)){

    //     }
    //     if(write(STDOUT_FILENO,"\n",1)){

    //     }
       
    // }
    return NGX_OK;
}

ngx_command_t ngx_http_getconf_module_cmds[]= {
    {
        ngx_string("getconf"),
        //type
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_TAKE1,
        ngx_http_getconf_module_cmds_set,
        //conf  CONF类型决定了寻址方式 虽然除了NGX_MAIN_CONF 和 NGX_DIRECT_CONF 都使用第三种寻址方式
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

ngx_module_t ngx_http_getconf_module = {
    NGX_MODULE_V1,
    &ngx_http_getconf_module_ctx,
    ngx_http_getconf_module_cmds,
    NGX_HTTP_MODULE,
    NULL, //init_master
    NULL, //init_module
    NULL, //init_process
    NULL, // init_thread
    NULL, //exit_thread
    NULL, //exit_process
    NULL, //exit_master
    NGX_MODULE_V1_PADDING
};


static void *
ngx_http_getconf_ctx_create_loc_conf(ngx_conf_t *cf){
    ngx_http_getconf_module_loc_conf_t *conf;

    conf = ngx_palloc(cf->pool,sizeof(ngx_http_getconf_module_loc_conf_t));

    // conf->name = NGX_CONF_UNSET;
    printf("ngx_http_getconf_ctx_create_loc_conf :%x\n",conf);

    return conf;
}
static void *
ngx_http_getconf_ctx_create_main_conf(ngx_conf_t *cf){
    ngx_http_getconf_module_loc_conf_t *conf;

    conf = ngx_palloc(cf->pool,sizeof(ngx_http_getconf_module_loc_conf_t));

    // conf->name = NGX_CONF_UNSET;
    printf("ngx_http_getconf_ctx_create_main_conf :%x\n",conf);

    return conf;
}

static void *
ngx_http_getconf_ctx_create_srv_conf(ngx_conf_t *cf){
    ngx_http_getconf_module_loc_conf_t *conf;


    conf = ngx_palloc(cf->pool,sizeof(ngx_http_getconf_module_loc_conf_t));

    // conf->name = NGX_CONF_UNSET;
    printf("ngx_http_getconf_ctx_create_srv_conf :%x\n",conf);

    return conf;
}

static ngx_int_t 
ngx_http_getconf_ctx_preconfigration(ngx_conf_t *cf){

    ngx_http_variable_t *v,*var;

    for(v=ngx_http_getconf_vars;v->name.len;v++) {
        //var现在已经挂到cmcf上了
        var = ngx_http_add_variable(cf,&v->name,v->flags);
        if(var == NULL){
            return NGX_ERROR;
        }
        var->get_handler = v->get_handler;
        var->data = v->data;
    }
    return NGX_OK;
}

static ngx_int_t 
ngx_http_getconf_ctx_postconfigration(ngx_conf_t *cf){
    
    ngx_http_core_main_conf_t  *cmcf;
    ngx_http_handler_pt        *h;

    // ngx_str_t name = ngx_string("remote_user");
    // ngx_int_t index = ngx_http_get_variable_index(cf,&name);
    // if(!index){
    //     return NGX_ERROR;
    // }
    // printf("remote_addr index :%d\n",index);
    // ngx_http_get_variable

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    
    if(h == NULL){
        return NGX_ERROR;
    }
    *h =  ngx_http_debug_handler;

    return NGX_OK;
}

static void ngx_http_getconf_vars_set(ngx_http_request_t *r,
ngx_http_variable_value_t *v, uintptr_t data){
    
    // volatile ngx_msec_t      ngx_current_msec;
    // volatile ngx_time_t     *ngx_cached_time;
    // volatile ngx_str_t       ngx_cached_err_log_time;
    // volatile ngx_str_t       ngx_cached_http_time;
    // volatile ngx_str_t       ngx_cached_http_log_time;
    // volatile ngx_str_t       ngx_cached_http_log_iso8601;
    // volatile ngx_str_t       ngx_cached_syslog_time;
    

}

static ngx_int_t ngx_http_getconf_vars_get(ngx_http_request_t *r,
ngx_http_variable_value_t *v, uintptr_t data){
    
    /*
        //ngx_http_variable_request 例子
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
    */

    return NGX_OK;
}

static ngx_int_t
ngx_http_debug_handler(ngx_http_request_t *r){
    // 如何在这里获取变量的值
    // r->variables 可以获取变量值
    //ngx_http_script_run  函数可以获取配置项的值
    // ngx_http_script_run
    ngx_http_core_main_conf_t    *cmcf;
    ngx_http_variable_t *variable;
    ngx_hash_keys_arrays_t *variables_hash;
    int index = 0;
    int i = 0;
    
    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);

    
    int fd = open("../logs/log3",O_RDWR | O_CREAT,0666);
    

    // dprintf(fd,"ngx_http_debug_handler\n");
    // if(write(fd,"ngx_http_debug_handler",strlen("ngx_http_debug_handler")) !=0){
    // }
    
    if(r->connection->log){
        dprintf(fd,"door\n");

        write(fd,r->connection->log->file->name.data,r->connection->log->file->name.len);
        if(r->variables->data){
            write(fd,r->variables->data,r->variables->len);
        }
        // write(fd,r->variables,sizeof(r->variables));
    }else{
        dprintf(fd,"fuck !!!\n");
    }
    
    //27个
    //$remote_addr 在这些里面
    variable = cmcf->variables.elts;
    variables_hash = cmcf->variables_keys;
    for(i=0; i < cmcf->variables.nelts; i++,variable++) {
        
        dprintf(fd,"variable %d is ",i);
        write(fd,variable->name.data,variable->name.len);
        
        if(0 == ngx_strncmp(variable->name.data,"myvar",variable->name.len)){
            dprintf(fd,"data value is %x\n",variable->data);
            dprintf(fd,"index value is %x\n",variable->index);
            write(fd,r->variables[variable->data].data,r->variables[variable->data].len);//获取变量值
            write(fd,r->variables[variable->index].data,r->variables[variable->data].len);
            dprintf(fd,"\n");
        }

        dprintf(fd,"\n");
       
    }
    // cmcf->variables_keys[1].

    dprintf(fd,"variables number is%ld\n",cmcf->variables.nelts);

    variable = cmcf->prefix_variables.elts;

    for(i=0; i < cmcf->prefix_variables.nelts; i++,variable++) {
        dprintf(fd,"variable %d is ",i);
        write(fd,variable->name.data,variable->name.len);
        dprintf(fd,"\n");
    }


    dprintf(fd,"prefix_variables number is %ld\n",cmcf->prefix_variables.nelts);
    
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http debug handler");

    // ngx_file_t
    // ngx_open_file();
    // r->connection->log 是../logs/debug.log
    
    //获取时间
    
    write(fd,ngx_cached_err_log_time.data,ngx_cached_err_log_time.len);

    
    ngx_log_error(NGX_LOG_DEBUG,r->connection->log,0,"hello world\n");
    // ngx_atomic_t
    // ngx_shm_t
    // ngx_accept_mutex
    // ngx_connection_counter
    // ngx_atomic_fetch_add()
    return NGX_OK;
}