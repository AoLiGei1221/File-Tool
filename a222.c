#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include "a2.h"

void print_target(int pid) {
    if(pid > 0) {
        printf(">>> Target PID: %d\n", pid);
        printf("\n");
        printf("\n");
    }
}

void skip_line() {
    printf("\n");
    printf("\n");
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
    if(*head == NULL) {
        *head = new_node;
    } else {
        process *curr = *head;
        while(curr -> next != NULL) {
            curr = curr -> next;
        }
        curr -> next = new_node;
    }
}

int get_current_pid() {
    struct passwd *pw = getpwuid(getuid());
    int pid = pw -> pw_uid;
    return pid;
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
            if(!(strcmp(pw -> pw_name, temp_real_user))) {
                //printf("%s\n", pw -> pw_name);
                int pid = strtol(entry -> d_name, NULL, 10);
                char fd_dir[1024];
                snprintf(fd_dir, 1024, "/proc/%d/fd", pid);
                DIR *fd = opendir(fd_dir);
                if (fd == NULL)
                    continue;
                if(errno == EACCES)
                    continue;
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
    if(t_flag) {
        print_target(pid);
        printf("         PID     FD       Filename       Inode\n");
        printf("         ========================================\n");
    } else {
        printf("         PID     FD       Filename       Inode\n");
        printf("         ========================================\n");
    }
}

void save_txt(process *head, int target) {
    int count = 0;
    FILE *f = fopen("compositeTable.txt", "w+");
    if(f == NULL) {
        fprintf(stderr, "Error on opening file");
        exit(1);
    } else {
        for(process *curr = head; curr != NULL; curr = curr -> next) {
            if(target)
                fprintf(f, "         %-8d%-8d%s       %lu\n", curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
            else
                fprintf(f, " %-8d%-8d%-8d%s       %lu\n", count++, curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
        }
    }
    fclose(f);
}

void save_bin(process *head, int target) {
    int count = 0;
    FILE *f = fopen("compositeTable.bin", "wb");
    if(f == NULL) {
        fprintf(stderr, "Error on opening file");
        exit(1);
    } else {
        for(process *curr = head; curr != NULL; curr = curr -> next) {
            if(target) {
                char s[2048];
                snprintf(s, 2048,"         %-8d%-8d%s       %lu\n", curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
                fwrite(&s, sizeof(s), 1, f);
            } else {
                char s[2048];
                snprintf(s, 2048," %-8d%-8d%-8d%s       %lu\n", count++, curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
                fwrite(&s, sizeof(s), 1, f);
            }
        }
    }
    fclose(f);
}

void save(process *head, int bin, int txt, int t) {
    if(txt)
        save_txt(head, t);
    else if(bin)
        save_bin(head, t);
}

void print_per_process(int target_flag, process *curr, int i, int pid) {
    if(!target_flag) {
        skip_line();
        printf("           PID     FD\n");
        printf("           ========================================\n");
        while(curr != NULL) {
            printf(" %-3d        %-8d%-3d\n", i++, curr -> pid, curr -> FD);
            curr = curr -> next;
        }
        printf("             ========================================\n");
    } else {
        print_target(pid);
        printf("        PID     FD\n");
        printf("        ========================================\n");
        while(curr != NULL) {
            if(curr -> pid == pid) {
                printf("        %-8d%-3d\n", curr -> pid, curr -> FD);
            }
            curr = curr -> next;
        }
        printf("        ========================================\n");
    }
}

void print_system_wide(int target_flag, process *curr, int i, int pid) {
    if(!target_flag) {
        skip_line();
        printf("            PID     FD      Filename\n");
        printf("            ========================================\n");
        while(curr != NULL) {          
            printf(" %-3d       %-8d%-8d%s\n", i++, curr -> pid, curr -> FD, curr -> file_name);
            curr = curr -> next;
        }
        printf("            ========================================\n");
    } else {
        print_target(pid);
        printf("            PID     FD      Filename\n");
        printf("            ========================================\n");
        while(curr != NULL) {  
            if(curr -> pid == pid) {
                printf("         %-8d%-8d%s\n", curr -> pid, curr -> FD, curr -> file_name);
            }        
            curr = curr -> next;
        }
        printf("            ========================================\n");
        skip_line();
    }
}

void print_Vnodes(int target_flag, process *curr, int i, int pid) {
    if(!target_flag) {
        skip_line();
        printf("           Inode\n");
        printf("           ========================================\n");
        while(curr != NULL) {          
            printf(" %-3d       %lu\n", i++, curr -> inode);
            curr = curr -> next;
        }
        printf("            ========================================\n");
    } else {
        print_target(pid);
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
                    printf("         %-8d%-8d%s       %lu\n", curr -> pid, curr -> FD, curr -> file_name, curr -> inode);
                }
                curr = curr -> next;
            }
        }
        printf("         ========================================\n");
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

int main(int argc, char **argv) {
    struct passwd *pw = getpwuid(getuid());
    char *user = pw -> pw_name;
    process *head = NULL;
    head = create_save_info(&head, user);
    int show_per_process = 0, show_system_wide = 0, show_Vnodes = 0, show_composite = 0;
    int threshold = -1, pid = -1, target_flag = 0;
    int txt = 0, bin = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--per-process") == 0)
            show_per_process = 1;
        else if (strcmp(argv[i], "--systemWide") == 0)
            show_system_wide = 1;
        else if (strcmp(argv[i], "--Vnodes") == 0)
            show_Vnodes = 1;
        else if (strcmp(argv[i], "--composite") == 0)
            show_composite = 1;
        else if (strstr(argv[i], "--threshold=") != NULL)
            threshold = strtol(argv[i] + 12, NULL, 10);
        else if (strcmp(argv[i], "--output_TXT") == 0)
            txt = 1;
        else if (strcmp(argv[i], "--output_binary") == 0)
            bin = 1;
        else {
            pid = strtol(argv[i], NULL, 10);
            target_flag = 1;
        }
    }

    save(head, bin, txt, target_flag);
    process_arg(show_per_process, show_system_wide, show_Vnodes, show_composite, target_flag, pid, head);
    return 0;
}