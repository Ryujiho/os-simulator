//
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2021
// Student Name: 류지호
// Student Number: B836042
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;					// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;	// The 2nd level page created(allocated);
	int numIHTConflictAccess; 	// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;		// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;		// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	//struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
} *procTable;

struct invertedEntry {
	int procid;
	int vpn;
	int frameNum;
	struct invertedEntry *prev;
	struct invertedEntry *next;
	struct invertedEntry *LRUprev;
	struct invertedEntry *LRUnext;
} *invertedTableList, currentHashList;

struct pageEntry {
	int vpn;
	int fIndex, sIndex;
	int cnt;
	int procid;
	int pageFrameNum;
	struct pageEntry *secondLevelPageTable;
	struct pageEntry *prev;
	struct pageEntry *next;
} currentPageList;

struct pageEntry **pageTable; //pageTable 2차원 Arrayx, 1차원은 proc Num(int), 2차원 (struct pageentry)

int numProcess, nFrame;

int FIRSTLevelBits, SECONDLevelBits;

void oneLevelVMSim(int replaceType) {
	int i, j;
	unsigned addr; //16진수 int
	char rw;
	int ch;
	int cnt_eof = 0;
	unsigned vpn, pIndex; //가상주소 index, page index
	unsigned PA; //물리주소
	int nFrameList[nFrame];

	for(j=0; j<nFrame; j++) nFrameList[j] = 0;

	int eof[numProcess];
	for(j=0; j<numProcess; j++) eof[j] = 0;


	//프로세스 0은 트레이스 0 첫번째 트레이스 부터~n프록까지	
	while(cnt_eof == 0/*cnt_eof < numProcess*/ ){
		for(j=0; j<numProcess; j++){
			//if(!eof[j]){
				ch = fscanf(procTable[j].tracefp,"%x %c",&addr,&rw);
				// fscanf는 끝에 도달하면 EOF를 반환한다.
				if(ch == EOF){
					eof[j] = 1; cnt_eof++;
					break;
				}
				procTable[j].ntraces++;
				//printf("process %d file %s - %dth trace: %x\n",j, procTable[j].traceName, procTable[j].ntraces, addr);
				
				//1. 12bit와 20bit로 나눈다.
				vpn = addr>>PAGESIZEBITS;
				pIndex = addr<<(VIRTUALADDRBITS-PAGESIZEBITS)>>(VIRTUALADDRBITS-PAGESIZEBITS);
				
				//printf("%x %x\n",vpn, pIndex);
				
				// 20bit(VPN)로  pagetable내 공간 확인
				// HIT이면
				if( pageTable[j][vpn].pageFrameNum != -1){ 
					//printf("----PAGE HIT-\n");
					procTable[j].numPageHit++;
					PA = pageTable[j][vpn].pageFrameNum<<PAGESIZEBITS + pIndex; //FRAME NUMBER + PAGE INDEX값 더하기? -> PA

					if(replaceType == 1){ //LRU
						//LRUFrameList 현재 frame맨 앞으로 
						struct pageEntry *tmp = currentPageList.prev;
						int hitFrameNum = pageTable[j][vpn].pageFrameNum;
						if(hitFrameNum == tmp->pageFrameNum) break; //이미 맨 앞이면 실행 x

						//printf("MoveToHead, frameNum :%d %d\n",  hitFrameNum, tmp->pageFrameNum);
							
							//printf("currentPageList(HIT_BEF) : %d\n",tmp);
							//printf("currentPageList : %d\n",tmp->next);
							//printf("currentPageList : %d\n",tmp->next->next);

						//현재 접근하려는 프레임 번호를 찾는다.
						while(tmp->pageFrameNum != hitFrameNum){
							//printf("--- : %d\n",tmp->pageFrameNum);
							tmp = tmp->next;
						} 
						//currentPageList안의 위치 파악이 되면 맨앞으로 옮긴다.
						//printf("MoveToHead, frameNum : %d\n",  tmp->pageFrameNum);

						//--------------------------------------------------------------
						if(tmp->next != NULL){
							tmp->next->prev = tmp->prev;
						} else {
							currentPageList.next = tmp->prev; //tmp가 맨 끝이니 tmp 전을 맨 끝으 설정
							currentPageList.next->next = NULL; 
							//currentPageList.next->prev = tmp;
						}
						if(tmp->prev != NULL){
							tmp->prev->next = tmp->next; // 옮기는 노드의 전과 후 노드끼리 연결
						}
						tmp->prev = NULL; //옮기려는 노드를 맨앞으로 옮긴다.
						tmp->next = currentPageList.prev;
						currentPageList.prev = tmp; //연결리스트 맨 앞을 설정
						currentPageList.prev->next->prev = tmp;
							/*printf("currentPageList(HIT) : %d\n",currentPageList.prev);
							printf("currentPageList : %d\n",currentPageList.next);

							printf("currentPageList(HIT_AFT) : %d\n",currentPageList.prev);
							printf("currentPageList : %d\n",currentPageList.prev->next);
							printf("currentPageList : %d\n",currentPageList.prev->next->next);*/
						
					}
					
				} else {
					//printf("---PAGE FAULT--\n");
					procTable[j].numPageFault++;
					
					//1. PyMem에 메모리 할당 (FRAME번호 빈곳부터! )
					for(i=0; i<nFrame; i++){
						if(nFrameList[i]) continue;
						nFrameList[i] = 1; 
						pageTable[j][vpn].procid = j;
						pageTable[j][vpn].vpn = vpn;
						pageTable[j][vpn].pageFrameNum = i;

						if(replaceType == 0){ //FIFO (뒤에 연결.)
							if(currentPageList.prev == NULL){
								currentPageList.cnt = 0;
								currentPageList.prev = currentPageList.next = &pageTable[j][vpn];
							} else {
								currentPageList.cnt++;
								//enQueue(list)
								pageTable[j][vpn].prev = currentPageList.next;

								currentPageList.next->next = &pageTable[j][vpn];
								currentPageList.next = &pageTable[j][vpn];
							}
						}
						if(replaceType == 1){ //LRU
							if(currentPageList.prev == NULL){ //이건 위로 빼기
								currentPageList.cnt = 0;
								currentPageList.prev = currentPageList.next = &pageTable[j][vpn];
							} else { //맨 앞에 추가
								currentPageList.cnt++;

								pageTable[j][vpn].prev = NULL;
								pageTable[j][vpn].next = currentPageList.prev;

								currentPageList.prev->prev = &pageTable[j][vpn];
								currentPageList.prev = &pageTable[j][vpn];
							}
							//printf("currentPageList(fault1) : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
						}
						break;
					} 

					//2. 메모리가 다 차셔 빈 공간 없을 시
					//dequeue (List)
					if(i == nFrame){//currentPageList->cnt 비교하기!! 맨 앞으로
						//printf("---MEMORY FULL--\n");

						//----------------FIFO 방식 -------------------
						if(replaceType == 0){
							int replacedVPN = currentPageList.prev->vpn;
							int replacedProcId = currentPageList.prev->procid;
							//printf("currentPageList : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);

							//printf("Replaced index : %x, %d\n",replacedVPN, currentPageList.prev->pageFrameNum);
							//2. 새로 추가할 frame에 내쫓을 곳의 frame번호 할당
							pageTable[j][vpn].pageFrameNum = currentPageList.prev->pageFrameNum;
							pageTable[j][vpn].vpn = vpn;
							pageTable[j][vpn].procid = j;

							//3. 가장 앞 리스트 노드 삭제
							currentPageList.prev = currentPageList.prev->next; //리스트에서 삭제노드 삭제
							if( pageTable[replacedProcId][replacedVPN].next != NULL) {
								pageTable[replacedProcId][replacedVPN].next->prev = NULL; //삭제다음노드의 삭제노드 연결 삭제
								pageTable[replacedProcId][replacedVPN].next = NULL;
							}
							pageTable[replacedProcId][replacedVPN].prev =  NULL;

							//1. 내쫓길 vpn에 -1로 초기화 
							pageTable[replacedProcId][replacedVPN].pageFrameNum = -1;

							//4. 새로 추가되는 PAGE FRAME LIST에 추가~! ENEQUEUE 함수화
							if(currentPageList.prev == NULL){
								currentPageList.cnt = 0;
								currentPageList.prev = currentPageList.next = &pageTable[j][vpn];
							} else {
								//enQueue(list)
								pageTable[j][vpn].prev = currentPageList.next;

								currentPageList.next->next = &pageTable[j][vpn];
								currentPageList.next = &pageTable[j][vpn];
							}
						} //FIFO
						//----------------LRU 방식 -------------------
						else if(replaceType == 1){ //맨뒤 삭제
							int replacedVPN = currentPageList.next->vpn;
							int replacedProcId = currentPageList.next->procid;
							//printf("currentPageList : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
							//printf("Replaced index : %x, %d\n",replacedVPN, currentPageList.next->pageFrameNum);
							//2. 내쫓을 곳의  frame번호 할당
							pageTable[j][vpn].pageFrameNum = currentPageList.next->pageFrameNum;
							pageTable[j][vpn].vpn = vpn;
							pageTable[j][vpn].procid = j;

							//3. 가장 뒤 노드 삭제
							currentPageList.next = currentPageList.next->prev; //리스트에서 삭제노드 삭제
							
							//printf("2222currentPageList : %d\n",currentPageList.next);
							//내쫓길 노드pageTable 초기화
							if( pageTable[replacedProcId][replacedVPN].prev != NULL) {
								pageTable[replacedProcId][replacedVPN].prev->next = NULL; //삭제다음노드의 삭제노드 연결 삭제
								pageTable[replacedProcId][replacedVPN].prev = NULL;
							}
							pageTable[replacedProcId][replacedVPN].next =  NULL;

							//1. 내쫓길 vpn에 -1로 초기화 
							pageTable[replacedProcId][replacedVPN].pageFrameNum = -1;

							//4. 새로 추가되는 PAGE FRAME LIST에 추가~! ENEQUEUE 함수화
							if(currentPageList.next == NULL){
								currentPageList.cnt = 0;
								currentPageList.prev = currentPageList.next = &pageTable[j][vpn];
							} else {//위 fault-nFrame 추가와 코드 동일?
								pageTable[j][vpn].prev = NULL;
								pageTable[j][vpn].next = currentPageList.prev;

								currentPageList.prev->prev = &pageTable[j][vpn];
								currentPageList.prev = &pageTable[j][vpn];
							}
							//printf("currentPageList(after replace fault) : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
						} //LRU
						
					} //메모리 다 찬 경우
				}//FAGE FAULT

			//} //TRACE가 남은경우
		}//PROCESS FOR문
	}

	
	//시뮬 종료 후 출력
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
	
}

void twoLevelVMSim(/*...*/) {
	int i, j, k;
	int rw, ch;
	int cnt_eof = 0;
	unsigned addr, firstIndex, secondIndex, pageIndex;
	int nFrameList[nFrame];

	SECONDLevelBits = VIRTUALADDRBITS - PAGESIZEBITS - FIRSTLevelBits;
	for(j=0; j<nFrame; j++) nFrameList[j] = 0;
	
	while(cnt_eof == 0){
		for(j=0; j<numProcess; j++){
				ch = fscanf(procTable[j].tracefp,"%x %c",&addr, &rw);
				if(ch == EOF){
					cnt_eof = 1;
					break;
				}

				procTable[j].ntraces++;
				//printf("process %d file %s - %dth trace: %x\n",j, procTable[j].traceName, procTable[j].ntraces, addr);
				
				//1. first bit와 second bit + page bit(12bit)로 나눈다.
				firstIndex = addr>>(PAGESIZEBITS+SECONDLevelBits);
				secondIndex = addr<<FIRSTLevelBits>>(FIRSTLevelBits+PAGESIZEBITS);
				pageIndex = addr<<(FIRSTLevelBits+SECONDLevelBits)>>(FIRSTLevelBits+SECONDLevelBits);
				
				//printf("%x %x %x\n",firstIndex, secondIndex, pageIndex);
				
				//2. First Entry가 비어 있고, pageFrameNum이 할당되어 있다면 HIT
				if((pageTable[j][firstIndex].secondLevelPageTable != NULL) 
					&& (pageTable[j][firstIndex].secondLevelPageTable[secondIndex].pageFrameNum != -1)){
						procTable[j].numPageHit++;
						//printf("HIT : proc %d\n", j);

						//-----------------------LRU 맨 앞으로 옮기기(moveToHead)
						struct pageEntry *tmp = currentPageList.prev;
						int hitFrameNum = pageTable[j][firstIndex].secondLevelPageTable[secondIndex].pageFrameNum;
						if(hitFrameNum == tmp->pageFrameNum) break; //이미 맨 앞이면 실행 x

						//printf("MoveToHead, frameNum :%d %d\n",  hitFrameNum, tmp->pageFrameNum);
							

						//현재 접근하려는 프레임 번호를 찾는다.
						while(tmp->pageFrameNum != hitFrameNum){
							//printf("--- : %d\n",tmp->pageFrameNum);
							tmp = tmp->next;
						} 
						//currentPageList안의 위치 파악이 되면 맨앞으로 옮긴다.
						//printf("MoveToHead, frameNum : %d\n",  tmp->pageFrameNum);

						//--------------------------------------------------------------
						if(tmp->next != NULL){
							tmp->next->prev = tmp->prev;
						} else {
							currentPageList.next = tmp->prev; //tmp가 맨 끝이니 tmp 전을 맨 끝으 설정
							currentPageList.next->next = NULL; 
							//currentPageList.next->prev = tmp;
						}
						if(tmp->prev != NULL){
							tmp->prev->next = tmp->next; // 옮기는 노드의 전과 후 노드끼리 연결
						}
						tmp->prev = NULL; //옮기려는 노드를 맨앞으로 옮긴다.
						tmp->next = currentPageList.prev;
						currentPageList.prev = tmp; //연결리스트 맨 앞을 설정
						currentPageList.prev->next->prev = tmp;
							//printf("currentPageList(HIT) : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
						//-----------------------LRU 맨 앞으로 옮기기
				}
				else {
					procTable[j].numPageFault++;
					//printf("FAULT : proc %d\n", j);
							//printf("currentPageList(FA) : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
					//테이블 없다면 -> SECOND TABLE 생성 (2^secondLevelBits크기)
					if(pageTable[j][firstIndex].secondLevelPageTable == NULL){
						pageTable[j][firstIndex].secondLevelPageTable  = (struct pageEntry *) malloc(sizeof(struct pageEntry)*(1L<<SECONDLevelBits));
						procTable[j].num2ndLevelPageTable++;
						for( k=0 ; k<(1L<<SECONDLevelBits); k++ ){
							pageTable[j][firstIndex].secondLevelPageTable[k].pageFrameNum = -1;
						}
					}

					//테이블 있거나 없어도 동일! 테이블 안에 값이 없을 때의 로직
					// 2ndIndex 값에 Frame할당!
					for(i = 0; i< nFrame; i++){
						if(nFrameList[i]) continue;
						pageTable[j][firstIndex].secondLevelPageTable[secondIndex].procid = j;
						pageTable[j][firstIndex].secondLevelPageTable[secondIndex].pageFrameNum = i;
						pageTable[j][firstIndex].secondLevelPageTable[secondIndex].fIndex = firstIndex;//LRU 사용
						pageTable[j][firstIndex].secondLevelPageTable[secondIndex].sIndex = secondIndex;//LRU 사용
						nFrameList[i] = 1;

						//LRU 맨 앞에 추가!------------------------------
						if(currentPageList.prev == NULL){ //이건 위로 빼기
							currentPageList.cnt = 0;
							currentPageList.prev = currentPageList.next = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
						} else {
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].prev = NULL;
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].next = currentPageList.prev;
							if(currentPageList.prev != NULL)
								currentPageList.prev->prev = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
							currentPageList.prev = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
						}
						//------------------------------
						break;
					}
					//Frame이 꽉찼다면
					if(i == nFrame){ //맨 뒤 삭제
							//연결리스트에는 secondLevelPageTable의 Entry가 들어 있다.
							int replacedFIndex = currentPageList.next->fIndex;
							int replacedSIndex = currentPageList.next->sIndex;
							int replacedProcId = currentPageList.next->procid;
							//printf("currentPageList : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
							//printf("Replaced index : %d, %x, %x, %d\n",currentPageList.next->pageFrameNum, replacedSIndex, replacedProcId);
							
							//2. 새로운 노드에 내쫓을 곳의  frame번호 할당
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].procid  = j;
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].pageFrameNum = currentPageList.next->pageFrameNum;
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].fIndex = firstIndex;//LRU 사용
							pageTable[j][firstIndex].secondLevelPageTable[secondIndex].sIndex = secondIndex;//LRU 사용
							
							//3. 가장 뒤 노드 삭제
							currentPageList.next = currentPageList.next->prev; //리스트에서 삭제노드 삭제
							
							//내쫓길 노드pageTable 초기화
							if( pageTable[replacedProcId][replacedFIndex].secondLevelPageTable[replacedSIndex].prev != NULL) {
								pageTable[replacedProcId][replacedFIndex].secondLevelPageTable[replacedSIndex].prev->next = NULL; //삭제다음노드의 삭제노드 연결 삭제
								pageTable[replacedProcId][replacedFIndex].secondLevelPageTable[replacedSIndex].prev = NULL;
							}
							pageTable[replacedProcId][replacedFIndex].secondLevelPageTable[replacedSIndex].next =  NULL;

							//1. 내쫓길 vpn에 -1로 초기화 
							pageTable[replacedProcId][replacedFIndex].secondLevelPageTable[replacedSIndex].pageFrameNum = -1;

							//4. 새로 추가되는 PAGE FRAME LIST에 추가~! ENEQUEUE 함수화
							if(currentPageList.next == NULL){
								currentPageList.cnt = 0;
								currentPageList.prev = currentPageList.next = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
							} else {//위 fault-nFrame 추가와 코드 동일?
								pageTable[j][firstIndex].secondLevelPageTable[secondIndex].prev = NULL;
								pageTable[j][firstIndex].secondLevelPageTable[secondIndex].next = currentPageList.prev;

								currentPageList.prev->prev = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
								currentPageList.prev = &pageTable[j][firstIndex].secondLevelPageTable[secondIndex];
							}
							//printf("currentPageList(AFTER FA) : %d\n",currentPageList.prev);
							//printf("currentPageList : %d\n",currentPageList.next);
					} //메모리 찬 경우 LRU
				} //page fault (2가지 경우)

		} //TRACE가 남은경우
	}//PROCESS FOR문


	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim(/*...*/) {
	int j, i;
	int rw, ch;
	int cnt_eof = 0;
	unsigned addr, vpn, pIndex;
	int nFrameList[nFrame];

	SECONDLevelBits = VIRTUALADDRBITS - PAGESIZEBITS - FIRSTLevelBits;
	for(j=0; j<nFrame; j++) nFrameList[j] = 0;
	
	while(cnt_eof == 0){
		for(j=0; j<numProcess; j++){
				struct invertedEntry *hitEntry = NULL;
				ch = fscanf(procTable[j].tracefp,"%x %c",&addr, &rw);
				if(ch == EOF){
					cnt_eof = 1;
					break;
				}
				procTable[j].ntraces++;
				
				//1. first bit와 second bit + page bit(12bit)로 나눈다.
				vpn = addr>>PAGESIZEBITS;
				pIndex = addr<<(VIRTUALADDRBITS-PAGESIZEBITS)>>(VIRTUALADDRBITS-PAGESIZEBITS);

				int hashTableIndex = (vpn+j)%nFrame;
				//printf("process %d - %dth VPN: %x, hashTableIndex: %d : ",j, procTable[j].ntraces, vpn, hashTableIndex);

				/*struct invertedEntry* temp =invertedTableList[hashTableIndex].prev;
					while(temp != NULL){
						//printf("Val: %x\n", temp->vpn);
						temp = temp->next;
					}*/
				if(invertedTableList[hashTableIndex].prev != NULL){
					procTable[j].numIHTNonNULLAcess++;
					hitEntry = invertedTableList[hashTableIndex].prev;

					while (hitEntry != NULL)
					{
						procTable[j].numIHTConflictAccess++; /*ENTRY 있을때마다 접근*/
						if(hitEntry->procid == j && hitEntry->vpn == vpn){
							procTable[j].numPageHit++;
							//printf("HIT\n");
							
							//printf("before movetohead : %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->vpn);
							//printf("before movetohead : %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->vpn);

							//printf("movetohead : %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->LRUprev);
							//printf("movetohead : %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->LRUnext);
							//printf("movetohead : %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->LRUprev);
							//printf("movetohead : %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->LRUnext);

							//-------------------------------------LRU MOVETOHEAD
							struct invertedEntry *findLRUEntry = currentHashList.LRUprev;
							
							if(hitEntry->frameNum == findLRUEntry->frameNum) break; /*이미 맨 앞이면 실행 x*/
							while(findLRUEntry != NULL)
							{
								if(findLRUEntry->frameNum == hitEntry->frameNum){
									/*맨앞인 경우는 위에서 검사함 (prev,next NULL이거나 prev가 null)*/
									if(findLRUEntry->LRUprev != NULL && findLRUEntry->LRUnext != NULL){ /*NEXT PREV 다 있음*/
										findLRUEntry->LRUprev->LRUnext = findLRUEntry->LRUnext;
										findLRUEntry->LRUnext->LRUprev = findLRUEntry->LRUprev;
									} else {
										 currentHashList.LRUnext = findLRUEntry->LRUprev;
										 currentHashList.LRUnext->LRUnext = NULL;
									}
									findLRUEntry->LRUprev = NULL;
									findLRUEntry->LRUnext = currentHashList.LRUprev;
									currentHashList.LRUprev->LRUprev = findLRUEntry;
									currentHashList.LRUprev = findLRUEntry;						
									break;
								}
								findLRUEntry = findLRUEntry->LRUnext;
							}
							//-------------------------------------LRU MOVETOHEAD
							//printf("movetohead : %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->vpn);
							//printf("movetohead : %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->vpn);
							break;
						}
						hitEntry = hitEntry->next;
					}
				} // IHTNonNULLAcess

					if(hitEntry != NULL){ continue; }
//printf("FAULT\n");
					procTable[j].numPageFault++;
					if(invertedTableList[hashTableIndex].prev == NULL){
						procTable[j].numIHTNULLAccess++;
					}

					//------------------------------------FRAME 공간 찾기
					 //1. 가장 작은 수의 frame num부터 할당 2. 꽉찬 경우 LRU로 맨 뒤 부터 삭제
					int newFrameNum;
					for(i=0; i<nFrame; i++){
						if(nFrameList[i]) continue;
						nFrameList[i] = 1; 
						newFrameNum = i;
						break;
					}
					if(i==nFrame){ /*Frame이 꽉 찬 경우 LRU의 맨 뒤의 FRAMENUM 할당*/
						//1. LRU 맨 뒤 번호 가져오기
						int lruFrameNum = currentHashList.LRUnext->frameNum;
						newFrameNum = lruFrameNum;
						//2. FRAMENUM에 해당하는 해쉬 삭제
						//삭제할 hashNumb 가져오기
						//모든 entry조사 -> 그 안에 당연히 값 있다.
						int deletedVPN = currentHashList.LRUnext->vpn;
						int deletedProcID = currentHashList.LRUnext->procid;
						int deletedHashIndex = (deletedVPN + deletedProcID) % nFrame;

						//printf("MEMORY FULL : DELETE : %x in Index : %d\n", deletedVPN, deletedHashIndex);
						struct invertedEntry * deletedEntry = invertedTableList[deletedHashIndex].prev;
						while(deletedEntry != NULL){
							if(deletedEntry->vpn == deletedVPN && deletedEntry->procid == deletedProcID){
								//삭제할 노드 위치 찾았다.이제 삭제해야 함. 
								// PREV: NULL , NEXT: NULL
								//printf("DELETE : %x in Index : %d\n", deletedEntry->vpn);
								//printf("DELETE : %d\n", deletedEntry->prev);
								//printf("DELETE : %d\n", deletedEntry->next);
								/*
								if(deletedEntry->prev != NULL)
								printf("DELETE : %x\n", deletedEntry->prev->vpn);
								if(deletedEntry->next != NULL)
								printf("DELETE : %x\n", deletedEntry->next->vpn);*/
								
								if(deletedEntry->prev == NULL && deletedEntry->next == NULL ){
									invertedTableList[deletedHashIndex].prev = invertedTableList[deletedHashIndex].next = NULL;
								}
								// PREV: 값 , NEXT: 값
								else if(deletedEntry->prev != NULL && deletedEntry->next != NULL){
									deletedEntry->next->prev = deletedEntry->prev;
									deletedEntry->prev->next = deletedEntry->next;
								}	
								// PREV: 값 , NEXT: NULL
								else if(deletedEntry->prev != NULL){
									deletedEntry->prev->next = NULL;
									invertedTableList[deletedHashIndex].next = deletedEntry->prev;
								}
								// PREV: NULL , NEXT: 값
								else if(deletedEntry->next != NULL){
									deletedEntry->next->prev = NULL;
									invertedTableList[deletedHashIndex].prev = deletedEntry->next;
									deletedEntry->next = NULL;
								}
						deletedEntry->prev = deletedEntry->next = NULL;							
								break;
							}
							deletedEntry = deletedEntry->next;
						}
				
						//3. LRU 연결 삭제
						struct invertedEntry* deletedNode = currentHashList.LRUnext;
						currentHashList.LRUnext = currentHashList.LRUnext->LRUprev;
						if(currentHashList.LRUnext != NULL)
							currentHashList.LRUnext->LRUnext = NULL;
						deletedNode->LRUprev = deletedNode->LRUnext = NULL;

						//printf("LRU DEL %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->vpn);
						//printf("LRU DEL %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->vpn);

						//printf("%d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->vpn);
						//printf("%d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->vpn);
					}


					//------------------------------------새로운 노드 할당
					struct invertedEntry *newNode = (struct invertedEntry *) malloc(sizeof(struct invertedEntry));//삭제 시 free!
					newNode->procid = j;
					newNode->vpn = vpn;
					newNode->prev = NULL;
					newNode->LRUnext = newNode->LRUprev = NULL;
					//HASH REPLACE 시 기존 ENTRY 삭제 된 후 연결 지어준다.
					if(invertedTableList[hashTableIndex].prev == NULL){ /*해시테이블 값이 null인 경우 */
						newNode->next = NULL;
						invertedTableList[hashTableIndex].prev = invertedTableList[hashTableIndex].next = newNode;
					} 
					else { /*해시테이블 값이 있지만 전체 연결리스트에 값이 없을 경우 */
						newNode->next = invertedTableList[hashTableIndex].prev;
						invertedTableList[hashTableIndex].prev->prev = newNode;
						invertedTableList[hashTableIndex].prev = newNode;
					}
					newNode->frameNum = newFrameNum;

				//printf("jiho %x \n",invertedTableList[hashTableIndex].prev->vpn);
				//printf("jiho %x \n",invertedTableList[hashTableIndex].next->vpn);
					//------------------------------------LRU insertToHead
					if(currentHashList.LRUnext == NULL){
						currentHashList.LRUprev = currentHashList.LRUnext = newNode;
					} else {
						newNode->LRUprev = NULL;
						newNode->LRUnext = currentHashList.LRUprev;

						currentHashList.LRUprev->LRUprev = newNode;
						currentHashList.LRUprev = newNode;
					}

					
					//printf("HEAD: %d : %x\n", currentHashList.LRUprev, currentHashList.LRUprev->vpn);
					//printf("HEAD: %d : %x\n", currentHashList.LRUnext, currentHashList.LRUnext->vpn);
					//------------------------------------LRU insertToHead

		} //process For loop
	}


	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i, j;
	int simType, phyMemSizeBits;
	int pageSize;

	simType = atoi(argv[1]);
	FIRSTLevelBits = atoi(argv[2]);
	phyMemSizeBits = atoi(argv[3]);

	numProcess = argc-4;
	procTable = (struct procEntry*) malloc(sizeof(struct procEntry) * numProcess);
	pageTable = (struct pageEntry**) malloc(sizeof(struct pageEntry*) * numProcess);

	if (argc < 5) {
		printf("simType firstLevelBits PhysicalMemorySizeBits TraceFileNames......\n");
		exit(1);
	}

	// initialize procTable for Memory Simulations
	for(i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n",i, argv[i + 4]); // i+optind+3
		procTable[i].tracefp = fopen(argv[i + 4],"r");
		if (procTable[i].tracefp == NULL) {
			printf("ERROR: can't open %s file; exiting...",argv[i+4]);
			exit(1);
		}
		procTable[i].pid = i;
		procTable[i].traceName = argv[i + 4];
		procTable[i].ntraces = 0;
		procTable[i].numPageHit = procTable[i].numPageFault = 0;

		//process별로 페이지 테이블(2^20 크기) 만들어야 함! 2차원 배열 동적할당
		if (simType == 2) {
			pageSize = 1L<<FIRSTLevelBits;
		} 
		else {
			pageSize = 1L<<(VIRTUALADDRBITS-PAGESIZEBITS);
		}
		pageTable[i] = (struct pageEntry *) malloc(sizeof(struct pageEntry) * (pageSize));
		for(j = 0; j < pageSize; j++) {
			pageTable[i][j].pageFrameNum = -1;
			pageTable[i][j].prev = pageTable[i][j].next = NULL;
			pageTable[i][j].secondLevelPageTable = NULL;
		}
	}

	nFrame  = (1L<<phyMemSizeBits) / (1L<<PAGESIZEBITS);
	printf("Num of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	if (simType == 0) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType); //FIFO
	}
	if (simType == 1) {
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		oneLevelVMSim(simType); //LRU
	}
	
	if (simType == 2) {
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		twoLevelVMSim();
	}
	
	if (simType == 3) {
		invertedTableList = (struct invertedEntry*) malloc(sizeof(struct invertedEntry) * nFrame);
		for(j = 0; j < nFrame; j++) {
			invertedTableList[j].prev = invertedTableList[j].next = NULL;
			invertedTableList[j].LRUprev = invertedTableList[j].LRUnext = NULL;
		}
		
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		invertedPageVMSim();
	}

	return(0);
}
