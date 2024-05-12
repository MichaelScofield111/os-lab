#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/wait.h>
#include <errno.h>  
#include <stdbool.h>
#include <stdlib.h>

// define init data
static int expr_count; 
static char *wrapper_func[] = {
	"int ", "__expr_wrapper_", "(){return ", ";}\n",
};
static FILE *fp;

int main(int argc, char *argv[]) {
    static char line[4096];

    fp = fopen("/tmp/a.c", "w");
	fclose(fp);
    
    while (1) {
        // init 
        bool is_expr = false;

        printf("crepl> ");
        fflush(stdout);
        
        // get input to line[4096];
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
        
        // open a temp file to  write line[4096].
        // Defensive programming.
        // check is the function(int).
        if (strncmp(line, "int ", 4) == 0) {
            fp = fopen("/tmp/a.c", "a");
            if (fp == NULL) {
            fprintf(stderr, "fopen error: %s\n", strerror(errno));
            exit(-1);
        }
            fprintf(fp, "\n%s\n", line);
            fclose(fp);
        }
        else{
            // is a expr to build a wrapper.
            fp = fopen("/tmp/a.c", "a");
            if (fp == NULL) {
                fprintf(stderr, "fopen error: %s\n", strerror(errno));
                exit(-1);
            }

            is_expr = true;
            fprintf(fp, "%s%s%d%s%s%s",
					wrapper_func[0], wrapper_func[1], expr_count, wrapper_func[2], line, wrapper_func[3]);
            fclose(fp);
            // build the liba.so
            char* exec_gcc[] = {
                "gcc",
                "-fPIC",
                "-shared",
                "/tmp/a.c",
                "-o",
                "/tmp/liba.so",
                NULL,
            };

            if(fork() == 0) {
                execvp("gcc", exec_gcc);
                perror("gcc error");
                exit(-1);
            }
            
            wait(NULL);

            // create other bash to execve.
            if (is_expr) {
                void *handle = dlopen("/tmp/liba.so", RTLD_LAZY);
                if(!handle)
                {
                    fprintf(stderr, "%s\n", dlerror());
                    exit(-1);
                }
            //clean fault
            dlerror();
            // get function address.
            char expr_name[100];
            sprintf(expr_name, "%s%d", wrapper_func[1], expr_count);
            int (*func)() = (int (*)()) dlsym(handle, expr_name);
            if (func == NULL){
                fprintf(stderr, "dlsym error: %s\n", dlerror());
                dlclose(handle);
                exit(-1);
            }
            
            printf("%d\n", func());
            dlclose(handle);
            expr_count ++;
            is_expr = 0;
            }
        }
    }
}
        



