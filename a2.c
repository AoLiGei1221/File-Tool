#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <grp.h>
#include "a2.h"


void print_target(int pid) {
    if(pid > 0) printf(">>> Target PID: %d\n", pid);
}

void skip_line() {
    printf("\n\n");
}

long int get_inode(char *f) {
    struct stat buf;
    stat(f, &buf);
    return buf.st_ino;
}

process *create_new_process() {
    process *new_process = NULL;
    new_process = (process *) calloc(1, sizeof(process));
    if(new_process == NULL)
        return NULL;
    new_process -> pid = -1;
    new_process -> FD = -1;
    strcpy(new_process -> file_name, "");
    new_process -> inode = -1;
    new_process -> next = NULL;
    return new_process;
}

void update(process *node, int new_pid, int new_FD, int inode, char filename[1024]) {
    node -> pid = new_pid;
    node -> FD = new_FD;
    node -> inode = inode;
    strcpy(node -> file_name, filename);
    node -> next = NULL;
}

void insertTail(process **head, process *new_node) {
    if(*head == NULL) *head = new_node;
    else {
        process *curr = *head;
        for(; curr -> next != NULL; curr = curr -> next);
        curr -> next = new_node;
    }
}

process* create_save_info(process **head, char *user) {
    char temp_real_user[1024];
    strcpy(temp_real_user, user);
    struct stat statbuf;
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        int f = entry -> d_type == DT_DIR && (isdigit(entry -> d_name[0]));
        if (f != 0) {
            char file_path[1024];
            file_path[0] = '\0';
            strcat(file_path, "/proc/");
            strcat(file_path, entry -> d_name);
            if(stat(file_path, &statbuf) == -1) {exit(1);}
            struct passwd *pw = getpwuid(statbuf.st_uid);
            if(strcmp(pw -> pw_name, temp_real_user) == 0) {
                int pid = strtol(entry -> d_name, NULL, 10);
                char fd_dir[1024];
                snprintf(fd_dir, 1024, "/proc/%d/fd", pid);
                DIR *fd = opendir(fd_dir);
                if (fd == NULL) continue;
                if(errno == EACCES) continue;
                struct dirent *fd_entry;
                while ((fd_entry = readdir(fd)) != NULL) {
                    if (fd_entry -> d_type == DT_LNK) {
                        char fd_path[1024];
                        snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, fd_entry -> d_name);
                        char filename[1024];
                        filename[0] = '\0';
                        ssize_t len = readlink(fd_path, filename, 1024);
                        if (len > 0) {
                            filename[len] = '\0';
                            long int inode = get_inode(fd_path);
                            process *new_node = create_new_process();
                            update(new_node, atoi(entry -> d_name), atoi(fd_entry -> d_name), inode, filename);
                            insertTail(head, new_node);
                        }
                    }
                }
                closedir(fd);
            }
        }
    }
    closedir(dir);
    return *head;
}

void print_for_composite(int t_flag, int pid) {
    /*in order to fully formatted for printing, I need to split into
      two cases.
    */
    if(t_flag) {
        print_target(pid);
        skip_line();
        printf("           PID     FD       Filename       Inode\n");
        printf("           ========================================\n");
    } else {
        printf("         PID     FD       Filename       Inode\n");
        printf("         ========================================\n");
    }
}

void save_txt(process *head, int target) {
    FILE *f = fopen("compositeTable.txt", "w+");
    if(f == NULL) {perror("Error on opening file"); exit(1);}
    int count = 0;
    char *header = "         PID     FD       Filename       Inode\n";
    char *temp = "         ========================================\n";
    if(target != -1) fprintf(f, ">>> Target PID: %d\n\n\n", target); 
    fputs(header, f);
    fputs(temp, f);
    for(; head != NULL; head = head -> next) {
        if(target == -1) {
            fprintf(f, " %-8d%-8d%-8d%s       %lu\n", count++,
                head -> pid, head -> FD, head -> file_name, head -> inode);
        }
        else if(target != -1 && head -> pid == target) {
            fprintf(f, "         %-8d%-8d%s       %lu\n", head -> pid,
                head -> FD, head -> file_name, head -> inode);
        }
    }
    fputs(temp, f);
    fclose(f);
}

void save_bin(process *head, int target) {
    FILE *f = fopen("compositeTable.bin", "wb");
    if(f == NULL) { perror("Error opening file"); exit(42); }
    int count = 0;
    char header[1024] = "         PID     FD       Filename       Inode\n";
    char temp[1024] = "         ========================================\n";
    if(target != -1) {
        char t[1024];
        if (sprintf(t, ">>> Target PID: %d\n", target) < 0) {
            perror("Error creating string");
            exit(42);
        }
        fwrite(t, sizeof(char), strlen(t), f);
    }
    fwrite(header, sizeof(char), strlen(header), f);
    fwrite(temp, sizeof(char), strlen(temp), f);
    while(head != NULL) {
        if(target > -1 && head -> pid == target) {
            fwrite(&count, sizeof(int), 1, f);
            count++;
            fwrite(&head->pid, sizeof(int), 1, f);
            fwrite(&head->FD, sizeof(int), 1, f);
            fwrite(head->file_name, sizeof(char), strlen(head->file_name), f);
            fwrite(&head->inode, sizeof(long int), 1, f);
        }
        if(target == -1) {
            fwrite(&head->pid, sizeof(int), 1, f);
            fwrite(&head->FD, sizeof(int), 1, f);
            fwrite(head->file_name, sizeof(char), strlen(head->file_name), f);
            fwrite(&head->inode, sizeof(long int), 1, f);
        }
        count++;
        head = head -> next;
    }
    fwrite(temp, sizeof(char), strlen(temp), f);
    fclose(f);
}

void save(process *head, int bin, int txt, int t) {
    if(txt) save_txt(head, t);
    else if(bin) save_bin(head, t);
}

void print_per_process(int target_flag, process *curr, int i, int pid) {
    if(target_flag) print_target(pid);
    skip_line();
    printf("           PID     FD\n");
    printf("           ========================================\n");
    if(!target_flag) {
        for(; curr != NULL; curr = curr -> next)
            printf(" %-3d       %-8d%-3d\n", i++, curr -> pid, curr -> FD);
    } else {
        for(;curr != NULL; curr = curr -> next)
            if(curr -> pid == pid)
                printf("           %-8d%-3d\n", curr -> pid, curr -> FD);
    }
    if(!target_flag)
        printf("            ========================================\n");
    else{
        printf("           ========================================\n");
        skip_line();
    }
}   

void print_system_wide(int target_flag, process *curr, int i, int pid) {
    if(target_flag == 1) print_target(pid);
    skip_line();
    printf("           PID     FD      Filename\n");
    printf("           ========================================\n");
    if(!target_flag) {
        for(; curr != NULL; curr = curr -> next)       
            printf(" %-3d       %-8d%-8d%s\n", i++, curr -> pid, curr -> FD, curr -> file_name);
    } else {
        for(; curr != NULL; curr = curr -> next) 
            if(curr -> pid == pid)
                printf("           %-8d%-8d%s\n", curr -> pid, curr -> FD, curr -> file_name);
    }
    if(!target_flag)
        printf("            ========================================\n");
    else {
        printf("           ========================================\n");
        skip_line();
    }
}

void print_Vnodes(int target_flag, process *curr, int i, int pid) {
    if(target_flag == 1) print_target(pid);
    skip_line();
    if(!target_flag) {
        printf("           Inode\n");
        printf("           ========================================\n");
        while(curr != NULL) {          
            printf(" %-3d       %lu\n", i++, curr -> inode);
            curr = curr -> next;
        }
        printf("            ========================================\n");
    } else {
        printf("           FD             Inode\n");
        printf("           ========================================\n");
        while(curr != NULL) {  
            if(curr -> pid == pid) {
                printf("           %-15d%lu\n", curr -> FD, curr -> inode);
            }        
            curr = curr -> next;
        }
        printf("           ========================================\n");
        skip_line();
    }
}

void print_composite(int target_flag, process *curr, int i, int pid) {
    if(!target_flag) {
        skip_line();
        print_for_composite(target_flag, curr -> pid);
        while(curr != NULL) {
            printf(" %-8d%-8d%-8d%s       %lu\n", i++, curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
            curr = curr -> next;
        }
    } else {
        print_for_composite(target_flag, pid);
        while(curr != NULL) {
            if(curr -> pid == pid) {
                printf("           %-8d%-8d%s       %lu\n", curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
            }
            curr = curr -> next;
        }
    }
    if(!target_flag)
        printf("         ========================================\n");
    else
        printf("           ========================================\n");
    skip_line();
}

void process_arg(int show_per_process, int show_system_wide, int show_Vnodes, int show_composite, int target_flag, int pid, process *head) {
    int default_flag = !show_per_process && !show_system_wide && !show_Vnodes && !show_composite, i;
    if(default_flag || show_composite) {
        process *curr = head;
        i = 0;
        print_composite(target_flag, curr, i, pid);
    }
    if(show_per_process) {
        process *curr = head;
        i = 0;
        print_per_process(target_flag, curr, i, pid);
    }
    if(show_system_wide) {
        process *curr = head;
        i = 0;
        print_system_wide(target_flag, curr, i, pid);
    }
    if(show_Vnodes) {
        process *curr = head;
        i = 0;
        print_Vnodes(target_flag, curr, i, pid);
    }
}

void print_threshold(process *head, int threshold) {
    int counter = 0, pid;
    printf("## Offending process -- #FD threshold=%d\n", threshold);
    if(head != NULL) { pid = head -> pid; }
    for(process *curr = head; curr != NULL; curr = curr -> next) {
        if(curr -> pid == pid) {
            counter += 1;
        } else {
            if(counter > threshold) { printf("%d (%d), ", pid, counter);}
            pid = curr -> pid;
            counter = 1;
        }
    }
    if(counter > threshold)
        printf("%d (%d), ", pid, counter);
    skip_line();
}

void delete_linked_list(process *head) {
    process* current = head;
    process* temp;
    while (current != NULL) {
        temp = current->next;
        free(current);
        current = temp;
    }
    head = NULL;
}

int main(int argc, char **argv) {
    struct passwd *pw = getpwuid(getuid());
    char *user = pw -> pw_name;
    process *head = NULL;
    head = create_save_info(&head, user);
    int show_per_process = 0, show_system_wide = 0, show_Vnodes = 0, show_composite = 0;
    int threshold = -1, pid = -1, target_flag = 0;
    int txt = 0, bin = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--per-process") == 0) show_per_process = 1;
        else if (strcmp(argv[i], "--systemWide") == 0) show_system_wide = 1;
        else if (strcmp(argv[i], "--Vnodes") == 0) show_Vnodes = 1;
        else if (strcmp(argv[i], "--composite") == 0) show_composite = 1;
        else if (strstr(argv[i], "--threshold=") != NULL)
            threshold = strtol(argv[i] + 12, NULL, 10);
        else if (strcmp(argv[i], "--output_TXT") == 0) txt = 1;
        else if (strcmp(argv[i], "--output_binary") == 0) bin = 1;
        else { pid = strtol(argv[i], NULL, 10); target_flag = 1; }
    }
    save(head, bin, txt, pid);
    process_arg(show_per_process, show_system_wide, show_Vnodes, show_composite, target_flag, pid, head);
    if(threshold != -1) print_threshold(head, threshold);
    delete_linked_list(head);
    return 0;
}
