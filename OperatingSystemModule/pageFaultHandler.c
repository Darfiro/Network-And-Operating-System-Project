#include<linux/module.h> 
#include<linux/version.h> 
#include<linux/kernel.h> 
#include<linux/init.h> 
#include<linux/kprobes.h> 
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
 
#define BYTES 1024
MODULE_LICENSE("GPL");

struct Process
{
    long pid;
    char* comm;
    long vma;
    long rss;
    long page_fault_counter;
    char* first_noticed;
    char* last_updated;
    struct Process *next;
};
char *timeBuf;
struct Process *head;
struct file_operations fops;
struct proc_dir_entry *proc_entry;
int read_index = 0;
int write_index = 0;
char *output_buffer;

char * convertToString(long number)
{
    char *buf = vmalloc(BYTES);
    memset(buf, BYTES, 0); 
    sprintf(buf, "%ld", number);
    return buf;
}

int len_str(char *comm)
{
    char *tmp = comm;
    int size = 0;
    while(*comm != '\0')
    {
        size++;
        comm++;
    }
    comm = tmp;
    return size;
}

char* create_separator(char* comm)
{
    int size = 24;
    int tmp_size = 24;
    char *separator = vmalloc(size);
    char tmp[size];
    int i = 0;
    int size_comm = len_str(comm);
    memset(separator, size, 0);
    tmp_size -= size_comm;
    if (tmp_size < 0)
    {
        sprintf(separator, " ");
    }
    else
    {
        while(i < tmp_size)
        {       
            tmp[i] = '-';
            i++;
        }
        while(i < size)
        {
            tmp[i] = '\0';
            i++;
        }
        sprintf(separator, "%s", tmp);        
    }
    return separator;
}

char* create_buffer(char * output_buffer, struct Process *head)
{
    char *tmp = vmalloc(BYTES);
    char *tabs;
    memset(tmp, BYTES, 0);
    memset(output_buffer, 4*PAGE_SIZE, 0);
    sprintf(output_buffer, "PID\tCOMM\t\t\tVMA/KB\tRSS/KB\tCOUNT\tFN\t\t\tLU\n");
    while(head)
    {
        tabs = create_separator(head->comm);
        sprintf(tmp, "%ld\t%s%s%ld\t%ld\t%ld\t%s\t%s\n", head->pid,
                                                           head->comm,
                                                           tabs,
                                                           head->vma, 
                                                           head->rss, 
                                                           head->page_fault_counter, 
                                                           head->first_noticed,
                                                           head->last_updated);
        strcat(output_buffer, tmp);
        vfree(tabs);
        head = head->next;
    }
    write_index = strlen(output_buffer);
    return output_buffer;
}

ssize_t read_list(struct file *file, char *buf, size_t count, loff_t *f_pos)
{
    int len;
    output_buffer = create_buffer(output_buffer, head);

    if (write_index == 0 || *f_pos > 0)
    {
        return 0;
    }
    read_index = 0;

    while(read_index < write_index)
    {        
        copy_to_user(buf, &output_buffer[read_index], count);
        len = strlen(&output_buffer[read_index]) + 1;
        read_index += len;
        *f_pos += len;
    }
    return len;
}

void push_process(struct Process **head, struct Process *new_proc)  
{
    if (*head == NULL)
    {
        *head = new_proc;
        (*head)->next = NULL;
    }
    else
    {        
        new_proc->next = *head;
        *head = new_proc;
    }
}

void free_list(struct Process *head)
{
    struct Process *tmp = NULL;
    while(head)
    {
        tmp = head->next;
        vfree(head->comm);
        vfree(head);
        head = tmp;
    }   
}

struct Process *find_process_by_id(struct Process *head, long pid)
{
    struct Process *tmp = NULL;
    struct Process *head_save = head;
    while (head && tmp == NULL)
    {      
        if (head->pid == pid)         
            tmp = head;
        head = head->next;
    }    
    head = head_save;
    return tmp;
}

long countVMA(struct task_struct *task)
{
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    long total = 0;
    mm = task->mm;
    for (vma = mm->mmap ; vma ; vma = vma->vm_next) 
    {
        total += vma->vm_end - vma->vm_start;
    }
    return total/BYTES;
}

long countRSS(struct task_struct *task)
{
    return get_mm_rss(task->mm)*PAGE_SIZE/BYTES; 
}

char* get_current_time(char *buf)
{    
    unsigned long get_time;
    int sec, hr, min, tmp1,tmp2, year, day, month;
    struct timeval tv;
    struct tm tv2;

    do_gettimeofday(&tv);
    get_time = tv.tv_sec;
    sec = get_time % 60;
    tmp1 = get_time / 60;
    min = tmp1 % 60;
    tmp2 = tmp1 / 60;
    hr = (tmp2 % 24) + 3;
    time_to_tm(get_time, 0, &tv2);
    year = tv2.tm_year + 1900;
    month = tv2.tm_mon + 1;
    day = tv2.tm_mday;

    sprintf(buf,"%d/%d/%d %02d:%02d:%02d", day, month, year, hr, min, sec); 
    return buf; 
}

int pre_handler(struct kprobe *p, struct pt_regs *regs)
{     
    return 0; 
} 
 
void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{ 
    struct Process *addition = find_process_by_id(head, current->pid);
    memset(timeBuf, BYTES, 0);
    timeBuf = get_current_time(timeBuf);
    if (addition == NULL)
    {
        addition = vmalloc(sizeof(struct Process));
        addition->comm = vmalloc(BYTES);
        memset(addition->comm, BYTES, 0);
        addition->last_updated = vmalloc(BYTES);
        memset(addition->last_updated, BYTES, 0);
        addition->first_noticed = vmalloc(BYTES);
        memset(addition->first_noticed, BYTES, 0);
        sprintf(addition->first_noticed, "%s", timeBuf);
        addition->pid = current->pid;
        sprintf(addition->comm, "%s", current->comm);
        addition->page_fault_counter = 0;
        addition->vma = 0;
        addition->rss = 0;
        if (current->mm != NULL)
            addition->vma = countVMA(current);
        push_process(&head, addition);
    }
    if (current->mm != NULL)
    {
        addition->rss = countRSS(current);
    }
    addition->page_fault_counter++;    
    sprintf(addition->last_updated, "%s", timeBuf);
} 

static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    printk(KERN_ERROR "fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
    return 0;
}
 
static struct kprobe kp; 
 
int myinit(void) 
{   
    timeBuf = vmalloc(BYTES);
    head = NULL;

    output_buffer = vmalloc(4*PAGE_SIZE);    

    fops.owner = THIS_MODULE;
    fops.read = read_list;

    proc_entry = proc_create_data("MemoryMonitor", 0666, NULL, &fops, NULL);
    if (!proc_entry)
    {
        vfree(timeBuf);
        printk(KERN_INFO "MemoryMonitor: Cannot create fortune file.\n");
        return -ENOMEM;
    }
 
    kp.pre_handler = pre_handler;
    kp.post_handler = post_handler; 
    kp.fault_handler = handler_fault;
    kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name("__handle_mm_fault");
    register_kprobe(&kp); 

    printk(KERN_INFO "MemoryMonitor: kprobe registered.\n"); 
    return 0; 
} 
 
void myexit(void) 
{ 
    unregister_kprobe(&kp); 
    free_list(head);
    if (timeBuf)
        vfree(timeBuf);
    remove_proc_entry("MemoryMonitor", NULL);
} 
 
module_init(myinit); 
module_exit(myexit); 