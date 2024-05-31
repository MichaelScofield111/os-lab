/*
 * @Author: MichealScofield
 * @Date:   2024-04-25 15:57:29
 */
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <algorithm>

#define NAME_LENGTH 1024

static int version{0}, show_pid{0}, sort_order{0};

struct Proc_Node{
    pid_t pid;
    pid_t* children;
    char name[NAME_LENGTH];
    int child_num;
};


void print_tree(Proc_Node* p, Proc_Node* procs, int depth, int show_pid) 
{
    if (depth > 0) {
		printf("\n%*s |\n", (depth - 1) * 4, "");
		printf("%*s", (depth - 1) * 4, "");
		printf(" +--");
	}

    printf("%s", p->name);
    if (show_pid) {
		printf("(%d)", p->pid);
	}

    for (int i = 0; i < p->child_num; i ++){
        Proc_Node* tmp = procs;
		while (tmp != NULL) {
            // find child of proc (p)
			if (tmp->pid == p->children[i]) {
				break;
			}
			++tmp;
		}
        print_tree(tmp, procs, depth + 1, show_pid);
    }
}

void get_all_procs_and_build_tree()
{   
    // get max_pid
    FILE* fp = fopen("/proc/sys/kernel/pid_max", "r");
    if(fp == NULL){
        perror("NO pid_max");
        exit(-1);
    }

    assert(fp);
    pid_t pid_max;
    fscanf(fp, "%d", &pid_max);
    fclose(fp);
    // malloc proc array store all procs
    int num_proc = std::min(32768, (int)pid_max); 
    Proc_Node* procs = (Proc_Node *)malloc((num_proc) * sizeof(Proc_Node)); 
    Proc_Node* p = procs;
    // malloc ppid array store all pid
    pid_t* ppids = (pid_t *)malloc(sizeof(pid_t) * pid_max);
    
    // open proc
    DIR* dir;
    if((dir = opendir("/proc")) == NULL){
        perror ("Cannot open .");
        exit (-1);
    }

    assert(dir);
    // read proc information
    struct dirent *dp;
    // Read proc directory.
    while ((dp = readdir(dir)) != NULL) {
        if(dp->d_name[0] >= '0' && dp->d_name[0] <= '9'){
            char proc_pid_status[512];
	    snprintf(proc_pid_status, sizeof(proc_pid_status),"/proc/%s/status", dp->d_name);
            FILE* fp = fopen(proc_pid_status, "r");
            assert(fp);
            char buf[1024];
            while ((fscanf(fp, "%s", buf) != EOF)) {
                if (strcmp(buf, "Name:") == 0) {
                    printf("%p", (void*)p->name);
                    fscanf(fp, "%s", p->name);
                }
                if (strcmp(buf, "Pid:") == 0) {
                    fscanf(fp, "%d", &p->pid);
                }
                if (strcmp(buf, "PPid:") == 0) {
                    fscanf(fp, "%d", &ppids[p - procs]);
                }
            }

            p++;
            fclose(fp);
            fp = NULL;
        }
    }   

    // all count of proc
    int proc_count = p - procs;
    assert(proc_count <= pid_max);
    printf("Total proc: %d\n", proc_count);
    //build the proc tree
    for (int i = 0; i < proc_count; i ++){
        procs[i].children = (pid_t *)malloc(sizeof(size_t) * proc_count);
        procs[i].child_num = 0;
    }

    for(int i = 0; i < proc_count; i ++){
        for(int j = 0; j < proc_count; j++){
            if(procs[i].pid == ppids[j]){
                // procs i is the father procs for i
                procs[i].children[procs[i].child_num ++] = procs[j].pid;
            }
        }
    }
    
    // print all procs
    for (int i = 0; i < proc_count; i ++){
        if(ppids[i] == 0) print_tree(&procs[i], procs, 0, show_pid);
    }
    
    printf("\n");
    free(procs);
    free(ppids);
    closedir(dir);
}

void execute()
{
    if(version){
        fprintf(stderr,"23.4\nCopyright (C) 1993-2024 Werner Almesberger and Craig Small\n");
        return;
    }

    get_all_procs_and_build_tree();
}

int main(int arc, char* argv[]){

    for(int i = 1; i < arc; i ++)
    {
        if(!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version"))
        {
            version = 1;
        }
        else if(!strcmp(argv[i],"-n") || !strcmp(argv[i],"--numeric-sort"))
        {
            sort_order = 1;
        }
        else if(!strcmp(argv[i],"-p") || !strcmp(argv[i], "--show-pids"))
        {
            show_pid = 1;
        }
        else{
             fprintf(stderr,"Usage: pstree [Options]...\n\t-V, --version\t\t"
                            "Display version information and exit.\n\t-p, "
                            "--show-pids\t\tShow PIDs.\n\t-n, --numeric-sort\t"
                            "Sort output by PID.\n");
        }
    }

    execute();

    return 0;
}

