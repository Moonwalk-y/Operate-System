#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>

void* compile_and_load(const char* code){
    char temp_path[] = "/tmp/src_XXXXXX";
    int fd = mkstemp(temp_path);
    write(fd,code,strlen(code));
    close(fd);

    char so_path[] = "/tmp/so_XXXXXX";
    int fd_so = mkstemp(so_path);
    close(fd_so);

    pid_t pid = fork();
    if(pid == -1){
        fprintf(stderr, "fork fail\n");
        return NULL;
    }else if(pid == 0){
        execlp("gcc","gcc","-x","c","-fPIC","-shared",temp_path,"-o",so_path,(char*)NULL);
    }else{
        int status;
        waitpid(pid, &status, 0);
        if(WEXITSTATUS(status) != 0){
            fprintf(stderr, "GCC fail\n");
            return NULL;
        }
    }

    void *handle = dlopen(so_path, RTLD_LAZY | RTLD_GLOBAL);

    unlink(temp_path);
    unlink(so_path);
    return handle;
}

// Compile a function definition and load it
bool define_function(const char* function_def, char func_list[],size_t *count) {
    char code[4000];
    char func_design[2000];
    for(int i = 0;i < *count;i++){
        func_design[i] = func_list[i];
    }
    func_design[*count] = '\0';
    sprintf(code,"%s\n%s",func_design,function_def);
    void* handle = compile_and_load(code);
    if(!handle) return false;

    int i = 0;
    while(function_def[i] != '\0' && function_def[i] != '{'){//构造当前函数的签名，写入func_list
        func_list[(*count)++] = function_def[i++];
    }
    func_list[(*count)++] = ';';

    return true;
}

// Evaluate an expression
bool evaluate_expression(const char* expression, int* result, int *number,const char func_list[],const size_t *count) {
    char wrapper_func[2000];
    sprintf(wrapper_func,"int __expr_wrapper_%d() {return (%s);}\n",*number,expression);
    (*number)++;
    char code[3000];
    char func_design[2000];
    for(int i = 0;i < *count;i++){
        func_design[i] = func_list[i];
    }
    func_design[*count] = '\0';
    sprintf(code,"%s\n%s",func_design,wrapper_func);

    void* handle = compile_and_load(code);
    if(!handle) return false;

    char func_name[100];
    sprintf(func_name,"__expr_wrapper_%d",(*number)-1);
    int (*run_me)() = dlsym(handle,func_name);
    if(run_me){
        *result = run_me();
        dlclose(handle);
        return true;
    }
    dlclose(handle);
    return false;
}

int main() {
    char line[1000];
    char func_list[2000];
    size_t count = 0;
    int number = 0;

    while(1){
        printf("crepl> ");
        fflush(stdout);
        if(!fgets(line,sizeof(line),stdin)) break;

        if(line[0] == 'i' && line[1] == 'n' && line[2] == 't'){
            if(define_function(line,func_list,&count)) printf("OK\n");
            else printf("compile error\n");
        }else{
            int result;
            if(evaluate_expression(line,&result,&number,func_list,&count)){
                printf("%d\n",result);
            }else{
                printf("expression error\n");
            }
        }
    }

    return 0;
}
