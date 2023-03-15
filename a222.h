#ifndef __ABC_H__
#define __ABC_H__

typedef struct process{
    int pid;
    int FD;
    char file_name[1024];
    long int inode;
    struct process *next;
} process;

void print_target(int pid);
long int get_inode(char *f);
process *create_new_process();
void update(process *node, int new_pid, int new_FD, int inode, char filename[1024]);
void insertTail(process **head, process *new_node);
int get_current_pid();
process *create_save_info(process **head, char *user);
void print_for_composite(int t_flag, int pid);
void save_txt(process *head, int target);
void save_bin(process *head, int target);
void save(process *head, int bin, int txt, int t);
void skip_line();
void print_per_process(int target_flag, process *curr, int i, int pid);
void print_system_wide(int target_flag, process *curr, int i, int pid);
void print_Vnodes(int target_flag, process *curr, int i, int pid);
void process_arg(int show_per_process, int show_system_wide, int show_Vnodes, int show_composite, int target_flag, int pid, process *head);
void print_composite(int target_flag, process *curr, int i, int pid);
#endif