#include <iostream>
#include <list>
#include <iterator>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>


#define NUM_SECONDS 20

#define WRITE(a) { const char *foo = a; write (1, foo, strlen (foo)); }

// make sure the asserts work
#undef NDEBUG
#include <assert.h>

#define EBUG
#ifdef EBUG
#   define dmess(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << a << endl;

#   define dprint(a) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << (#a) << " = " << a << endl;

#   define dprintt(a,b) cout << "in " << __FILE__ << \
    " at " << __LINE__ << " " << a << " " << (#b) << " = " \
    << b << endl
#else
#   define dprint(a)
#endif /* EBUG */

using namespace std;

enum STATE { NEW, RUNNING, WAITING, READY, TERMINATED };

/*
** a signal handler for those signals delivered to this process, but
** not already handled.
*/
void grab (int signum) { dprint (signum); }

// c++decl> declare ISV as array 32 of pointer to function (int) returning
// void
void (*ISV[32])(int) = {
/*        00    01    02    03    04    05    06    07    08    09 */
/*  0 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 10 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 20 */ grab, grab, grab, grab, grab, grab, grab, grab, grab, grab,
/* 30 */ grab, grab
};

struct PCB
{
    STATE state;
    const char *name;   // name of the executable
    int pid;            // process id from fork();
    int ppid;           // parent process id
    int interrupts;     // number of times interrupted
    int switches;       // may be < interrupts
    int started;        // the time this process started
    int p2k[2];         // pipe process to kernal
    int k2p[2];         // pipe from kernal to process
};

/*
** an overloaded output operator that prints a PCB
*/
ostream& operator << (ostream &os, struct PCB *pcb)
{
    os << "state:        " << pcb->state << endl;
    os << "name:         " << pcb->name << endl;
    os << "pid:          " << pcb->pid << endl;
    os << "ppid:         " << pcb->ppid << endl;
    os << "interrupts:   " << pcb->interrupts << endl;
    os << "switches:     " << pcb->switches << endl;
    os << "started:      " << pcb->started << endl;
    return (os);
}

/*
** an overloaded output operator that prints a list of PCBs
*/
ostream& operator << (ostream &os, list<PCB *> which)
{
    list<PCB *>::iterator PCB_iter;
    for (PCB_iter = which.begin(); PCB_iter != which.end(); PCB_iter++)
    {
        os << (*PCB_iter);
    }
    return (os);
}

PCB *running;
PCB *idle;

// http://www.cplusplus.com/reference/list/list/
list<PCB *> new_list;
list<PCB *> processes;

int sys_time;

/*
**  send signal to process pid every interval for number of times.
*/
void send_signals (int signal, int pid, int interval, int number)
{
    dprintt ("at beginning of send_signals", getpid ());

    for (int i = 1; i <= number; i++)
    {
        sleep (interval);

        dprintt ("sending", signal);
        dprintt ("to", pid);

        if (kill (pid, signal) == -1)
        {
            perror ("kill");
            return;
        }
    }
    dmess ("at end of send_signals");
}

/*
** Async-safe integer to a string. i is assumed to be positive. The number
** of characters converted is returned; -1 will be returned if bufsize is
** less than one or if the string isn't long enough to hold the entire
** number. Numbers are right justified. The base must be between 2 and 16;
** otherwise the string is filled with spaces and -1 is returned.
*/
int eye2eh (int i, char *buf, int bufsize, int base)
{
    if (bufsize < 1) return (-1);
    buf[bufsize-1] = '\0';
    if (bufsize == 1) return (0);
    if (base < 2 || base > 16)
    {
        for (int j = bufsize-2; j >= 0; j--)
        {
            buf[j] = ' ';
        }
        return (-1);
    }

    int count = 0;
    const char *digits = "0123456789ABCDEF";
    for (int j = bufsize-2; j >= 0; j--)
    {
        if (i == 0)
        {
            buf[j] = ' ';
        }
        else
        {
            buf[j] = digits[i%base];
            i = i/base;
            count++;
        }
    }
    if (i != 0) return (-1);
    return (count);
}

struct sigaction *create_handler (int signum, void (*handler)(int))
{
    struct sigaction *action = new (struct sigaction);

    action->sa_handler = handler;
/*
**  SA_NOCLDSTOP
**  If  signum  is  SIGCHLD, do not receive notification when
**  child processes stop (i.e., when child processes  receive
**  one of SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU).
*/
    if (signum == SIGCHLD)
    {
        action->sa_flags = SA_NOCLDSTOP;
    }
    else
    {
        action->sa_flags = 0;
    }

    sigemptyset (&(action->sa_mask));

    assert (sigaction (signum, action, NULL) == 0);
    return (action);
}

PCB* choose_process ()
{
    /*  b) If there are any processes on the new_list, do the following.
        i) Take the one off the new_list and put it on the processes list.
        ii) Change its state to RUNNING, and fork() and execl() it. */
    
    if( !new_list.empty() ){ //if new list contains any items
        processes.push_front( new_list.front() ); 
        new_list.pop_front();
        running = processes.front();
        running->state = RUNNING;
        running->started = sys_time;
        running->ppid = getpid();
        
        pid_t childPID = fork();
        if( childPID == 0){  //child process

           close (running->p2k[0]); //close read end of process to kernal pipe
           close (running->k2p[1]); //close write end of kernal to process pipe

            // assign fildes 3 and 4 to the pipe ends in the child
            dup2 (running->p2k[1], 3);
            dup2 (running->k2p[0], 4);

            execl(running->name, running->name, NULL);
        }
        else if(childPID < 0){ //error
            perror("fork");
        }
        else{ //parent
            running->pid = childPID;
            close(running->p2k[1]); //close write end of the process to kernal pipe
            close(running->k2p[0]); //close read end of kernal to process pipe
        }
        return running;
    }

    /*  c) Modify choose_process to round robin the processes in the processes
        queue that are READY. If no process is READY in the queue, execute
        the idle process.  */
    if( new_list.empty() && !processes.empty() ){ //if new list is empty and proccesses contains items
        
        //linear search slow if processes list had a lot of items

        list<PCB *>::iterator i;
        for (i = processes.begin(); i != processes.end(); i++){
            if( (*i)->state == READY){
                    //cout << i;
                    running = (*i);
                    running->state = RUNNING;
                    running->switches++;
                    processes.remove(*i);
                    processes.push_back(*i);
                    return running;
                }else{}
            }
        
        /* Moodle didnt like this version
        for(PCB * i : processes){
            if(i->state == READY){
                //cout << i;
                foundReady = true;
                running = i;
                running->state = RUNNING;
                running->switches++;
                processes.remove(i);
                processes.push_back(i);
                return running;
            }else{}
        }
        */
    } //end if

    return idle;
}


void scheduler (int signum)
{
    assert (signum == SIGALRM);
    sys_time++;

    /* a) Update the PCB for the process that was interrupted including the
        number of context switches and interrupts it had, and changing its
        state from RUNNING to READY. */

    running->interrupts++;
    running->state = READY;
    running->switches++;

    PCB* tocont = choose_process();

    dprintt ("continuing", tocont->pid);
    if (kill (tocont->pid, SIGCONT) == -1)
    {
        perror ("kill");
        return;
    }


}

void kernal_call (int signum){
    assert (signum == SIGTRAP);

    WRITE("---- entering kernal_call\n");

    /*  iterate through the PCB on the processes list
        if they have stuff in the pipe then respond to it  

        first build list of process names
        probobly belongs somewhere else in code */
    
    ostringstream proc_names;

    list<PCB *>::iterator ii;
    for(ii = processes.begin(); ii != processes.end(); ii++){
        proc_names << (*ii)->name << "\n" ;
    }    

    list<PCB *>::iterator i;
    for (i = processes.begin(); i != processes.end(); i++){
        char buf[1024];
        int num_read = read((*i)->p2k[0], buf, 1023);
        if (num_read > 0){
            buf[num_read] = '\0';
            WRITE("**** *** ** * kernel read: \n");
            WRITE(buf);
            WRITE("\n**** *** ** * \n");
                // request
            string req(buf);
            
                // respond
            ostringstream os;
            if(req == "pslist"){ //process list request
                os << "\ncurrent processes:\n" << proc_names.str();
            }
            else if(req == "systime"){ //system time request
                os << "\nsystem time of response:" << sys_time << "\n" ;
            }else{}

            string s = os.str();
            char *message = new char [s.length()+1];
            std::strcpy (message, s.c_str());
            write ((*i)->k2p[1], message, strlen (message));
            delete[] (char*)message;
        }            
    }

    WRITE("---- leaving kernal_call\n");
}



void process_done (int signum)
{
    assert (signum == SIGCHLD);

    int status, cpid;

    cpid = waitpid (-1, &status, WNOHANG);

    dprintt ("in process_done", cpid);

    if  (cpid == -1)
    {
        perror ("waitpid");
    }
    else if (cpid == 0)
    {
        if (errno == EINTR) { return; }
        perror ("no children");
    }
    else
    {
        dprint (WEXITSTATUS (status));

            char buf[10];
            assert (eye2eh (cpid, buf, 10, 10) != -1);
            WRITE("process exited:");
            WRITE(buf);
            WRITE("\n");
    }

    /*  4) When a SIGCHLD arrives notifying that a child has exited, process_done() is
    called. process_done() currently only prints out the PID and the status.
    a) Add the printing of the information in the PCB including the number
        of times it was interrupted, the number of times it was context
        switched (this may be fewer than the interrupts if a process
        becomes the only non-idle process in the ready queue), and the total
        system time the process took.
    b) Change the state to TERMINATED.
    c) Start the idle process to use the rest of the time slice.     */

    //another linear search 

    list<PCB *>::iterator p_iter;
    for (p_iter = processes.begin(); p_iter != processes.end(); p_iter++)
    {
        if((*p_iter)->pid == cpid){
            (*p_iter)->state = TERMINATED;
            std::cout<< *p_iter;
            std::cout << "total system time used: "
              << (sys_time - (*p_iter)->started + 1)
              << std::endl ;
            break;
        }
    }

    /*  Moodle didnt like this version because its C11 only
    for(PCB * p : processes){
        if(p->pid == cpid){
            p->state = TERMINATED;
            std::cout<< p;
            std::cout << "total system time used: "
              << (sys_time - p->started + 1)
              << std::endl ;
            break;
        } 
    } */
    
    running = idle;
}

/*
** stop the running process and index into the ISV to call the ISR
*/
void ISR (int signum)
{
    if (kill (running->pid, SIGSTOP) == -1)
    {
        perror ("kill");
        return;
    }
    dprintt ("stopped", running->pid);

    ISV[signum](signum);
}

/*
** set up the "hardware"
*/
void boot (int pid)
{
    ISV[SIGALRM] = scheduler;       create_handler (SIGALRM, ISR);
    ISV[SIGCHLD] = process_done;    create_handler (SIGCHLD, ISR);
    ISV[SIGTRAP] = kernal_call;     create_handler (SIGTRAP, ISR);

    // start up clock interrupt
    int ret;
    if ((ret = fork ()) == 0)
    {
        // signal this process once a second for three times
        send_signals (SIGALRM, pid, 1, NUM_SECONDS);

        // once that's done, really kill everything...

        list<PCB *>::iterator p_iter;
        for (p_iter = processes.begin(); p_iter != processes.end(); p_iter++){
            delete(*p_iter);
            *p_iter = NULL;
        }

        delete(idle);

        kill (0, SIGTERM); 
    }

    if (ret < 0)
    {
        perror ("fork");
    }
}

void create_idle ()
{
    int idlepid;

    if ((idlepid = fork ()) == 0)
    {
        dprintt ("idle", getpid ());

        // the pause might be interrupted, so we need to
        // repeat it forever.
        for (;;)
        {
            dmess ("going to sleep");
            pause ();
            if (errno == EINTR)
            {
                dmess ("waking up");
                continue;
            }
            perror ("pause");
        }
    }
    idle = new (PCB);
    idle->state = RUNNING;
    idle->name = "IDLE";
    idle->pid = idlepid;
    idle->ppid = 0;
    idle->interrupts = 0;
    idle->switches = 0;
    idle->started = sys_time;
}

int main (int argc, char *argv[])
{
    int pid = getpid();
    dprintt ("main", pid);

    sys_time = 0;

    boot (pid);

    /* 2) Take any number of arguments for executables, and place each on new_list.
    The executable will not require arguments themselves. */
    int i = 1;
    while(argv[i] != NULL){
        PCB * pTemp = new(PCB);
        pTemp->state = NEW;
        pTemp->name = argv[i++];
        pTemp->pid = 0;
        pTemp->ppid = 0;
        pTemp->interrupts = 0;
        pTemp->switches = 0;
        pTemp->started = 0;

        //create pipes
        assert( pipe(pTemp->p2k) == 0); // process to kernal
        assert( pipe(pTemp->k2p) == 0); // kernal to process

        // make the read end of the p2K pipe non-blocking. magic or something idk
        assert (fcntl (pTemp->p2k[0], F_SETFL, 
                            fcntl(pTemp->p2k[0], F_GETFL) | O_NONBLOCK) == 0);


        new_list.push_front (pTemp);
    }

    // create a process to soak up cycles
    create_idle ();
    running = idle;

    std::cout << running;

    // we keep this process around so that the children don't die and
    // to keep the IRQs in place.
    for (;;)
    {
        pause();
        if (errno == EINTR) { continue; }
        perror ("pause");
    }
}
