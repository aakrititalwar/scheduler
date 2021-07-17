#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>
#include <cstring>
#include <cctype>
#include <string>
#include <bits/stdc++.h>
#include <queue>
using namespace std;

enum trans_state {CREATED, READY, RUN, BLOCK, PREEMPT, DONE};

struct Process {
    int arrival_time;
    int cpu_time;
    int CPU_burst;
    int IO_burst;
    int curr_io_burst;
    int curr_cpu_burst;
    int rem;
    int state_ts;
    int total_IO_time=0;
    int pid;
    int static_prio;
    int dynamic_prio;
    int FT;
    int WT;
    trans_state PrevState;
    bool preempt;
    int elapsed_time;

};

struct Event {
    int evtTimeStamp;
    Process* evtProcess;
    trans_state trans_from;
    trans_state trans_to;
};



//queue <Event*> event_queue;
list <Event*> event_list;
list <Process*> run_queue;
vector <Process*> pro_vector;
//queue <Process> pro_queue;
FILE* rfile;
FILE* inpfile;
int CURRENT_TIME = 0;
bool CALL_SCHEDULER = false;
int ofs;
Process* CURRENT_RUNNING_PROCESS = NULL;
int rlinenum;
int randvals[40000]; 
static char linebuf[1024];
static char rbuf[1024];
int timeinPrevState;
const char* DELIM = " \t\n\r\v\f";
map<int, string> state_map;
int maxprio = 4;
int time_quantum = 10000;
vector<list<Process*> >* active_queue;
vector<list<Process*> >* expired_queue;
map<int,int> ioValues;
int totalIoTime;
bool verbose = false;

//int pro_num_counter = -1;

class scheduler{
public :
    string type;
    virtual void add_to_run_queue(Process* proc){
        printf("No scheduler type specified");
    }
    virtual Process* get_next_process(){
        if (run_queue.empty()){
            return NULL;
        }
        Process* p = run_queue.front();
        run_queue.pop_front();
        return p;
    }
    virtual bool testpreempt(){
        return false;
    }

};

class FCFS : public scheduler{
public :
    FCFS(){
        this->type = "FCFS";
    }
    void add_to_run_queue(Process* proc){
        run_queue.push_back(proc);
    }
    //int time_quantum = 6;
};

class LCFS : public scheduler{
    public :
    LCFS(){
        this->type = "LCFS";
    }
    void add_to_run_queue(Process* proc){
        run_queue.push_front(proc);
    }

};

class RR : public scheduler{
    public :
    
    RR(){
        this->type = "RR";
    }
    void add_to_run_queue(Process* proc){
        if(proc->dynamic_prio == -1 and proc->PrevState == PREEMPT){
            proc->dynamic_prio = proc->static_prio - 1;
        }
        run_queue.push_back(proc);
    }
};

class SRTF : public scheduler{
    public :
    SRTF(){
        this->type = "SRTF";
    }
    void add_to_run_queue(Process* proc){
         list<Process*>::iterator itr = run_queue.begin();  
        if(run_queue.empty()){
            run_queue.insert(itr, proc);
            return;
        }
            while( itr != run_queue.end()) {
            if(((*itr)->rem)>proc->rem){
                run_queue.insert(itr, proc);
                break;
            }
            advance(itr, 1);
        }
        if(itr == run_queue.end()){
            run_queue.insert(itr, proc);
        } 
        
    }

};

class PRIO : public scheduler{
    
     vector<list<Process*> >* active_queue;
     vector<list<Process*> >* expired_queue;
     vector<list<Process*> > a_vector;
     vector<list<Process*> > e_vector;
    public :

    PRIO(){

    this->type = "PRIO";
    //cout << "maxprio" << maxprio << endl;
    for(int i = 0;i<maxprio;i++){
        a_vector.push_back(list<Process*>());
        e_vector.push_back(list<Process*>());
    }
    active_queue = &a_vector;
    expired_queue = &e_vector;
    }

    void add_to_run_queue(Process* p){
        //cout << "dp"<< p->dynamic_prio  << endl;
        if(p->dynamic_prio < 0){
            p->dynamic_prio = p->static_prio - 1;
            (expired_queue->at(p->dynamic_prio)).push_back(p);
        }
        else{
            (active_queue->at(p->dynamic_prio)).push_back(p);
        }
    }

    void switch_queue(){
        vector<list<Process*> >* temp;
        temp = active_queue;
        active_queue = expired_queue;
        expired_queue = temp;
    }

    Process* get_next_process(){
        
        Process* p;
        for(int i = active_queue->size() - 1; i>=0; i--){
            if(active_queue->at(i).empty()){
                continue;
            }
            else{
                p = (active_queue->at(i)).front();
                (active_queue->at(i)).pop_front();
                return p;
            }
        }
        //printf("None found");
        switch_queue();

        for(int i = active_queue->size() - 1; i>=0; i--){
            if(active_queue->at(i).empty()){
                continue;
            }
            else{
                p = (active_queue->at(i)).front();
                (active_queue->at(i)).pop_front();
                return p;
            }
        }
        return NULL;

        
    }

};

class PREPRIO : public PRIO {
    public :
    PREPRIO(){
        //printf("hey");
        this->type = "PREPRIO";
    }
    bool testpreempt(){
        return true;
    }

};

// class PRIO : public scheduler{

// }

scheduler* THE_SCHEDULER;

 int myrandom(int burst) {
     if(ofs >= rlinenum){
         ofs = rlinenum - ofs;
     }
    //cout << "in random function " << ofs;
    int random = 1 + (randvals[ofs] % burst); 
    ofs = ofs + 1;
    return random;
    }

void print_run_queue(){
    for (list<Process*>::iterator i = run_queue.begin();
         i != run_queue.end();
         i++)
        cout << (*i)->arrival_time << " ";
  
    cout << endl;
}

void print_list(){
    for (list<Event*>::iterator i = event_list.begin();
         i != event_list.end();
         i++){
        cout << (*i)->evtTimeStamp << " " << endl;
        cout << (*i)->trans_from << endl;
        cout << (*i)->trans_to << endl;
        cout << ((*i)->evtProcess)->pid << endl;
         }
        //printf("hi");
    cout << endl;
    return;
}

void insert_to_EQ(Event * e){
    list<Event*>::iterator itr = event_list.begin();  
        if(event_list.empty()){
            event_list.insert(itr, e);
            //print_list();
            return;
        }
            while( itr != event_list.end()) {
            if(((*itr)->evtTimeStamp)>e->evtTimeStamp){
                event_list.insert(itr, e);
                //print_list();
                return;
            }
            advance(itr, 1);
        }
        if(itr == event_list.end()){
            //printf("HI event_list end") ;   
            event_list.insert(itr, e);
            //print_list();
            
}
        }

bool check_event_same_ts(Process* p, Process* CurrRunPro){
    list<Event*>:: iterator i = event_list.begin();
    while (i != event_list.end()){
        if((((*i)->evtProcess)->pid == CurrRunPro->pid) && ((*i)->evtTimeStamp == CURRENT_TIME)){
            //cout << "10" << endl; 
            return true;
        }
        else if((*i)->evtTimeStamp> CURRENT_TIME){
            //cout << "20" << endl;
            return false;
        }
        else{
            i++;
        }
    }
    return false;
}

void delete_future_event(Process* p){
    list<Event*>::iterator it = event_list.begin();
    while(it != event_list.end()){
        if(((*it)->evtProcess)->pid == p->pid){
            event_list.erase(it);
            break;
        }
        it++;
    }

}
        
bool checkpreempt(Process* p, Process* CurrRunPro){
    if(CurrRunPro == NULL){
        return false;
    }
    if(THE_SCHEDULER->testpreempt() && !(p->preempt)){
        //cout << "yo" << endl;
       if((p->dynamic_prio > CurrRunPro->dynamic_prio) && !(check_event_same_ts(p, CurrRunPro))){
           //cout << "yo" << endl;
           return true;
       }
    }
    return false;
}

void add_event(int evt_ts, trans_state tra_from, trans_state tra_to, Process* proc){
    Event* evt = new Event();
    evt->evtTimeStamp = evt_ts;
    evt->trans_from = tra_from;
    evt->trans_to = tra_to;
    evt->evtProcess = proc;
    insert_to_EQ(evt);
    //cout << "returned " << endl;
}

void print_pro_vector(){
    //printf("Hi");
    for(int i=0; i < pro_vector.size(); i++){
        Process* proc = pro_vector.at(i);
        cout << proc->arrival_time << proc->cpu_time << proc->CPU_burst << proc->IO_burst;
    }
}

void parseInput(int argc, char *argv[]) {
    int flag;
    while ((flag = getopt(argc, argv, "s:")) != -1) {
        char temp;
        char schedulerType;
        if (flag == 's') {
            // cout << optarg << endl;
            schedulerType = optarg[0];
            if (schedulerType == 'F') {
                THE_SCHEDULER = new FCFS();
            } else if (schedulerType == 'L') {
                THE_SCHEDULER = new LCFS();
            } else if (schedulerType == 'S') {
                THE_SCHEDULER = new SRTF();
            } else if (schedulerType == 'R') {
                THE_SCHEDULER = new RR();
                sscanf(optarg, "%c%d", &temp, &time_quantum);
            } else if (schedulerType == 'P') {
                sscanf(optarg, "%c%d:%d", &temp, &time_quantum, &maxprio);
                THE_SCHEDULER = new PRIO();
            } else if (schedulerType == 'E') {
                sscanf(optarg, "%c%d:%d", &temp, &time_quantum, &maxprio);
                THE_SCHEDULER = new PREPRIO();
            }
        } else {
            cout << "Invalid Argument";
            exit(0);
        }
    }
    // read the input files
}

void readinpfile(){
    int i = -1;
    while(fgets(linebuf,1024, inpfile)){
        i++;
        Process* p = new Process();
        char* tok = strtok(linebuf, DELIM);
        p->arrival_time = atoi(tok);
        char* ct = strtok(NULL, DELIM);
        p->cpu_time = atoi(ct);
        char* cb = strtok(NULL, DELIM);
        p->CPU_burst = atoi(cb);
        char* ib = strtok(NULL, DELIM);
        p->IO_burst = atoi(ib);
        p->curr_io_burst = 0;
        p->curr_cpu_burst = 0;
        p->rem = p->cpu_time;
        p->state_ts = p->arrival_time;
        p->pid = i;
        p->static_prio = myrandom(maxprio);
        p->WT = 0;
        p->preempt = false;
        p->elapsed_time = -1;
        //cout << "Static Prio " << i <<" " << p->static_prio << " Offset" << ofs << endl;
        p->dynamic_prio = p->static_prio-1;
        //cout << p->pid << p->static_prio << p->dynamic_prio << endl;
        pro_vector.insert(pro_vector.begin()+i,p);
        add_event(p->arrival_time, CREATED, READY, p);
        //pro_queue.push(p);
        //delete p;
    }
    //print_pro_vector(); 
    //cout << "returned" << endl;
} 

void initialize_sheduler(int option){
    switch (option) {
        case 1:
        THE_SCHEDULER = new FCFS();
        //printf("FCFS");
        break;
        case 2 :
        THE_SCHEDULER = new LCFS();
        break;
        case 3 :
        THE_SCHEDULER = new SRTF();
        break;
        case 4 :
        THE_SCHEDULER = new RR();
        break;
        case 5 :
        THE_SCHEDULER = new PRIO();
        break;
        case 6 :
        THE_SCHEDULER = new PREPRIO();
        break;
        default :
        THE_SCHEDULER = new scheduler();
    }   
}

void initialize_state_map(){
         state_map.insert({0,"CREATED"});
         state_map.insert({1,"READY"});
         state_map.insert({2,"RUN"});
         state_map.insert({3,"BLOCK"});
         state_map.insert({4,"PREEMPT"});
         state_map.insert({5,"DONE"});
         //cout << "map" << endl;

}

Event* get_event(){
    return event_list.front();
}

int get_next_event_time(){
    if(event_list.empty()){
        return -1;
    }

    return (event_list.front())->evtTimeStamp;
}

// int readrfile_linenum(){
//        int i = 0;
//        while(fgets(rbuf,1024, rfile)){
//         i++;
//        }
//        return i;
// }

void readrfile(){
    
    int i = 0;
    fgets(rbuf,1024, rfile);
    char* tok = strtok(rbuf, DELIM);
    rlinenum = atoi(tok);
    while(fgets(rbuf,1024, rfile)){
        char* tok = strtok(rbuf, DELIM);
        randvals[i] = atoi(tok);
        //cout << randvals[i] << endl;
        i++;
       }
}

// void sim(){
//     //printf("hi");
//     Event* evt;
//     while(evt = get_event()){
//         cout << evt->evtTimeStamp;
//         Process *proc = evt->evtProcess;
//         CURRENT_TIME = evt->evtTimeStamp;
//         switch (evt->trans_to){
//             case READY:
//             {
//                 printf("Case READY");


//                 THE_SCHEDULER->add_to_run_queue(proc);
//                 CALL_SCHEDULER = true;
//                 break;
//             }
//             case RUN:
//             // create event for either preemption or blocking
//             {
//             printf("Case RUN");
//             cout << proc->IO_burst << endl;
//             int ib = myrandom(proc->IO_burst);
//             cout << "ib=" << ib;
//             proc-> curr_io_burst = ib;
//             add_event(CURRENT_TIME+proc->curr_cpu_burst,RUN,BLOCK,proc);
//             break;
//             }
//             case BLOCK:
//             {
//                 printf("Case BLOCK");
//                 break;
//             }

//         }
//         evt = NULL;
//         event_list.pop_front();
//         //cout << evt->trans_to;
//     }
// }

void simulation(){
    Event* evt;
    //printf("hi");
    while (evt = get_event()){
        Process *proc = evt->evtProcess;
        //cout << "ra" << endl;
        CURRENT_TIME = evt->evtTimeStamp;
        proc->PrevState = evt->trans_from;
        timeinPrevState = CURRENT_TIME - proc->state_ts;
        if(verbose)
        cout << CURRENT_TIME << " " <<proc->pid << " " << timeinPrevState << " " << state_map[evt->trans_from]<<"->" << state_map[evt->trans_to] ;
        //print_list();
        event_list.pop_front();
        //cout << "Popped " << evt->trans_to  << endl;
        switch(evt->trans_to) { // which state to transition to?

        case READY:
        // must come from BLOCKED or from PREEMPTION
        // must add to run queue
        {
       //printf("Case READY");
       if(proc->PrevState == BLOCK ){
           proc->dynamic_prio = proc->static_prio - 1;
       }
       //cout << "cb=" << proc->curr_cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
       //cout << "before checking print event_list" << endl;
       //print_list();
    //    if(CURRENT_RUNNING_PROCESS != NULL){
    //    cout << "1 " << CURRENT_RUNNING_PROCESS->dynamic_prio << "2 " << proc->dynamic_prio << endl;
    //    }
       //cout << "yay" << endl;
       
       
       if(checkpreempt(proc,CURRENT_RUNNING_PROCESS)){
           if(verbose)
           cout << "----------------preemption YES--------------------" << CURRENT_RUNNING_PROCESS->pid << "by" << proc->pid << endl;
           delete_future_event(CURRENT_RUNNING_PROCESS);   
           CURRENT_RUNNING_PROCESS->preempt = true;
           CURRENT_RUNNING_PROCESS->elapsed_time = CURRENT_TIME - CURRENT_RUNNING_PROCESS->state_ts;
           add_event(CURRENT_TIME, RUN, PREEMPT, CURRENT_RUNNING_PROCESS);
            //cout << "ET " << CURRENT_RUNNING_PROCESS->elapsed_time;
            // CURRENT_RUNNING_PROCESS->curr_cpu_burst = CURRENT_RUNNING_PROCESS->curr_cpu_burst - elapsed_time;
            // CURRENT_RUNNING_PROCESS->rem = CURRENT_RUNNING_PROCESS->rem - elapsed_time;
       }
       proc->preempt = false;
       //cout << "pp" << endl;
       //p->preempt = false
       proc->state_ts = CURRENT_TIME;
       THE_SCHEDULER->add_to_run_queue(proc);
       CALL_SCHEDULER = true; // conditional on whether something is run
        break;
        }
        case RUN:
        // create event for either preemption or blocking
        {
        //printf("Case RUN");
        //if(proc->rem)
        proc->WT = proc->WT + timeinPrevState;
        if(proc->curr_cpu_burst == 0){
                   int cb = myrandom(proc->CPU_burst);
                   if(cb>proc->rem){
                    cb = proc->rem;
                    }
               //cout << "cb =" << cb << " rem=" <<  CURRENT_RUNNING_PROCESS->rem << endl;
                    proc->curr_cpu_burst = cb;
        }
        proc->state_ts = CURRENT_TIME;
        if(verbose)
        cout << "cb=" << proc->curr_cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
        if(time_quantum<proc->curr_cpu_burst){
            add_event(CURRENT_TIME+time_quantum, RUN, PREEMPT, proc);
            
            //proc->rem = proc->rem - time_quantum;
        }
        else if(proc->rem == proc->curr_cpu_burst){
            add_event(CURRENT_TIME+proc->curr_cpu_burst,RUN,DONE,proc);
        }
        else {
            add_event(CURRENT_TIME+proc->curr_cpu_burst,RUN,BLOCK,proc);
        }

        break;
        }
        case BLOCK:
        {   
            //printf("CASE BLOCK");
            int ib = myrandom(proc->IO_burst);
            proc-> curr_io_burst = ib;
            proc->rem = proc->rem - proc->curr_cpu_burst;
            CURRENT_RUNNING_PROCESS = NULL; 
            proc->curr_cpu_burst = 0; 
            proc->state_ts = CURRENT_TIME;
            if(verbose)
            cout << " ib=" << proc->curr_io_burst <<"rem=" << proc->rem;
            proc->total_IO_time = proc->total_IO_time + proc->curr_io_burst;
            //cout << CURRENT_TIME+proc->curr_io_burst << endl;
            ioValues.insert(pair<int,int>(CURRENT_TIME, CURRENT_TIME + ib));
            add_event(CURRENT_TIME+proc->curr_io_burst, BLOCK, READY,proc);
            //printf("hi4");
        //create an event for when process becomes READY again
             CALL_SCHEDULER = true;
             break;
        }
        case PREEMPT:
        // add to runqueue (no event is generated)
        {
        //printf("CASE PREEMPT");
        //THE_SCHEDULER->add_to_run_queue(proc);
        if(proc->preempt){
            proc->curr_cpu_burst = proc->curr_cpu_burst - proc->elapsed_time;
            proc->rem = proc->rem - proc->elapsed_time;
            //proc->preempt = false;
        }
        else{
            proc->curr_cpu_burst = proc->curr_cpu_burst - time_quantum;
            proc->rem = proc->rem - time_quantum;
        }      
        CURRENT_RUNNING_PROCESS = NULL;
        proc->dynamic_prio--;
        if(verbose)
        cout << "prio=" <<  proc->dynamic_prio;
        add_event(CURRENT_TIME,PREEMPT,READY,proc);
        CALL_SCHEDULER = true;
        break;
        }
        case DONE :
        {                                                  
            //printf("CASE DONE\n");
            proc->FT = CURRENT_TIME;
            CURRENT_RUNNING_PROCESS = NULL;
            CALL_SCHEDULER = true;
            if (verbose)
            cout << endl;
            break;
        }
        default :
             printf("Invalid State");
        }
        // remove current event object from Memory
        delete evt; evt = NULL;
        //cout << "hi1" << endl;
        //event_list.pop_front();
        //cout << endl;
        if(CALL_SCHEDULER) {
            //cout << "hey " << get_next_event_time() <<  endl;
            if (get_next_event_time() == CURRENT_TIME) {
                //cout << "yo" << endl;
                continue;
            }
             //process next event from Event queue
             //cout << "lala" << endl;
            CALL_SCHEDULER = false; // reset global flag
            if (CURRENT_RUNNING_PROCESS == NULL) {
                //printf("hi2");
            CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
            //cout << "next process pid" << CURRENT_RUNNING_PROCESS->pid;
            if (CURRENT_RUNNING_PROCESS == NULL)
            continue;
                
               
               add_event(CURRENT_TIME, READY, RUN, CURRENT_RUNNING_PROCESS);
               //run_queue.pop_front();
               //cout << "empty run queue" << run_queue.empty() << endl;

            // create event to make this process runnable for same time.
            } 
            
            }      
    }


}

double calCPUutil(){
    int sum = 0;
    for(int i = 0; i<pro_vector.size(); i++){
        Process* p = pro_vector.at(i);
        sum = sum + p->cpu_time;
    }
    double cpuutil;
    //cout << sum << endl;
    //cpuutil = (sum/CURRENT_TIME * 100);
    //cout << cpuutil;
    return (double (sum)/CURRENT_TIME)* 100;

}

double avgtat(){
    int avgtat = 0;
    for(int i = 0; i<pro_vector.size(); i++){
        Process* p = pro_vector.at(i);
        avgtat = avgtat + (p->FT - p->arrival_time);
    }
    //cout << avgtat << endl;
    return (double (avgtat)/pro_vector.size());
}

double avgwt(){
    int avgwt = 0;
    for(int i = 0; i<pro_vector.size(); i++){
        Process* p = pro_vector.at(i);
        avgwt = avgwt + (p->WT);
    }
    return (double (avgwt)/pro_vector.size());

}

void calculateTotalIO(){
    int total=0, totalOverlap=0, firstValue=0, secondValue=0;
//    cout << "mymap contains:\n";
    map<int,int>::iterator it = ioValues.begin();
    firstValue=it->first;
    secondValue=it->second;
    //cout<<secondValue<<endl;
    for (it=ioValues.begin(); it!=ioValues.end(); ++it) {
        total = total + (it->second - it->first);
        if(it->first<secondValue && it!=ioValues.begin()){
            if(it->second<secondValue) {
                totalOverlap=totalOverlap+(it->second-it->first);
            }else {
                totalOverlap = totalOverlap + (secondValue - it->first);
            }
        }
        if(it->second>secondValue) {
            secondValue = it->second;
        }
    }
    //cout <<total <<" overlap: "<<totalOverlap;
    totalIoTime=total-totalOverlap;
}

void print_output(){
    cout << THE_SCHEDULER->type ;
    if(time_quantum == 10000){
        cout << endl;
    }
    else{
        cout << " " << time_quantum << endl;
    }
    for(int i =0; i<pro_vector.size();i++){
        Process* p = pro_vector.at(i);
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",p->pid,p->arrival_time,p->cpu_time,p->CPU_burst,p->IO_burst,p->static_prio,p->FT,(p->FT - p->arrival_time),p->total_IO_time,p->WT);
    }
    calculateTotalIO();
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf",CURRENT_TIME,calCPUutil(),(double (totalIoTime)/CURRENT_TIME) *100, avgtat(), avgwt(), (double (pro_vector.size())/CURRENT_TIME)* 100);
    cout << endl;
}


int main(int argc, char *argv[])
{
    // Process proc1;
    // proc1.arrival_time = 0;
    // cout << "proc1AT" << proc1.arrival_time;
    parseInput(argc, argv);
    rfile = fopen(argv[3], "r");
    inpfile = fopen(argv[2], "r");
    //rlinenum = readrfile_linenum();
    readrfile();
    //cout << "rlinenum" << rlinenum << "ok" << endl;
    //fseek(rfile, 0, SEEK_SET);
    readinpfile();
    int option = 6;
    //initialize_sheduler(option);
    //cout << "TIME Quantum" << time_quantum << endl;
    initialize_state_map();
    //add_event(500, CREATED, RUN, pro_vector.at(1));
    //add_event(0, CREATED, RUN, pro_vector.at(0));
    //printf("Simulation");
    //sim();
    simulation();
    print_output();

    
    
}