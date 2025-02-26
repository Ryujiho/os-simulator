// 2021 Operating System 운영체제
// CPU Schedule Simulator Homework
// Student Number : B836042
// Name : 류지호
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include <unistd.h>
#include <limits.h>

#define SEED 10

// process states
#define S_IDLE 0			
#define S_READY 1
#define S_BLOCKED 2
#define S_RUNNING 3
#define S_TERMINATE 4

//#define DEBUG 1
int NPROC, NIOREQ, QUANTUM;

struct ioDoneEvent {
	int procid;
	int doneTime;
	int len;
	struct ioDoneEvent *prev;
	struct ioDoneEvent *next;
} ioDoneEventQueue, *ioDoneEvent;

struct process {
	int id;
	int len;		// for queue
	int targetServiceTime;
	int serviceTime;
	int startTime;
	int endTime;
	char state;
	int priority;
	int saveReg0, saveReg1;
	struct process *prev;
	struct process *next;
} *procTable;

struct process idleProc;
struct process readyQueue; //NULL = &IDLE
struct process blockedQueue;
struct process *runningProc = NULL;

int cpuReg0, cpuReg1;
int currentTime = 0;
int *procIntArrTime, *procServTime, *ioReqIntArrTime, *ioServTime;

void compute() {
	// DO NOT CHANGE THIS FUNCTION
	cpuReg0 = cpuReg0 + runningProc->id;
	cpuReg1 = cpuReg1 + runningProc->id;
#ifdef DEBUG
	printf("In computer proc %d cpuReg0 %d\n",runningProc->id,cpuReg0);
#endif
}

int IsEmpty(struct process* queue){
	return queue->len == 0;
}
void EnQueue(struct process *queue, struct process *addProcess){
#ifdef DEBUG
	printf("process %d added to Ready Queue.\n", addProcess->id);
#endif
	if(IsEmpty(queue)){
		queue->prev = queue->next = addProcess;
		addProcess->prev = addProcess->next = NULL; //1022
	} else {
	//	queue->prev->next = addProcess;
		addProcess->prev = queue->next;
		addProcess->next = NULL;
		queue->next->next = addProcess;
	}
	queue->next = addProcess;
	queue->len++;
}

///여기 queue없애도 될듯..?
void deQueue(struct process *queue, struct process *deleteProcess){
	if(IsEmpty(queue)){
		return;
	} 
#ifdef DEBUG
	printf("[DEQUEUE] readyQueue length: %d\n", readyQueue.len);
#endif
	struct process* tmp = readyQueue.prev;
	while(tmp != NULL){
		if(tmp->id == deleteProcess->id){
			//Delete & Connect
#ifdef DEBUG
			printf("tmp: %d, delete: %d will be deleted now.\n",tmp->id, deleteProcess->id);
#endif
			if(tmp == readyQueue.prev){
				readyQueue.prev = tmp->next;
				tmp->next = NULL;
				if(readyQueue.prev != NULL)
					readyQueue.prev->prev =  NULL;
			} else if(tmp == readyQueue.next){
				readyQueue.next = tmp->prev;
				tmp->prev = NULL;
				if(readyQueue.next != NULL)
					readyQueue.next->next = NULL;
			} else {
				tmp->next->prev = tmp->prev;
				tmp->prev->next = tmp->next;
			}
#ifdef DEBUG
			printf("process %d deleted in Ready Queue.\n", deleteProcess->id);
#endif
			readyQueue.len--;
			break;
		}
		tmp = tmp->next;
	}

#ifdef DEBUG
	printf("[DEQUEUE] readyQueue length: %d\n", readyQueue.len);
#endif
}

int IsEmptyIO(struct ioDoneEvent* queue){
	return queue->len == 0;
}
		
void EnQueueIO(struct ioDoneEvent *queue, struct ioDoneEvent *addProcess){
#ifdef DEBUG
		printf("process %d added to ioDoneEvent Queue with doneTime = %d \n", addProcess->procid, addProcess->doneTime);
#endif		
		if(IsEmptyIO(queue)){
			queue->prev = addProcess;
		} else {
			addProcess->prev = queue->next;
			queue->next->next = addProcess;
		}
		queue->next = addProcess;
		queue->len++;
	}
	void initProcTable() {
		int i;
		for(i=0; i < NPROC; i++) {
			procTable[i].id = i;
			procTable[i].len = 0;
			procTable[i].targetServiceTime = procServTime[i];
			procTable[i].serviceTime = 0;
			procTable[i].startTime = 0;
			procTable[i].endTime = 0;
			procTable[i].state = S_IDLE;
			procTable[i].priority = 0;
			procTable[i].saveReg0 = 0;
			procTable[i].saveReg1 = 0;
			procTable[i].prev = NULL;
			procTable[i].next = NULL;
		}
	}

	void procExecSim(struct process *(*scheduler)()) {
		int pid, qTime=0, cpuUseTime = 0, nproc=0, termProc = 0, nioreq=0;
		char schedule = 0, nextState = S_IDLE;
		int nextForkTime, nextIOReqTime;
		//int interTime = 0; 
		int cnt_terminate = 0;
		int IS_FORKED = 0, IS_READY = 0; //fork, IO DONE

		nextForkTime = procIntArrTime[nproc];
		nextIOReqTime = ioReqIntArrTime[nioreq];
#ifdef DEBUG
		printf("Initial nextForkTime %d nextIOReqTime %d\n", nextForkTime, nextIOReqTime);
#endif
		while(1) {
			currentTime++;// interTime++;
			if(runningProc !=  NULL){
				cpuUseTime++;
				qTime++;//1012
				compute(); 
				runningProc->serviceTime++;
#ifdef DEBUG
				printf("%d processing Proc %d servT %d targetT %d nproc %d cpuUseTime %d qTime %d\n", currentTime, runningProc->id, runningProc->serviceTime, runningProc->targetServiceTime, nproc, cpuUseTime, qTime);
#endif
			runningProc->saveReg0 = cpuReg0;
			runningProc->saveReg1 = cpuReg1;
		} else {
			#ifdef DEBUG
			printf("%d processing Proc -1 nproc %d cpuUseTime %d \n", currentTime, nproc, cpuUseTime, qTime);
			#endif
		}

		if (currentTime == nextForkTime){ 
			// CASE 2 : a new process created 
#ifdef DEBUG
			printf("****process %d created.\n", nproc);
#endif
			procTable[nproc].startTime = currentTime;
			EnQueue(&readyQueue, &procTable[nproc++]);
			IS_FORKED = 1;
			nextForkTime = procIntArrTime[nproc]+currentTime;
		#ifdef DEBUG
			printf("NEXT Fork Time  %d.\n", nextForkTime);
		#endif
		}
		//	printf("-------------------1\n");

		if (runningProc != NULL && qTime == QUANTUM ) { 
			// CASE 1 : The quantum expires
			#ifdef DEBUG
			printf("**process %d servT %d targetServT %d Quantum Expires\n", runningProc->id, runningProc->serviceTime, runningProc->targetServiceTime);
			#endif
			runningProc->state = S_READY;
			//if(runningProc->priority != 0){
				runningProc->priority--;
			//}
			IS_READY = 1;
		}
		if (runningProc != NULL && nioreq < NIOREQ && cpuUseTime == nextIOReqTime) { 
			// CASE 5: reqest IO operations (only when the process does not terminate)
#ifdef DEBUG
			printf("**process %d request IO Req Event\n", runningProc->id);
#endif
			runningProc->state = S_BLOCKED;
			if(qTime != QUANTUM) runningProc->priority++;
			deQueue(&readyQueue, runningProc);

			ioDoneEvent[nioreq].procid = runningProc->id;
			ioDoneEvent[nioreq].doneTime = ioServTime[nioreq]+currentTime;
			EnQueueIO(&ioDoneEventQueue, &ioDoneEvent[nioreq]);

			if(nioreq < NIOREQ-1){
				nextIOReqTime = ioReqIntArrTime[++nioreq]+cpuUseTime;
				#ifdef DEBUG
				printf("%d-th next IOReq Time : %d\n", nioreq, nextIOReqTime);
				#endif
			}
		}

		struct ioDoneEvent* tmp = ioDoneEventQueue.prev;
		while (tmp != NULL) { //!IsEmptyIO(&ioDoneEventQueue)&& ioDoneEventQueue.prev->doneTime == currentTime) { 
			// CASE 3 : IO Done Event
			if(tmp->doneTime == currentTime) { 
				#ifdef DEBUG
				printf("IO Done Event for pid %d\n", tmp->procid);
				#endif
				if(procTable[tmp->procid].state != S_TERMINATE){
					EnQueue(&readyQueue, &procTable[tmp->procid]);
				}
				if(runningProc != NULL && runningProc->state != S_BLOCKED){
					runningProc->state = S_READY;
				}
				IS_READY = 1;
				//deQueue(&ioDoneEventQueue, tmp);//추가?
				ioDoneEventQueue.len--;
			} 
			tmp = tmp->next;
		}
		if (runningProc != NULL && runningProc->serviceTime == runningProc->targetServiceTime) { 
			// CASE 4 : the process job done and terminates
			#ifdef DEBUG
			printf("**%d-th process terminated at %d : Process %d\n",cnt_terminate+1, currentTime, runningProc->id);
			#endif
			runningProc->state = S_TERMINATE;
			runningProc->endTime = currentTime;
			deQueue(&readyQueue, runningProc);

			if(++cnt_terminate >= NPROC) break;
		}
		//printf("-------------3\n");

		//마지막에 스케줄링
		if( IS_FORKED || IS_READY || runningProc!= NULL && runningProc->state != S_RUNNING){
			if(runningProc != NULL && runningProc->state == S_RUNNING || runningProc != NULL && runningProc->state == S_READY){
				EnQueue(&readyQueue, runningProc);
			}
			
			runningProc = scheduler();
			if(runningProc != NULL){
				cpuReg0 = runningProc->saveReg0;
				cpuReg1 = runningProc->saveReg1;
				runningProc->state = S_RUNNING;	
			}

			qTime = IS_FORKED = IS_READY =  0;
		}
		#ifdef DEBUG
		printf("---------------------\n");
		#endif
		
	} // while loop
}

//RR,SJF(Modified),SRTN,Guaranteed Scheduling(modified),Simple Feed Back Scheduling
struct process* RRschedule() {
	struct process* tmp;
	if(IsEmpty(&readyQueue)){ return NULL; }
	
#ifdef DEBUG
	printf("readyQueue length: %d\n", readyQueue.len);
#endif
	tmp = readyQueue.prev;
	readyQueue.prev = tmp->next;
	if(readyQueue.prev != NULL) readyQueue.prev->prev = NULL;
	tmp->next = tmp->prev = NULL;
	
	readyQueue.len--;
#ifdef DEBUG
	printf("**Scheduler selects process %d. \n", tmp->id);
	printf("readyQueue length: %d\n", readyQueue.len);
#endif
	return tmp;
}

//Shrotest Job First(Modified) - targetServiceTime
struct process* SJFschedule() {
	if(IsEmpty(&readyQueue)){ return NULL; }
	#ifdef DEBUG
	printf("readyQueue length: %d\n", readyQueue.len);
	#endif
	struct process* minProc;
	int min = INT_MAX;
	
	struct process* tmp = readyQueue.prev;
	while(tmp != NULL){
		if(tmp->targetServiceTime < min){
			minProc = tmp;
			min = tmp->targetServiceTime;
		}
		tmp = tmp->next;
	}
	deQueue(&readyQueue, minProc);
	
	//readyQueue.len--; - deQueue에서 수행
#ifdef DEBUG
	printf("**Scheduler selects process %d. \n", minProc->id);
	printf("readyQueue length: %d\n", readyQueue.len);
#endif
	return minProc;
}

//Shortes Remaining Time Next (SRTN) (targetServiceTime – serviceTime)
struct process* SRTNschedule() {
	if(IsEmpty(&readyQueue)){ return NULL; }
	//printf("readyQueue length: %d\n", readyQueue.len);
	
	struct process* minRemainProc;
	int min = INT_MAX;
	
	struct process* tmp = readyQueue.prev;
	while(tmp != NULL){
		int remainTime = tmp->targetServiceTime - tmp->serviceTime;
		if(remainTime < min){
			minRemainProc = tmp;
			min = remainTime;
		}
		tmp = tmp->next;
	}
	deQueue(&readyQueue, minRemainProc);
#ifdef DEBUG
	printf("**Scheduler selects process %d with remainTime : %d. \n", minRemainProc->id, min);
#endif
	return minRemainProc;
}

// Guaranteed Scheduling (Modified)
struct process* GSschedule() {
	if(IsEmpty(&readyQueue)){ return NULL; }
	//printf("readyQueue length: %d\n", readyQueue.len);
	
	struct process* tmp = readyQueue.prev;
	struct process* minRemainProc;
	float minRatio = INT_MAX;//FLOAT_MAX?

	while(tmp != NULL){
		float remainRatio = (float)(tmp->serviceTime)/tmp->targetServiceTime;
		//printf("**----------------- %d : %f, remain: %f\n", tmp->id, minRatio, remainRatio);
		if(remainRatio < minRatio){
			minRemainProc = tmp;
			minRatio = remainRatio;
		}
		tmp = tmp->next;
	}
	deQueue(&readyQueue, minRemainProc);
#ifdef DEBUG
	printf("**Scheduler selects process %d with remainTime : %f. \n", minRemainProc->id, minRatio);
#endif
	return minRemainProc;
}

// Simple Feedback Scheduling
struct process* SFSschedule() {
	if(IsEmpty(&readyQueue)){ return NULL; }
	//printf("readyQueue length: %d\n", readyQueue.len);
	
	struct process* tmp = readyQueue.prev;
	struct process* minProc = tmp;
	int highPriority = tmp->priority;

	while(tmp != NULL){
		int tmpPriority = tmp->priority;
		//printf("**----------------- %d %d, highest: %d\n", tmp->id, tmpPriority, highPriority);
		if(tmpPriority > highPriority){
			minProc = tmp;
			highPriority = tmpPriority;
		}
		tmp = tmp->next;
	}
	deQueue(&readyQueue, minProc);
#ifdef DEBUG
	printf("**Scheduler selects process %d with priority : %d. \n", minProc->id, highPriority);
#endif
	return minProc;
}

void printResult() {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	long totalProcIntArrTime=0,totalProcServTime=0,totalIOReqIntArrTime=0,totalIOServTime=0;
	long totalWallTime=0, totalRegValue=0;
	for(i=0; i < NPROC; i++) {
		totalWallTime += procTable[i].endTime - procTable[i].startTime;
		/*printf("%d : startTime %d endTime %d %d\n",i, procTable[i].startTime,procTable[i].endTime);
		
		printf("proc %d serviceTime %d targetServiceTime %d saveReg0 %d\n",
			i,procTable[i].serviceTime,procTable[i].targetServiceTime, procTable[i].saveReg0);*/
		
		totalRegValue += procTable[i].saveReg0+procTable[i].saveReg1;
		//printf("reg0 %d reg1 %d totalRegValue %d\n",procTable[i].saveReg0,procTable[i].saveReg1,totalRegValue);
	}
	for(i = 0; i < NPROC; i++ ) { 
		totalProcIntArrTime += procIntArrTime[i];
		totalProcServTime += procServTime[i];
	}
	for(i = 0; i < NIOREQ; i++ ) { 
		totalIOReqIntArrTime += ioReqIntArrTime[i];
		totalIOServTime += ioServTime[i];
	}
	
	printf("Avg Proc Inter Arrival Time : %g \tAverage Proc Service Time : %g\n", (float) totalProcIntArrTime/NPROC, (float) totalProcServTime/NPROC);
	printf("Avg IOReq Inter Arrival Time : %g \tAverage IOReq Service Time : %g\n", (float) totalIOReqIntArrTime/NIOREQ, (float) totalIOServTime/NIOREQ);
	printf("%d Process processed with %d IO requests\n", NPROC,NIOREQ);
	printf("Average Wall Clock Service Time : %g \tAverage Two Register Sum Value %g\n", (float) totalWallTime/NPROC, (float) totalRegValue/NPROC);
	
}

int main(int argc, char *argv[]) {
	// DO NOT CHANGE THIS FUNCTION
	int i;
	int totalProcServTime = 0, ioReqAvgIntArrTime;
	int SCHEDULING_METHOD, MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME, MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME;
	
	if (argc < 12) {
		printf("%s: SCHEDULING_METHOD NPROC NIOREQ QUANTUM MIN_INT_ARRTIME MAX_INT_ARRTIME MIN_SERVTIME MAX_SERVTIME MIN_IO_SERVTIME MAX_IO_SERVTIME MIN_IOREQ_INT_ARRTIME\n",argv[0]);
		exit(1);
	}
	
	SCHEDULING_METHOD = atoi(argv[1]);
	NPROC = atoi(argv[2]);
	NIOREQ = atoi(argv[3]);
	QUANTUM = atoi(argv[4]);
	MIN_INT_ARRTIME = atoi(argv[5]);
	MAX_INT_ARRTIME = atoi(argv[6]);
	MIN_SERVTIME = atoi(argv[7]);
	MAX_SERVTIME = atoi(argv[8]);
	MIN_IO_SERVTIME = atoi(argv[9]);
	MAX_IO_SERVTIME = atoi(argv[10]);
	MIN_IOREQ_INT_ARRTIME = atoi(argv[11]);
	
	printf("SIMULATION PARAMETERS : SCHEDULING_METHOD %d NPROC %d NIOREQ %d QUANTUM %d \n", SCHEDULING_METHOD, NPROC, NIOREQ, QUANTUM);
	printf("MIN_INT_ARRTIME %d MAX_INT_ARRTIME %d MIN_SERVTIME %d MAX_SERVTIME %d\n", MIN_INT_ARRTIME, MAX_INT_ARRTIME, MIN_SERVTIME, MAX_SERVTIME);
	printf("MIN_IO_SERVTIME %d MAX_IO_SERVTIME %d MIN_IOREQ_INT_ARRTIME %d\n", MIN_IO_SERVTIME, MAX_IO_SERVTIME, MIN_IOREQ_INT_ARRTIME);
	
	//srandom(SEED);
	srand(SEED);
	// allocate array structures
	procTable = (struct process *) malloc(sizeof(struct process) * NPROC);
	ioDoneEvent = (struct ioDoneEvent *) malloc(sizeof(struct ioDoneEvent) * NIOREQ);
	procIntArrTime = (int *) malloc(sizeof(int) * NPROC);
	procServTime = (int *) malloc(sizeof(int) * NPROC);
	ioReqIntArrTime = (int *) malloc(sizeof(int) * NIOREQ);
	ioServTime = (int *) malloc(sizeof(int) * NIOREQ);

	// initialize queues
	readyQueue.next = readyQueue.prev = &readyQueue;
	
	blockedQueue.next = blockedQueue.prev = &blockedQueue;
	ioDoneEventQueue.next = ioDoneEventQueue.prev = NULL; //&ioDoneEventQueue;
	ioDoneEventQueue.doneTime = INT_MAX;
	ioDoneEventQueue.procid = -1;
	ioDoneEventQueue.len  = readyQueue.len = blockedQueue.len = 0;
	
	// generate process interarrival times
	for(i = 0; i < NPROC; i++ ) { 
		procIntArrTime[i] = random()%(MAX_INT_ARRTIME - MIN_INT_ARRTIME+1) + MIN_INT_ARRTIME;
	}
	
	// assign service time for each process
	for(i=0; i < NPROC; i++) {
		procServTime[i] = random()% (MAX_SERVTIME - MIN_SERVTIME + 1) + MIN_SERVTIME;
		totalProcServTime += procServTime[i];	
	}
	
	ioReqAvgIntArrTime = totalProcServTime/(NIOREQ+1);
	assert(ioReqAvgIntArrTime > MIN_IOREQ_INT_ARRTIME);
	
	// generate io request interarrival time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioReqIntArrTime[i] = random()%((ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+1) + MIN_IOREQ_INT_ARRTIME;
	}
	
	// generate io request service time
	for(i = 0; i < NIOREQ; i++ ) { 
		ioServTime[i] = random()%(MAX_IO_SERVTIME - MIN_IO_SERVTIME+1) + MIN_IO_SERVTIME;
	}
	
#ifdef DEBUG
	// printing process interarrival time and service time
	printf("Process Interarrival Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procIntArrTime[i]);
	}
	printf("\n");
	printf("Process Target Service Time :\n");
	for(i = 0; i < NPROC; i++ ) { 
		printf("%d ",procServTime[i]); //procTable[i].targetServiceTime);
	}
	printf("\n");
#endif
	
	// printing io request interarrival time and io request service time
	printf("IO Req Average InterArrival Time %d\n", ioReqAvgIntArrTime);
	printf("IO Req InterArrival Time range : %d ~ %d\n",MIN_IOREQ_INT_ARRTIME,
			(ioReqAvgIntArrTime - MIN_IOREQ_INT_ARRTIME)*2+ MIN_IOREQ_INT_ARRTIME);
			
#ifdef DEBUG		
	printf("IO Req Interarrival Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioReqIntArrTime[i]);
	}
	printf("\n");
	printf("IO Req Service Time :\n");
	for(i = 0; i < NIOREQ; i++ ) { 
		printf("%d ",ioServTime[i]);
	}
	printf("\n");
#endif
	
	struct process* (*schFunc)();
	switch(SCHEDULING_METHOD) {
		case 1 : schFunc = RRschedule; break;
		case 2 : schFunc = SJFschedule; break;
		case 3 : schFunc = SRTNschedule; break;
		case 4 : schFunc = GSschedule; break;
		case 5 : schFunc = SFSschedule; break;
		default : printf("ERROR : Unknown Scheduling Method\n"); exit(1);
	}
	initProcTable();
	procExecSim(schFunc);
	printResult();
	
}
