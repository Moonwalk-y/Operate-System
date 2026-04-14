#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>

#define MAX_PID 65536

typedef struct ProcessNode{
    pid_t pid;
    pid_t ppid;
    char comm[256];

    struct ProcessNode *parent;
    struct ProcessNode *first_child;
    struct ProcessNode *next_sibling;
}ProcessNode;

static int read_comm(pid_t pid, char *buf, size_t n) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, (int)n, f)) { fclose(f); return -1; }
    buf[strcspn(buf, "\n")] = 0;
    fclose(f);
    return 0;
}

static int get_ppid_from_stat(pid_t pid, pid_t *ppid_out) {
    char path[64], line[4096];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);

    int id, ppid;
    char comm[256], state;
    if (sscanf(line, "%d (%255[^)]) %c %d", &id, comm, &state, &ppid) != 4) return -1;
    *ppid_out = (pid_t)ppid;
    return 0;
}

static int collect_all_processes(ProcessNode* proc_table[]) {
    DIR *d = opendir("/proc");
    if (!d) { perror("opendir /proc"); return -1; }

    struct dirent *de;
    int count = 0;

    while ((de = readdir(d)) != NULL) {
        if (!isdigit((unsigned char)de->d_name[0])) continue;
        pid_t pid = (pid_t)atoi(de->d_name);

        pid_t ppid;
        if (get_ppid_from_stat(pid, &ppid) != 0) continue;

        ProcessNode *node = (ProcessNode*)malloc(sizeof(ProcessNode));
        if (!node) { closedir(d); return -1; }
        node->pid = pid;
        node->ppid = ppid;
        node->parent = NULL;
        node->first_child = NULL;
        node->next_sibling = NULL;
        if (read_comm(pid, node->comm, sizeof(node->comm)) != 0) {
            free(node);
            continue;
        }

        proc_table[pid] = node;
        count++;
    }

    closedir(d);
    return count;
}

static void link_parent_child(ProcessNode* proc_table[]) {
    for (int pid = 0; pid <= MAX_PID; pid++) {
        ProcessNode *node = proc_table[pid];
        if (!node) continue;

        pid_t ppid = node->ppid;
        if (ppid > 0 && ppid <= MAX_PID && proc_table[ppid]) {
            ProcessNode *parent = proc_table[ppid];
            node->parent = parent;
            if (parent->first_child == NULL) {
                parent->first_child = node;
            } else {
                ProcessNode *cur = parent->first_child;
                while (cur->next_sibling != NULL) {
                    cur = cur->next_sibling;
                }
                cur->next_sibling = node;
            }
        }
    }
}


void print_tree(ProcessNode *root, int level, int has_pid) {
    if (!root) return;

    if (level > 0) {
    for (int i = 0; i < level - 1; i++) printf("    ");
    printf("  |-");
    }
    if(has_pid) printf("%s(%d)\n", root->comm, root->pid);
    else printf("%s\n", root->comm);

    ProcessNode *cur = root->first_child;
    while (cur) {
        print_tree(cur, level + 1, has_pid);
        cur = cur->next_sibling;
    }
}

static int build_tree(ProcessNode* root){
    if (!root) return 0;

    ProcessNode **proc_table = (ProcessNode**)calloc(MAX_PID + 1, sizeof(ProcessNode*));
    if (!proc_table) return 1;

    int count = collect_all_processes(proc_table);
    if (count < 0) {
        free(proc_table);
        return 1;
    }

    link_parent_child(proc_table);

    // Ensure root node exists in proc_table (use the passed root node)
    pid_t root_pid = root->pid;
    if (root_pid <= MAX_PID && proc_table[root_pid]) {
        // There is already a node for root_pid created by collect_all_processes
        ProcessNode *existing = proc_table[root_pid];
        // Copy child list from existing to root
        root->first_child = existing->first_child;
        // Update parent pointers of children to point to root
        ProcessNode *child = root->first_child;
        while (child) {
            child->parent = root;
            child = child->next_sibling;
        }
        // Free the duplicate node (but keep its children linked to root)
        existing->first_child = NULL; // prevent double free of children
        free(existing);
        proc_table[root_pid] = root;
    }
    // If root_pid not found in proc_table (should not happen for pid 1), keep root as is

    // Free process table array (but not the nodes - they are now part of the tree)
    free(proc_table);

    return 0;
}

int main(int argc, char *argv[]) {
    int opt;
    int version_flag = 0;
    int pid_flag = 0;
    int numeric_flag = 0;

    struct option long_options[] = {
        {"show-pids", no_argument, 0, 'p'},
        {"numeric-sort", no_argument, 0, 'n'},
        {"version", no_argument, 0, 'V'},
        {0,0,0,0}
    };

    while((opt = getopt_long(argc, argv, "pnV", long_options, NULL)) != -1){
        switch(opt){
            case 'p':
                pid_flag = 1;
                break;
            case 'n':
                numeric_flag = 1;
                break;
            case 'V':
                version_flag = 1;
                break;
            case '?':
                return 1;
        }
    }

    if(version_flag == 1){
        if(argc == 2){
            printf("pstree 1.0\n");
            return 0;
        }else return 1;
    }

    if(pid_flag == 1){
        if(1){
            ProcessNode* root = (ProcessNode*)malloc(sizeof(ProcessNode));
            root->pid = 1;
            root->ppid = 0;
            root->parent = NULL;
            read_comm(root->pid,root->comm,sizeof root->comm);
            root->next_sibling = NULL;
            if(build_tree(root)){
                printf("build tree error\n");
                return 1;
            }
            print_tree(root,0,1);
        }else return 1;
    }else{
        ProcessNode* root = (ProcessNode*)malloc(sizeof(ProcessNode));
            root->pid = 1;
            root->ppid = 0;
            root->parent = NULL;
            read_comm(root->pid,root->comm,sizeof root->comm);
            root->next_sibling = NULL;
            if(build_tree(root)){
                printf("build tree error\n");
                return 1;
            }
            print_tree(root,0,0);
    }

    if(numeric_flag == 1){
        if(1){
            
        }else return 1;
    }

    return 0;
}
