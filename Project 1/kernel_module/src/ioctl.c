//Project 1: Pranav Gaikwad, pmgaikwa; Shashank Shekhar, sshekha4;

//////////////////////////////////////////////////////////////////////
//                      North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng, Yu-Chia Liu
//
//   Description:
//     Core of Kernel Module for Processor Container
//
////////////////////////////////////////////////////////////////////////

#include "processor_container.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kthread.h>

/*Structure for threads starts*/
typedef struct tnode{
    int tid;
    struct task_struct *task_details;
    struct tnode *t_ptr;
}task;
/*Structure for threads ends*/
/*Structure for container starts*/
typedef struct node{
    long long unsigned int cid;
    struct node *c_ptr;
    task *link;
}container;
/*Structure for container ends*/

typedef struct tcnode{
    int t_id;
    long long unsigned int c_id;
    struct tcnode *tc_link;
}task_container;

//Initialize a head pointer to the container linked list
container *head = NULL;

//Initialize a head pointer to the task_container linked list
task_container *tc_head = NULL;

//Define and initialize mutex
static DEFINE_MUTEX(my_mutex);

//Print the entire list (containers and tasks)
void printlist(void){
    //printk("Entering the list print function---->\n");
    container *ptr = head;
    while(ptr != NULL){
        int i = 0;
        //printk("Container Id: %llu\n", ptr->cid);
        //printk("---------------------------\n");
        task * task_ptr = ptr->link;
        while(task_ptr != NULL){
            //printk("Task %d has id: %d\n", i++, task_ptr->tid);
            task_ptr = task_ptr->t_ptr;
        }
        //printk("----------------------------------------------\n");
        ptr = ptr->c_ptr;
    }
    //printk("Exiting the list print function\n");
}

void print_container_lookup(void){
    task_container *curr = tc_head;
    //printk("Printing Container Lookup List: \n");
    while(curr->tc_link != NULL){
        //printk("%d -->", curr->t_id);
    }
    //printk("\n");
}

/*Function to lookup the container which contains the current thread starts*/
long long unsigned int container_lookup(int tid){
    //Assign initial value of 0 to cid
    long long unsigned int cid = ULLONG_MAX;
    task_container *curr = tc_head;
    if(curr == NULL){
        //Error condition
        //printk("Reached bad code. Value of cid(ULLONG_MAX): %llu\n", cid);
    }
    else{
        //curr should never be NULL. If it is NULL, then it is an error condition
        while(curr->t_id != tid){
            curr = curr->tc_link;
            if(curr == NULL){
                //Error condition, break
                break;
            }
        }
        if(curr == NULL){
            //Error condition, break
            //printk("Reached bad code. Value of cid(ULLONG_MAX): %llu\n", cid);
        }
        else if(curr->t_id == tid){
            cid = curr->c_id;
        }
    }
    return cid;
}
/*Function to lookup the container which contains the current thread ends*/

/*Function to insert the container which contains the current thread starts*/
void container_insert(int tid, long long unsigned int cid){
    //printk("Loop again - Container Insert Started!!\n");
    task_container *newnode = NULL;
    if(tc_head == NULL){
        //This means that we are creating 1st node. Create an empty node and insert the data in it.
        tc_head = (task_container *)kmalloc(sizeof(task_container), GFP_KERNEL); 
        tc_head->t_id = tid;
        tc_head->c_id = cid;
        tc_head->tc_link = NULL;
    }
    else{
        task_container *curr_ptr = tc_head;
        //Loop through till the last node and insert a node at the last
        while(curr_ptr->tc_link != NULL){
            curr_ptr = curr_ptr->tc_link;
        }
        newnode = (task_container *)kmalloc(sizeof(task_container), GFP_KERNEL); 
        newnode->t_id = tid;
        newnode->c_id = cid;
        newnode->tc_link = NULL;
        curr_ptr->tc_link = newnode;
    }
    //printk("Loop again - Container Insert Completed!!\n");
}
/*Function to insert the container which contains the current thread ends*/

/*Function to delete the container which contains the current thread starts*/
void container_delete(int tid){
    //printk("Loop again - Container Delete Started!!\n");
    //tc_head can never be NULL
    task_container *curr_ptr = tc_head;
    task_container *prev_ptr = NULL;

    if(curr_ptr == NULL){
        // Then this is an error condition
    }

    while(curr_ptr->t_id != tid){
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->tc_link;

        //If you reach the end of this list without actually finding the node, then it is error condition
        if(curr_ptr == NULL){
            //Error condition
            break;
        }

    }
    if(curr_ptr == NULL){
        //Error condition. This shouldn't happen. There needs to exist an entry in this list
    }
    //Now for real cases
    if(curr_ptr == tc_head && prev_ptr == NULL){
        //Meaning 1st node
        tc_head = curr_ptr->tc_link;
    }
    else{
        //Meaning any other node
        prev_ptr->tc_link = curr_ptr->tc_link;
    }
    //printk("Loop again - Container Delete Completed!!\n");
}
/*Function to delete the container which contains the current thread ends*/

/**
 * Delete the task in the container.
 * 
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(), 
 */
int processor_container_delete(struct processor_container_cmd __user *user_cmd)
{
    /* Lock */
    if(mutex_lock_interruptible(&my_mutex)){
        return -ERESTARTSYS;
    }
    //printk("Delete - Lock held\n");
    
    //Find the container that has this task
    container *c_pointer = head;
    container *c_prev_pointer = NULL;

    //Get the container cid from the user space to the kernel space
    long long unsigned int *ptr = kmalloc(sizeof(long long unsigned int), GFP_KERNEL);
    copy_from_user(ptr, &(user_cmd->cid), sizeof(user_cmd->cid));

    //printk("In delete - Container Id: %llu associated with thread id %d\n", *ptr, current->pid);

    if(head == NULL){
        //printk("HEAD is NULL\n");
    }
    else{
        //printk("HEAD is not NULL\n");
    }
    // //printk("C_Pointer cid: %llu \n", c_pointer->cid);
    // //printk("PTR: %llu", *ptr);
    while(c_pointer != NULL){
        //When you get the correct container which has the thread to be deleted
        if(c_pointer->cid == *ptr){
            //Find the correct thread to be deleted in this container
            task *t_pointer = c_pointer->link;
            task *t_prev_pointer = NULL;
            while(t_pointer != NULL){
                // //printk("I am here\n");
                // //printk("%d\n", (t_pointer->tid == current->pid));
                if(t_pointer->tid == current->pid){
                    //First task to be deleted
                    if(t_prev_pointer == NULL){
                        c_pointer->link = t_pointer->t_ptr;
                        //printk("Task %d deleted\n", current->pid);

                        if(c_pointer->link != NULL){                                
                            //Release lock for 1st task of the same container
                            //printk("Waking up task (1st node): %d\n", (c_pointer->link)->task_details->pid);
                            wake_up_process((c_pointer->link)->task_details);   
                        }
                    } //Any other task to be deleted
                    else{
                        t_prev_pointer->t_ptr = t_pointer->t_ptr;
                        //printk("Task %d deleted\n", current->pid);

                        //Release lock for 1st task of the same container
                        //printk("Waking up task: %d\n", (c_pointer->link)->task_details->pid);
                        wake_up_process((c_pointer->link)->task_details);
                    }
                    //Now check if the container needs to be deleted meaning if there are no threads.
                    //Check this by checking if the link of the c_pointer is having NULL
                    if(c_pointer->link == NULL){
                        //Check which container is getting deleted. First or a random container
                        //First container to be deleted
                        if(c_prev_pointer == NULL){
                            head = c_pointer->c_ptr;
                            //printk("Container %llu deleted\n", *ptr);
                        }//Any other container to be deleted
                        else{
                            c_prev_pointer->c_ptr = c_pointer->c_ptr;
                            //printk("Container %llu deleted\n", *ptr);
                        }
                    }
                    break;
                }
                else{
                    t_prev_pointer = t_pointer;
                    t_pointer = t_pointer->t_ptr;
                }
            }
            break;
        }
        else{
            c_prev_pointer = c_pointer;
            c_pointer = c_pointer->c_ptr;
        }
    }

    //Before unlocking, delete the entry of the task from the container_delete function
    container_delete(current->pid);

    //Printing the list after delete complete
    //printk("Printing the list after delete complete:\n");
    printlist();

    /* Unlock */
    mutex_unlock(&my_mutex);    
    //printk("Delete - Lock released\n");

    return 0;
}

/**
 * Create a task in the corresponding container.
 * external functions needed:
 * copy_from_user(), mutex_lock(), mutex_unlock(), set_current_state(), schedule()
 * 
 * external variables needed:
 * struct task_struct* current  
 */
int processor_container_create(struct processor_container_cmd __user *user_cmd)
{
    /* Lock */
    if(mutex_lock_interruptible(&my_mutex)){
        return -ERESTARTSYS;
    }
    //printk("Create - Lock held\n");

    //Create an empty pointer to task structure to be used later
    task *t_pointer = NULL;
    //Get the container cid from the user space to the kernel space
    long long unsigned int *ptr = kmalloc(sizeof(long long unsigned int), GFP_KERNEL);
    copy_from_user(ptr, &(user_cmd->cid), sizeof(user_cmd->cid));
    //printk("In create - Container Id: %llu associated with thread id %d\n", *ptr, current->pid);
    if(head == NULL){
        //printk("HEAD is NULL\n");
    }
    else{
        //printk("HEAD is not NULL\n");
    }
    //First check if container exists using cid
    if(head == NULL){
        //printk("I am in head and creating a new node!!\n");
        //Meaning there are no container nodes. Create a container node
        head = (container *)kmalloc(sizeof(container), GFP_KERNEL);
        head->cid = *ptr;
        head->c_ptr = NULL;
        //Now, create a task node and then attach the task to the empty container created
        head->link = (task *)kmalloc(sizeof(task), GFP_KERNEL);
        (head->link)->tid = current->pid; 
        (head->link)->task_details = current;
        //printk("Current Thread %d inserted\n", head->link->tid);
        head->link->t_ptr = NULL;
    }
    else{
        container *c_pointer = head;
        //Set a flag variable to check if the container exists
        bool flag = true;
        //printk("I am not in head and proceeding forward!!\n");
        while(c_pointer->c_ptr != NULL){
            if(c_pointer->cid == *ptr){
                //printk("Found container existing. Inserting current task %d to container\n", current->pid);
                //Now, associate the task with this container
                t_pointer = c_pointer->link;
                while(t_pointer->t_ptr != NULL){
                    t_pointer = t_pointer->t_ptr;
                }
                t_pointer->t_ptr = (task *)kmalloc(sizeof(task), GFP_KERNEL);
                //Now, associate the task to this container
                (t_pointer->t_ptr)->tid = current->pid; 
                (t_pointer->t_ptr)->task_details = current;
                //printk("Current Thread %d inserted\n", t_pointer->tid);
                t_pointer = t_pointer->t_ptr;
                t_pointer->t_ptr = NULL;
                //Now, change the value of flag to show that there is no need to create a container and then break from the loop
                flag = false;
                break;
            }
            else{
                c_pointer = c_pointer->c_ptr;
            }
        } 
        //If the flag is still true, then just check the last node. If that node also doesn't match in value, then it means 
        //that the container doesn't exist. Hence, traverse to the end of the container list and create the container
        if(flag){
            if(c_pointer->cid == *ptr){
                //printk("Found container existing. Inserting current task %d to container\n", current->pid);
                //Now, associate the task with this container
                t_pointer = c_pointer->link;
                while(t_pointer->t_ptr != NULL){
                    t_pointer = t_pointer->t_ptr;
                }
                t_pointer->t_ptr = (task *)kmalloc(sizeof(task), GFP_KERNEL);
                //Now, associate the task to this container
                (t_pointer->t_ptr)->tid = current->pid; 
                (t_pointer->t_ptr)->task_details = current;
                //printk("Current Thread %d inserted\n", t_pointer->tid);
                t_pointer = t_pointer->t_ptr;
                t_pointer->t_ptr = NULL;
                //Now, change the value of flag to show that there is no need to create a container and then break from the loop
                flag = false;
            }
            else{
                //printk("Didn't find a container. Creating new container at list end\n");
                //Create a new container node at the end of the list and then associate the current task to the container just created
                c_pointer->c_ptr = (container *)kmalloc(sizeof(container), GFP_KERNEL);
                //printk("KMALLOC worked fine for container creation.\n");
                c_pointer = c_pointer->c_ptr;
                c_pointer->cid = *ptr;
                c_pointer->c_ptr = NULL;
                //printk("Container creation completed.\n");
                //Now associate the current task to the container
                //Create a new task node
                //printk("Task association started.\n");
                c_pointer->link = (task *)kmalloc(sizeof(task), GFP_KERNEL);
                //printk("KMALLOC worked fine for task creation.\n");
                t_pointer = c_pointer->link;
                t_pointer->tid = current->pid;
                t_pointer->task_details = current;
                //printk("Current Thread %d inserted\n", t_pointer->tid);
                t_pointer->t_ptr = NULL;
                //printk("Task association completed.\n");
            }
        }
    }

    //Before unlocking, make an entry of the task in the container_insert function
    container_insert(current->pid, *ptr);

    //Printing the list after create complete
    //printk("Printing the list after create complete:\n");
    printlist();

    /* Unlock */
    mutex_unlock(&my_mutex);
    //printk("Create - Lock released\n");

    return 0;
}

/**
 * switch to the next task in the next container
 * 
 * external functions needed:
 * mutex_lock(), mutex_unlock(), wake_up_process(), set_current_state(), schedule()
 */
int processor_container_switch(struct processor_container_cmd __user *user_cmd)
{
    /* Lock */
    if(mutex_lock_interruptible(&my_mutex)){
        return -ERESTARTSYS;
    }
    //printk("Switch - Lock held\n");

    //printk("Switch Started!! - Current Task: %d\n", current->pid);
    //Get the current thread and then find the container to which the current thread belongs
    // print_container_lookup();
    long long unsigned int cid = container_lookup(current->pid);
    //printk("Container ID after lookup is: %llu\n", cid);
    if(cid == ULLONG_MAX){
        mutex_unlock(&my_mutex);
        //printk("Since cid equals ULLONG_MAX, no proceeding forward in code. Value: %llu\n", cid);
        //printk("Switch - Lock released\n");
        return 0;
    }
    container *ptr = head;
    container *prev_ptr = NULL;
    //Again, ptr should never be NULL. While looping, we should always find a node and never reach the loop end

    while(ptr->cid != cid){
        prev_ptr = ptr;
        ptr = ptr->c_ptr;

        if(ptr == NULL){
            //Error condition
            break;
        }
    }
    if(ptr->cid == cid){
        //printk("Reached the required container: %llu\n", cid);
        task *t_link = ptr->link;
        task *t_prev_link = NULL;
        while(t_link->tid != current->pid){
            t_prev_link = t_link;
            t_link = t_link->t_ptr;
            if(t_link == NULL){
                //Error condition
                break;
            }
        }
        if(t_link->tid == current->pid){
            //printk("Reached the required task (Sleep this task if not the top task): %d . Wake up the task: %d\n", t_link->tid, (ptr->link)->tid);
            bool top_task = false; //To check if the current selected task is the top task
            if(ptr->link == t_link){
                //Meaning this is the 1st task in the container. Just update the top_task variable to true
                top_task = true;
                //printk("I am the top task\n");
            }
            //Now, always (meaning always !!) wake up the top-most process (make it running) [if already not woken up] and then put 
            //that process to the back of the list so that the next time, we schedule the task that is currently on top of the list 
            //(earlier it was 2nd on the list)
            task *task_list_top = ptr->link;
            if(!top_task){
                //printk("More than 1 task - Wake up task %d !!!\n", task_list_top->tid);
                //printk("Wake up task id: %d\n", task_list_top->tid);
                wake_up_process(task_list_top->task_details);
            }
            task *replacement_node = (task *)kmalloc(sizeof(task), GFP_KERNEL);
            replacement_node->tid = task_list_top->tid;
            replacement_node->task_details = task_list_top->task_details;
            replacement_node->t_ptr = NULL;
            //Now, go to the end of this list and insert the replacement task
            while(task_list_top->t_ptr != NULL){
                task_list_top = task_list_top->t_ptr;
            }
            task_list_top->t_ptr = replacement_node;
            //Now assign head node (ptr->link) to the 2nd element. In case of only 1 element, it will point back to the 1st element
            ptr->link = (ptr->link)->t_ptr;

            //Printing the list after switch complete but before releasing the lock
            //printk("Switch Completed!!\n");
            //printk("Printing the list after switch complete:\n");
            printlist();

            if(!top_task){
                //printk("Current task is not the top task and more than 1 task - Sleep task %d !!!\n", current->pid);
                //Any other node. Put that node to sleep.
                set_current_state(TASK_INTERRUPTIBLE);
                /* Unlock */
                mutex_unlock(&my_mutex);
                //printk("Switch - Lock released\n");
                //printk("Sleeping task id: %d\n", current->pid);
                schedule();
            }
            else{
                /* Unlock */
                mutex_unlock(&my_mutex);
                //printk("Switch - Lock released\n");
            }
        }
    }
    return 0;
}

/**
 * control function that receive the command in user space and pass arguments to
 * corresponding functions.
 */
int processor_container_ioctl(struct file *filp, unsigned int cmd,
                              unsigned long arg)
{
    switch (cmd)
    {
    case PCONTAINER_IOCTL_CSWITCH:
        return processor_container_switch((void __user *)arg);
    case PCONTAINER_IOCTL_CREATE:
        return processor_container_create((void __user *)arg);
    case PCONTAINER_IOCTL_DELETE:
        return processor_container_delete((void __user *)arg);
    default:
        return -ENOTTY;
    }
}
