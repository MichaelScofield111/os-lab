#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#define MAX_PROC 1024

static int version{0}, show_pid{0} , sort_order{0};

// 我们需要把可以记录子进程的pid
struct Define_proc
{
    pid_t pid;
    pid_t fpid;
    short cpid[MAX_PROC];
    short cpid_num;
    char name[NAME_MAX];
};

struct Define_proc* hash_table[MAX_PROC]{NULL};

int hash(pid_t id)
{
    return id % MAX_PROC;
}

void insert(Define_proc* proc)
{
    int x{hash(proc->pid)};
    while (hash_table[x] != NULL)
    {
        x ++;
        if(x == MAX_PROC) x = 0;
    }

    hash_table[x] = proc;

}

Define_proc* get_proc(pid_t pid)
{
    int x{hash(pid)};
    while (hash_table[x]->pid != pid)
    {
        x ++;
        if(x == MAX_PROC) x = 0;
    }

    return hash_table[x];
}

short get_idx(pid_t pid)
{
    int x = hash(pid);
    while (hash_table[x]->pid != pid)
    {
        x ++;
        if(x == MAX_PROC) x = 0;
    }
    return x;
}


void get_all_procs()
{
    DIR *proc_dir = opendir("/proc");
    struct dirent *proc_entry;
    while ((proc_entry = readdir(proc_dir)))
    {
        // get /proc all content;
        if (proc_entry->d_name[0] >= '0' && proc_entry->d_name[0] <= '9')
        {
            char proc_path[32];
            sprintf(proc_path, "/proc/.8s/stat", proc_entry->d_name);
            FILE* proc_stat = fopen(proc_path, "r");
            Define_proc* proc = new Define_proc();
            fscanf(proc_stat, "%d %s %*c %d", &proc->pid, &proc->name, &proc->fpid);
            fclose(proc_stat);
            insert(proc);
            if (proc->fpid != 0)
            {
                Define_proc* parent = get_proc(proc->fpid);
                parent->cpid[parent->cpid_num++] = get_idx(proc->pid);
            }

        }
    }
    closedir(proc_dir);
}

void print_tree(Define_proc* proc, int depth)
{
    if(show_pid == 0)
    {
        printf("%*s--%s\n", depth * 2, " ", proc->name);
    } 
    else 
    {
    printf("%*s--%s< %d >\n", depth * 2, " ", proc->name, proc->pid);
    }

    if (proc->cpid_num == 0)
    {
        free(proc);
        std::cout << std::endl;
        return;
    }

    for (int i = 0; i < proc->cpid_num; i ++)
    {
        struct Define_proc* child = hash_table[proc->cpid[i]];
        print_tree(child, depth + 1);
    }
}

void execute()
{
    if(version) 
    {
        fprintf(stderr,"23.4\nCopyright (C) 1993-2020 Werner Almesberger and Craig Small");
        return;
    }
    
    get_all_procs();
    print_tree(get_proc(1), 0);
}

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i ++)
    {
        if(!strcmp(argv[i],"-V") || !strcmp(argv[i], "--version"))
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
        else
        {
            fprintf(stderr,"Usage: pstree [Options]...\n\t-V, --version\t\t"
                            "Display version information and exit.\n\t-p, "
                            "--show-pids\t\tShow PIDs.\n\t-n, --numeric-sort\t"
                            "Sort output by PID.\n");
        }
    }

    execute();
    
    return 0;
}

