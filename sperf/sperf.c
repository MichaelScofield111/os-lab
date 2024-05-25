#include <stdio.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>


// store the syscall informaton in the list
typedef struct list_node{
  char*  sys_name;
  double time;
  struct  list_node* next;
}list_node;

// add vitural head
list_node head_node = {
  .sys_name="start",
  .time=99999,
  .next=NULL
};

list_node* head = &head_node; 

list_node* insert_node(list_node* head, list_node* node)
{
    if(node == NULL) return head;
    assert(node);

    if(head->next == NULL){
        node->next = head->next;
        head->next = node;
    }
    else
    {   list_node* pre = NULL;
        list_node* cur = NULL;
        for (cur = head; cur != NULL; cur = cur->next){
            if(node->time > cur->time){
                // insert
                pre->next = node;
                node->next = cur;
            }
            pre = cur;
        }

        // special check
        if(cur == NULL){
            pre->next = node;
            node ->next = cur;
        }
    }
    return head;
}

void free_list(struct list_node *head) {
  list_node *curr = head;
  while (curr != NULL) {
    struct list_node *next = curr->next;
    if (curr->sys_name != NULL) {
      free(curr->sys_name);
    }
    free(curr);
    curr = next;
  }
  return;
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }

   assert(!argv[argc]);
    // create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(-1);
    }

    int pid = fork();
    if(pid == -1){
      perror("fork");
      exit(-1);
    }else if (pid == 0){
      // Child
      // close output stream
      // 并将管道写入端重定向到标准输出
      close(pipefd[0]);
      // copy stderror to pipefd[1]
      // dup2会尝试将fd[1]复制到STDERR_FILENO，
      // 这样所有写入到标准错误（stderr）的数据都会被发送到
      // pipefd[1]所指向的文件或管道中。
      if (dup2(pipefd[1], STDERR_FILENO) == -1) {
        perror("relocate fail");
        close(pipefd[1]);
        exit(-1);
      }

        // execve starce -T command
       char *exec_argv[argc + 2];
       exec_argv[0] = "strace";
       exec_argv[1] = "-T";
       for (int i = 1; i < argc; i ++){
            exec_argv[i + 1] = argv[i];
       }

       // execve the command
       exec_argv[argc + 1] = NULL;
       char *exec_envp[] = { "PATH=/bin", NULL, };
       execve("strace",          exec_argv, exec_envp);
       execve("/bin/strace",     exec_argv, exec_envp);
       execve("/usr/bin/strace", exec_argv, exec_envp);
       close(pipefd[1]);
       perror(argv[0]);
       exit(-1);
    }
    else
    { 
      // init data
      double total_time = 0;
      int curr_time = 0;

      // child finsh the command,  Parent to analyse the  stream
      // 定义正则表达式
      regex_t re_1;
      regex_t re_2;
      const char* pattern_sys_name = "[^\\(\n\r\b\t]*\\(";;
      const char* pattern_sys_time = "<[0-9\\.]*>\n";
      if(regcomp(&re_1, pattern_sys_name, REG_EXTENDED) != 0 || regcomp(&re_2, pattern_sys_time, REG_EXTENDED) != 0){
          perror("falut regcmp create");
          exit(-1);
      }

      // read stace form pipe
      char buffer[512];
      close(pipefd[1]);
      FILE* fp = fdopen(pipefd[0], "r");
      while(fgets(buffer,sizeof(buffer), fp) != NULL)
      {
        if (strstr(buffer, "+++ exited with 0 +++") != NULL || strstr(buffer, "+++ exited with 1 +++") != NULL) {
            break;
        }

        // check sys_name
        regmatch_t sys_name;
        if(regexec(&re_1, buffer, (size_t) 1, &sys_name, 0) != 0) {
          continue;
        }
        char temp_name[512];
        size_t sys_name_len = sys_name.rm_eo - sys_name.rm_so;
        strncpy(temp_name, buffer + sys_name.rm_so, sys_name_len);
        char *name = (char *)malloc(sizeof(char) * sys_name_len);
        int j = 0;
        for (int i = 0; i < sys_name_len; i ++){
            if(temp_name[i] == '(') continue;            
            name[j] = temp_name[i];
            j ++;
        }
        name[j] = '\0';

        //check sys spend time
        regmatch_t sys_time;
        if(regexec(&re_2, buffer, (size_t) 1, &sys_time, 0) != 0) {
            continue;
        }
        char time[512];
        size_t sys_time_len = sys_time.rm_eo - sys_time.rm_so;
        strncpy(time, buffer + sys_time.rm_so + 1, sys_time_len);
        char *num_time = (char *)malloc(sizeof(char) * sys_name_len);
        j = 0;
        for (int i = 0; time[i];  i ++){
            if((time[i] >= '0' && time[i] <= '9') || time[i] == '.'){
                num_time[j] = time[i];
                j ++;
            }
        }
        num_time[j] = '\0';
        

        double spent_time = atof(num_time);
        free(num_time);
        // all time update
        total_time += spent_time;

        // store the data in the list
        // check update
        int update_flag = 0;
        list_node* node = NULL;
        list_node* pre = NULL;
        for (list_node* curr = head; curr != NULL; curr = curr->next)
        {
          if(strcmp(curr->sys_name, name) == 0){
            update_flag = 1;
            pre->next = curr->next;
            node = curr;
            free(name);
            break;
          }
          pre = curr;
        }

        if (update_flag) {
          // 更新信息
          node->time += spent_time;
        } else {
          // 新建节点，初始化信息
          node = (list_node *)malloc(sizeof(list_node));
          node->sys_name = name;
          node->time = spent_time;
        }

        // 插入链表
        head = insert_node(head, node);
 
    }

        //format print 5th spend time
        printf("Time: %f\n", total_time);
        int k = 0;
        list_node* st = head->next;
        for (list_node* start = st; start != NULL; start = start->next)
        {  
          if((k ++) > 6) break;
          printf("%10s:%10lfs(%.2lf%%)\n",start->sys_name,start->time, start->time / total_time * 100);
        }

        printf("===================================\n");

        fclose(fp);
        close(pipefd[0]);
        regfree(&re_1);
        regfree(&re_2);
        //free_list(head);
    }
    return 0;
}





