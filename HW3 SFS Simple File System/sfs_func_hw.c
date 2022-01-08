//
// Simple FIle System
// Student Name : B836042
// Student Number : 류지호
// 2021, Operating System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//gcc sfs_disk.c sfs_func_hw.c sfs_main.c sfs_func_ext.o
/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // current working directory


void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	case -11:
		printf("%s: input file size exceeds the max file size\n",message, path); return;
	case -12:
		printf("%s: can't open %s input file\n",message, path); return;
			
	default:
		printf("unknown error code\n");
		return;
	}
}

void sfs_mount(const char* path)
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount() {

	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}
//[Step 2]
void sfs_touch(const char* path)
{
	// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int emptyDirectIndex = -1, emptyBlockIndex = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0)
		{ emptyDirectIndex = i; emptyBlockIndex = 0; break; }
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0)
				{ emptyDirectIndex = i; emptyBlockIndex = j;  break; }
			if(!strcmp(path, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				error_message("touch", path, -6); //Already exists
				return;
			}
		}
		
		if(emptyBlockIndex != -1) break; //빈공간 찾았다면
		
	}//for: direct

	if(emptyBlockIndex == -1){ //다 돌아도 빈 공간이 없다면
		error_message("touch", path, -3); //Directory full
		return;
	}
	/*int emptyIndirectIndex = -1;
	//[2] inode 블록의 SFS_INDIRECT 체크 (파일만 들어갈수있다.)
	if(i == SFS_NDIRECT){ //INDIRECT 
		struct sfs_dir dir_block[SFS_DENTRYPERBLOCK];
		if(si.sfi_indirect != 0){
			disk_read(sd, si.sfi_indirect); 
			//--------------------------------------------------
			for(i=0; i < SFS_DENTRYPERBLOCK; i++) {
				if (sd[i] == 0)
				{ emptyIndirectIndex = i; emptyBlockIndex = 0; break; }

				disk_read(dir_block, sd[i]); 
				for(j=0; j<SFS_DENTRYPERBLOCK; j++) {
					if (dir_block[i] == 0)
					{ emptyIndirectIndex = i; emptyBlockIndex = j; break; }

					disk_read(&inode_file,dir_block[j]);
					if(!strcmp(path, dir_block[j].sfd_name)){
						error_message(path, path, -6); //Already exists
						return;
					}
					if(emptyIndirectIndex != -1) break;
				}
			}
			//--------------------------------------------------
		} else { //새로 블록 할당해야
			emptyIndirectIndex = emptyBlockIndex = 0; 

			struct sfs_dir indirect_dir_block[SFS_DENTRYPERBLOCK];
			//블록 할당
			int new_block_ino = getEmptyBitMapNum();
			if(new_block_ino == -1){
				error_message("mkdir", org_path, -4); //No block available
				return;
			}
			bzero(&sd,SFS_BLOCKSIZE);
			si.sfi_direct[emptyDirectIndex] = new_block_ino;
			disk_write( sd, si.sfi_direct[emptyDirectIndex] );
		}

		if(emptyBlockIndex != -1){ //다 돌아도 빈 공간이 없다면
			error_message(path, path, -3); //Directory full
			return;
		}
	}*/
	


	//[3] 빈 디스크 블록 번호 가져오기 (비트맵 체크)
	int newbie_ino = getEmptyBitMapNum();
	if(newbie_ino == -1){
		error_message("touch", path, -4); //No block available
		return;
	}

	//-=--------------------------------------------
	//allocate new block
	//int newbie_ino = 6;

	//이미 위 for문에서 break 당시의 emptyBlockIndex
	//printf("- %d\n", emptyDirectIndex);
	//printf("- %d\n", emptyBlockIndex);
	//printf("-newbie_ino: %d\n", newbie_ino);


	//[4] dir 전체 블록 가져와서 새로운 inode(file)의 빈 디스크 블록 번호 할당
	// 다시 write
	if(emptyBlockIndex == 0) { // 빈 블록이면 블록 할당해줘야 함.
		int new_block_ino = getEmptyBitMapNum();
		if(new_block_ino == -1){
			error_message("touch", path, -4); //No block available
			return;
		}
		bzero(&sd,SFS_BLOCKSIZE);
		si.sfi_direct[emptyDirectIndex] = new_block_ino;
		
		//[indirect]추가
		//disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	} else { // 빈 블록이 아니면
		disk_read(sd, si.sfi_direct[emptyDirectIndex]);
	}

	sd[emptyBlockIndex].sfd_ino = newbie_ino;
	strncpy( sd[emptyBlockIndex].sfd_name, path, SFS_NAMELEN );

	disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );


	struct sfs_inode newbie;
	bzero(&newbie,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = 0;
	newbie.sfi_type = SFS_TYPE_FILE;

	disk_write( &newbie, newbie_ino );

}

// [비트맵 빈 공간 번호 가져오기]
int getEmptyBitMapNum(){
	int i, j;
	unsigned char bitmap[512];

	//한 비트맵블럭당 : 512*8 blocks 표현 가능 -=> 총 3개 블록?
	//[0] 번째 비트맵
	disk_read(&bitmap , SFS_MAP_LOCATION); 
	for(i=0; i< SFS_BLOCKSIZE; i++ ){
		//printf("bitmap[i] : %x\n", bitmap[i]);
		for(j=0; j<8; j++){
			//printf("binary : %x\n", bitmap[i] & (1<<j)) ;
			if( bitmap[i] & (1<<j) ) continue;
			bitmap[i] = bitmap[i] | (1<<j); //1: 사용중 표시
			disk_write(&bitmap , SFS_MAP_LOCATION); 
			return i*8 + j; //newbie_ino 
		}
	}
	//printf("----------2-------------------------\n");
	//[0] 번째 비트맵
	unsigned char bitmap2[512];
	disk_read(&bitmap2 , SFS_MAP_LOCATION+1); 
	for(i=0; i< SFS_BLOCKSIZE; i++ ){
		//printf("bitmap[%d] : %x\n", i, bitmap2[i]);
		for(j=0; j<8; j++){
			//printf("binary : %x\n", bitmap2[i] & (1<<j)) ;
			if( bitmap2[i] & (1<<j) ) continue;
			bitmap2[i] = bitmap2[i] | (1<<j); //1: 사용중 표시
			disk_write(&bitmap2 , SFS_MAP_LOCATION+1); 
			//printf("INDEX INO: %d\n", SFS_BLOCKSIZE + (i*8 + j));
			return SFS_BLOCKSIZE*8 + (i*8 + j); //newbie_ino 
		}
	}

	//printf("-----------------------3-----------\n");
	//[0] 번째 비트맵
	unsigned char bitmap3[512];
	disk_read(&bitmap3 , SFS_MAP_LOCATION+2); 
	for(i=0; i< SFS_BLOCKSIZE; i++ ){
		//printf("bitmap[i] : %x\n", bitmap[i]);
		for(j=0; j<8; j++){
			//printf("binary : %x\n", bitmap[i] & (1<<j)) ;
			if( bitmap3[i] & (1<<j) ) continue;
			bitmap3[i] = bitmap3[i] | (1<<j); //1: 사용중 표시
			disk_write(&bitmap3 , SFS_MAP_LOCATION+2); 
			return SFS_BLOCKSIZE*2*8 + (i*8 + j); //newbie_ino 
		}
	}

	return -1;
}

void updateBitMap(int num){
	int i, j;
	unsigned char bitmap[512];
	int location = num/512;
	num = num%512;
	switch(location){
		case 0:
			//[1]
			disk_read(&bitmap , SFS_MAP_LOCATION); 
			i = num/8;
			j = num%8;
			//printf("bitmap[ num/8] : %x\n", bitmap[i]);
			//9번 삭제 시 1번 방으로 간다. [1]: 0000111 -> LSB: 1110 MSB: 0000
			bitmap[i] = bitmap[i] ^ (1<<j); //0: empty 로 free
			disk_write(&bitmap , SFS_MAP_LOCATION); 

			break;
		case 1:
			//[2]
			disk_read(&bitmap , SFS_MAP_LOCATION); 
			i = num/8;
			j = num%8;
			//printf("bitmap[ num/8] : %x\n", bitmap[i]);
			//9번 삭제 시 1번 방으로 간다. [1]: 0000111 -> LSB: 1110 MSB: 0000
			bitmap[i] = bitmap[i] ^ (1<<j); //0: empty 로 free
			disk_write(&bitmap , SFS_MAP_LOCATION); 
			break;
		case 2: 
			//[3]
			disk_read(&bitmap , SFS_MAP_LOCATION); 
			i = num/8;
			j = num%8;
			//printf("bitmap[ num/8] : %x\n", bitmap[i]);
			//9번 삭제 시 1번 방으로 간다. [1]: 0000111 -> LSB: 1110 MSB: 0000
			bitmap[i] = bitmap[i] ^ (1<<j); //0: empty 로 free
			disk_write(&bitmap , SFS_MAP_LOCATION); 
			break;
	}
	
}

//[Step 1]
void sfs_cd(const char* path)
{
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK]; //출력 dir
	struct sfs_inode inode; // 할당받는 inode
	int i, j;

	int isDirExist = 0, isInputFileName = 0;
	if(path == NULL){ //root로 이동
		sd_cwd.sfd_ino = 1;		//init at root
		sd_cwd.sfd_name[0] = '/';
		sd_cwd.sfd_name[1] = '\0';
		return;
	}


	disk_read( &inode, sd_cwd.sfd_ino);

	if (inode.sfi_type == SFS_TYPE_DIR) {
		//inode의 direct에 할당된 inode 번호들 확인
		//printf(" %d ", inode.sfi_direct[i]);
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]); //할당된 data block읽기

			if(path != NULL) { //PATH 입력
				for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
					if(dir_entry[j].sfd_ino != 0 && !strcmp(path, dir_entry[j].sfd_name)){
						disk_read(&inode,dir_entry[j].sfd_ino);
						if (inode.sfi_type == SFS_TYPE_FILE) {
							isInputFileName = 1;
							break;
						}
						isDirExist = 1;	
						sd_cwd.sfd_ino = dir_entry[j].sfd_ino;
						strcpy(sd_cwd.sfd_name, dir_entry[j].sfd_name);
						break;
					}
				}
				if(isInputFileName) {
					error_message("cd", path, -2); //Not a directory
					break;
				} 
				if(!isDirExist) {
					error_message("cd", path, -1); //No such file or directory
					break;
				} 
			} //if: path exists

		}//for: cwd-direct 
	} //if: type -readable

}

//[Step 1]
void sfs_ls(const char* path)
{
	//현재 inode 번호 : 1 (root)
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK]; //출력 dir
	struct sfs_inode inode_dir, inode_file; // 할당받는 inode
	int i, j;
	int isDirExist = 0;

	//cwd의 inode 블럭가져오기
	disk_read( &inode_dir, sd_cwd.sfd_ino);

	//inode의 direct에 할당된 inode 번호들 
	//printf(" %d ", inode.sfi_direct[i]);
	for(i=0; i < SFS_NDIRECT; i++) {
		if (inode_dir.sfi_direct[i] == 0) continue;
		disk_read(dir_entry, inode_dir.sfi_direct[i]); //할당된 data block읽기

		if(path != NULL) { //PATH 입력
			for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
				if(dir_entry[j].sfd_ino != 0 && !strcmp(path, dir_entry[j].sfd_name)){
					disk_read(&inode_file,dir_entry[j].sfd_ino);

					if (inode_file.sfi_type == SFS_TYPE_FILE) {
						printf("%s\n", dir_entry[j].sfd_name); 
						return;
					}
					else{ //디렉토리라면
						isDirExist = 1;
						i =0; path = NULL;
						inode_dir = inode_file;
						disk_read(dir_entry, inode_file.sfi_direct[i]);
					}
				}
				if(isDirExist) break;
			}

			if(!isDirExist) {
				error_message("ls", path, -1); //No such file or directory
				return;
			} 
		} //if: path exists
		//else {
			//데이터 블럭 내용 inode로 읽기 (블럭 내부의 entry개수만큼.)
			for(j=0; j < SFS_DENTRYPERBLOCK;j++) {
				if(dir_entry[j].sfd_ino == 0) continue;
				
				disk_read(&inode_file,dir_entry[j].sfd_ino);
				if (inode_file.sfi_type == SFS_TYPE_FILE) {
					printf("%s\t", dir_entry[j].sfd_name); 
				} else {
					printf("%s/\t", dir_entry[j].sfd_name); 
				}
				
			} //데이터블럭 리스트 읽기
		//} //else : cwd read

	}//for: direct[i]
	printf("\n");
}

//[Step 2]
void sfs_mkdir(const char* org_path) 
{
// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int emptyDirectIndex = -1, emptyBlockIndex = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0)
		{ emptyDirectIndex = i; emptyBlockIndex = 0; break; }
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0)
				{ emptyDirectIndex = i; emptyBlockIndex = j;  break; }
			if(!strcmp(org_path, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				disk_read(&inode_file,sd[j].sfd_ino);
				error_message("mkdir", org_path, -6); //Already exists
				return;
			}
		}

		if(emptyBlockIndex != -1) break; //빈공간 찾았다면
		if(i+1 == SFS_NDIRECT){ //다 돌아도 빈 공간이 없다면
			error_message("mkdir", org_path, -3); //Directory full
			return;
		}
	}//for: direct

	//[X] 새로운 DIRECT 노드 블록 할당
	if(emptyBlockIndex == 0) { // 빈 블록이면 블록 할당해줘야 함.
		int new_block_ino = getEmptyBitMapNum();
		if(new_block_ino == -1){
			error_message("mkdir", org_path, -4); //No block available
			return;
		}
		bzero(&sd,SFS_BLOCKSIZE);
		si.sfi_direct[emptyDirectIndex] = new_block_ino;
		//disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	} else { // 빈 블록이 아니면
		disk_read(sd, si.sfi_direct[emptyDirectIndex]);
	}

	//[3] 빈 디스크 블록 번호 가져오기 (비트맵 체크)
	int newbie_ino = getEmptyBitMapNum();
	if(newbie_ino == -1){
		error_message("mkdir", org_path, -4); //No block available
		return;
	}

	//-=--------------------------------------------
	//allocate new block

	//[4] dir 전체 블록 가져와서 새로운 inode(file)의 빈 디스크 블록 번호 할당 다시 write
	

	sd[emptyBlockIndex].sfd_ino = newbie_ino;
	strncpy( sd[emptyBlockIndex].sfd_name, org_path, SFS_NAMELEN );

	disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );

	//[5] 새로 생성하는 dir inode + 블록 만들기
	int new_dir_block_num = getEmptyBitMapNum();
	if(new_dir_block_num == -1){
		error_message("mkdir", org_path, -4); //No block available
		return;
	}

	struct sfs_inode newbie;
	bzero(&newbie,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = sizeof(struct sfs_dir)*2;
	newbie.sfi_type = SFS_TYPE_DIR;
	newbie.sfi_direct[0] = new_dir_block_num;

	disk_write( &newbie, newbie_ino );

	struct sfs_dir new_dir_block[SFS_DENTRYPERBLOCK];
	bzero(&new_dir_block,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	new_dir_block[0].sfd_ino = newbie_ino;
	new_dir_block[1].sfd_ino = sd_cwd.sfd_ino;
	strncpy( new_dir_block[0].sfd_name, ".", SFS_NAMELEN );
	strncpy( new_dir_block[1].sfd_name, "..", SFS_NAMELEN );
	/*for(i=2; i< SFS_DENTRYPERBLOCK; i++){
		new_dir_block[i].sfd_ino = SFS_NOINO;
		strncpy( new_dir_block[i].sfd_name, "f", SFS_NAMELEN );
	}*/
	disk_write( &new_dir_block, new_dir_block_num );

}
//[Step 2]
void sfs_rmdir(const char* org_path) 
{	
	if(!strcmp(org_path, ".")){
		error_message("rmdir", org_path, -8); //Invalid argument
		return;
	}

// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int rmInodeNum = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) continue;
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0) continue;
			if(!strcmp(org_path, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				disk_read(&inode_file,sd[j].sfd_ino);
				if(inode_file.sfi_type == SFS_TYPE_FILE){
					error_message("rmdir", org_path, -2); //Not a directory
					return;
				}
				if(inode_file.sfi_size > 128) {
					error_message("rmdir", org_path, -7); //Directory not empty
					return;
				}
				//여기서는 무사통과 디렉 찾음!
				rmInodeNum = sd[j].sfd_ino;
				sd[j].sfd_ino = SFS_NOINO;
				disk_write( sd, si.sfi_direct[i] );
			}
			if(rmInodeNum != -1) break;
		}
		if(rmInodeNum != -1) break; //빈공간 찾았다면

	}//for: direct

	if(rmInodeNum == -1){ //다 돌아도 빈 공간이 없다면
		error_message("rmdir", org_path, -1); //No such file or directory
		return;
	}

	//rmInodeNum Free시켜줘야 한다. (inode + directory block)
	//bitmap도 초기화
	struct sfs_dir rmDirBlock[SFS_DENTRYPERBLOCK];
	struct sfs_inode rmInodeBlock;

	bzero(&rmDirBlock,SFS_BLOCKSIZE);
	bzero(&rmInodeBlock, SFS_BLOCKSIZE);
	disk_write(&rmInodeBlock, rmInodeNum);

	for(i=0; i<SFS_NDIRECT; i++){
		if(!inode_file.sfi_direct[i]) continue;
		disk_write(rmDirBlock, inode_file.sfi_direct[i]);
		updateBitMap(inode_file.sfi_direct[i]);
	}

	updateBitMap(rmInodeNum);

	si.sfi_size -= sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );

}
//[Step 2]
void sfs_mv(const char* src_name, const char* dst_name) 
{
// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int mvInodeNum = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) continue;
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0) continue;
			if(!strcmp(dst_name, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				error_message("mv", dst_name, -6); //Already exists
				return;
			}
			if(!strcmp(src_name, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				disk_read(&inode_file,sd[j].sfd_ino);
				//파일 및 디렉 찾음!
				mvInodeNum = sd[j].sfd_ino;
				strncpy( sd[j].sfd_name, dst_name, SFS_NAMELEN );
			}
		}
		if(mvInodeNum != -1){
			disk_write( sd, si.sfi_direct[i] );
			break; //빈공간 찾았다면
		} 
	}//for: direct

	if(mvInodeNum == -1){ //다 돌아도 빈 공간이 없다면
		error_message("mv", src_name, -1); //No such file or directory
		return;
	}
}
//[Step 2]
void sfs_rm(const char* path) 
{
// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int rmInodeNum = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) continue;
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0) continue;
			if(!strcmp(path, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				disk_read(&inode_file,sd[j].sfd_ino);
				if(inode_file.sfi_type == SFS_TYPE_DIR){
					error_message("rm", path, -9); //Is a directory
					return;
				}
				//파일 찾음!
				rmInodeNum = sd[j].sfd_ino;
				sd[j].sfd_ino = SFS_NOINO;
				disk_write( sd, si.sfi_direct[i] );
			}
			if(rmInodeNum != -1) break;
		}
		if(rmInodeNum != -1) break; //빈공간 찾았다면

	}//for: direct

	if(rmInodeNum == -1){ //다 돌아도 빈 공간이 없다면
		error_message("rm", path, -1); //No such file or directory
		return;
	}

	//rmInodeNum Free시켜줘야 한다. (inode + directory block)
	//bitmap도 초기화
	struct sfs_dir rmDirBlock[SFS_DENTRYPERBLOCK];
	struct sfs_inode rmInodeBlock;
	bzero(&rmDirBlock,SFS_BLOCKSIZE);
	bzero(&rmInodeBlock, SFS_BLOCKSIZE);
	disk_write(&rmInodeBlock, rmInodeNum);

	for(i=0; i<SFS_NDIRECT; i++){
		if(inode_file.sfi_direct[i] == 0) continue;
		disk_write(rmDirBlock, inode_file.sfi_direct[i]);
		updateBitMap(inode_file.sfi_direct[i]);
	}

	//INDIRECT까지 삭제해야 함.
	if(inode_file.sfi_indirect != 0){
		struct sfs_inode inode_indirect;
		disk_read(&inode_indirect, inode_file.sfi_indirect);

		for(i=0; i<SFS_DENTRYPERBLOCK; i++){
			if(!inode_indirect.sfi_direct[i]) continue;
			disk_write(rmDirBlock, inode_indirect.sfi_direct[i]);
			updateBitMap(inode_file.sfi_direct[i]);
		}
	}

	updateBitMap(rmInodeNum);

	si.sfi_size -= sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );
}

/**/
void sfs_cpin( const char* path, const char* local_path) 
{
	int in_fd = open(local_path, O_RDONLY);
	if(in_fd<0){
		error_message("cpin", local_path, -12); //No such file or directory
		return;
	}
	
	//int out_fd = creat(path, OUTPUT_MODE);
	//if(out_fd < 0) exit(3);

	// 최대 사이즈: 7680byte + (512/4)*512byte (64kbyte)
	int MAX_BUF_SIZE = (120*64) + (512/4)*512;

	FILE *fp = fopen(local_path, "r");
	fseek(fp, 0, SEEK_END);    // 파일 포인터를 파일의 끝으로 이동시킴
	int size = ftell(fp);          // 파일 포인터의 현재 위치를 얻음
	fclose(fp);
	if(size > MAX_BUF_SIZE){
		error_message("cpin", "", -11);
		return;
	}
			

	//[1] local_file 읽을 수 있는가
	//[2] 현재 디스크에 공간이 있는가
	//[3] 둘다 가능하면 새로운 파일 생성 (touch)
	//[4] local_path 읽으면서 동시에 현재 파일에 write ?? localpath는 open, path는 disk_write

//--------------------------------------------------------------[1][2] 새로운 파일 만들기-
	// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

//	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int emptyDirectIndex = -1, emptyBlockIndex = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0)
		{ emptyDirectIndex = i; emptyBlockIndex = 0; break; }
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0)
				{ emptyDirectIndex = i; emptyBlockIndex = j;  break; }
			if(!strcmp(path, sd[j].sfd_name)){ //중복 파일, 중복 디렉토리 체크
				error_message("cpin", path, -6); //Already exists
				return;
			}
		}
		
		if(emptyBlockIndex != -1) break; //빈공간 찾았다면
		
	}//for: direct

	if(emptyBlockIndex == -1){ //다 돌아도 빈 공간이 없다면
		error_message("cpin", path, -3); //Directory full
		return;
	}

	//printf("================\n");
	//-=--------------------------------------------
	//allocate new block

	//[4] dir 전체 블록 가져와서 새로운 inode(file)의 빈 디스크 블록 번호 할당
	// 다시 write
	if(emptyBlockIndex == 0) { // 빈 블록이면 블록 할당해줘야 함.
		int new_block_ino = getEmptyBitMapNum();
		if(new_block_ino == -1){
			error_message("cpin", path, -4); //No block available
			return;
		}
		bzero(&sd,SFS_BLOCKSIZE);
		si.sfi_direct[emptyDirectIndex] = new_block_ino;
		
		//[indirect]추가
		//disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	} else { // 빈 블록이 아니면
		disk_read(sd, si.sfi_direct[emptyDirectIndex]);
	}

	//[3] 빈 디스크 블록 번호 가져오기 (비트맵 체크)
	int newbie_ino = getEmptyBitMapNum();
	if(newbie_ino == -1){
		error_message("cpin", path, -4); //No block available
		return;
	}

	sd[emptyBlockIndex].sfd_ino = newbie_ino;
	strncpy( sd[emptyBlockIndex].sfd_name, path, SFS_NAMELEN );

	disk_write( sd, si.sfi_direct[emptyDirectIndex] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );


	struct sfs_inode newbie;
	bzero(&newbie,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = 0;
	newbie.sfi_type = SFS_TYPE_FILE;

	disk_write( &newbie, newbie_ino );
	
	//-=--------------------------------------------여기까지가 새로운 파일 만들기!
	int indir_cnt = 0;
	int BUF_SIZE = SFS_BLOCKSIZE;
	char buffer[BUF_SIZE]; //BUF_SIZE
    memset(buffer, 0, sizeof(buffer));

	//[4] local_path 읽으면서 동시에 현재 파일에 write ?? localpath는 open, path는 disk_write
	int rd_count;
	int addIndirect = 0;
	int new_indir_db[SFS_BLOCKSIZE/4]; 
	struct sfs_inode inode_new_file;
	disk_read( &inode_new_file, newbie_ino );
	while(1){
		rd_count = read(in_fd, buffer, BUF_SIZE);
		if(rd_count <= 0) {
			break;
		}
		//wt_count = disk_write(out_fd, buffer, rd_count);
		//--------------DISK에 파일 복사하기
		if(addIndirect == 0){
			for(i=0; i < SFS_NDIRECT; i++) {
				if (inode_new_file.sfi_direct[i] != 0) continue;

				//direct공간에 새로 데이터블록 만들어야 함
				int new_block_ino = getEmptyBitMapNum();
				if(new_block_ino == -1){
					error_message("cpin", path, -4); //No block available
					return;
				}
				inode_new_file.sfi_direct[i] = new_block_ino;

				/*for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
					//printf(" buffer[j].sfd_ino : %d\n", buffer[j*sizeof(struct sfs_dir)]);
					if(buffer[j] == 0) break;
					inode_new_file.sfi_size += sizeof(struct sfs_dir);
				}*/
				inode_new_file.sfi_size += SFS_BLOCKSIZE;
				disk_write( buffer, inode_new_file.sfi_direct[i] );
				disk_write( &inode_new_file, newbie_ino );
				
				if(i+1 == SFS_NDIRECT) addIndirect = 1;
				break;	
			} //direct
		}
		//[indirect]추가 (max size: (8)*512byte (64kbyte))
		else if(addIndirect){
			if (inode_new_file.sfi_indirect == 0){
				//새로 데이터블록 만들어야 함
				int new_block_ino = getEmptyBitMapNum();
				if(new_block_ino == -1){
					error_message("cpin", path, -4); //No block available
					return;
				}
				bzero(&new_indir_db,SFS_BLOCKSIZE);
				inode_new_file.sfi_indirect = new_block_ino;
				disk_write( &new_indir_db, new_block_ino );
				disk_write( &inode_new_file, newbie_ino );
			}
			else {
				disk_read( new_indir_db, inode_new_file.sfi_indirect );
			}

			//indirect 안의 direct 블록들 읽기
			for(i=0; i < SFS_BLOCKSIZE/4; i++) {
				if (new_indir_db[i] != 0) continue;
				//direct공간에 새로 데이터블록 만들어야 함
				int new_block_ino = getEmptyBitMapNum();
				if(new_block_ino == -1){
					size = SFS_BLOCKSIZE*SFS_NDIRECT + indir_cnt*SFS_BLOCKSIZE;
					inode_new_file.sfi_size = size;
					disk_write( &inode_new_file, newbie_ino );
					error_message("cpin", path, -4); //No block available
					return;
				}

				indir_cnt++;
				new_indir_db[i] = new_block_ino;
				disk_write( &new_indir_db, inode_new_file.sfi_indirect );
				
				/*for(j=0; j < SFS_BLOCKSIZE; j++) {
					//printf(" buffer[j].sfd_ino : %d\n", buffer[j*sizeof(struct sfs_dir)]);
					if(buffer[j] == 0) continue;
					inode_new_file.sfi_size += 1;
				}*/
				
				disk_write( buffer, new_indir_db[i] );
				disk_write( &inode_new_file, newbie_ino );
				break;
			} //for 8 direct 
		} //addIndirect
		
		inode_new_file.sfi_size = size;
		disk_write( &inode_new_file, newbie_ino );
		//--------------DISK에 파일 복사하기
	} //file read
	close(in_fd);
}

void sfs_cpout(const char* path, const char* local_path) 
{
	int out_fd = open(local_path, O_RDONLY);
	if(out_fd >= 0 ){
		error_message("cpout", local_path, -6); //Already exists
		return;
	}
	out_fd = creat(local_path, 0666);

	//[1] local_file 읽을 수 있는가
	//[2] 현재 디스크에 공간이 있는가
	//[4] local_path 읽으면서 동시에 현재 파일에 write ?? localpath는 open, path는 disk_write

//--------------------------------------------------------------[1][2] 현재 디스크 공간에 파일 존재여부확인
	// 현재 디렉토리의 inode
	struct sfs_inode si, inode_file;
	disk_read( &si, sd_cwd.sfd_ino );

//	assert( si.sfi_type == SFS_TYPE_DIR );

	//buffer for disk read (현재 디렉토리 블록) 사이즈 256/4btye = 8개?
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	int i, j;
	int file_ino = -1;

	//[1] inode 블록의 빈 공간 찾기 
	//[1] inode 블록의 SFS_NDIRECT 체크 
	for(i=0; i < SFS_NDIRECT; i++) {
		if (si.sfi_direct[i] == 0) break; 
		disk_read(sd, si.sfi_direct[i]);

		//printf("[dirblock] si.sfi_direct[i]: %d\n",si.sfi_direct[i]);
		
		for(j=0; j < SFS_DENTRYPERBLOCK; j++) {
			//printf("[dirblock] inode: %d, %d\n",j, sd[j].sfd_ino);
			if(sd[j].sfd_ino == 0) break; 
			if(!strcmp(path, sd[j].sfd_name)){ 
				//파일 존재 
				file_ino = sd[j].sfd_ino;
				break;
			}
			if(file_ino != -1) break; 
		}
		if(file_ino != -1) break; 
	}//for: direct

	if(file_ino == -1){ //다 돌아도 빈 공간이 없다면
		error_message("cpout", path, -1); //No such file or directory
		return;
	}
	
	//-=--------------------------------------------외부로 out하기
	int BUF_SIZE = SFS_BLOCKSIZE;
	char buffer[BUF_SIZE]; //BUF_SIZE
    memset(buffer, 0, sizeof(buffer));

	//[4]
	int wt_count;
	int addIndirect = 0;
	int new_indir_db[SFS_BLOCKSIZE/4]; 

	struct sfs_inode inode_new_file;
	int file_size = inode_new_file.sfi_size;
	disk_read( &inode_new_file, file_ino );


	
	//-------------- 파일 복사하기
	for(i=0; i < SFS_NDIRECT; i++) {
		if (inode_new_file.sfi_direct[i] == 0) break;

		disk_read( buffer, inode_new_file.sfi_direct[i] );
		wt_count = write(out_fd, buffer, BUF_SIZE);
		if(wt_count <= 0) {
			break;
		}
	} //direct

	//[indirect]추가 (max size: (8)*512byte (64kbyte))
	if (inode_new_file.sfi_indirect != 0){
		disk_read( new_indir_db, inode_new_file.sfi_indirect );
		
		//indirect 안의 direct 블록들
		for(i=0; i < SFS_BLOCKSIZE/4; i++) {
			if (new_indir_db[i] == 0) break;			
			
			disk_read( buffer, new_indir_db[i] );
			/*if(new_indir_db[i+1] == 0){
				FILE *fp = fopen(local_path, "r");
				fseek(fp, 0, SEEK_END);    // 파일 포인터를 파일의 끝으로 이동시킴
				int size = ftell(fp);          // 파일 포인터의 현재 위치를 얻음
				fclose(fp);
	
				BUF_SIZE = file_size - size;
				printf("--%d\n", BUF_SIZE);
			}*/
			wt_count = write(out_fd, buffer, BUF_SIZE);
			if(wt_count <= 0) {
				break;
			}
		} //for 8 direct 
	}

	//--------------DISK에 파일 복사하기
	close(out_fd);
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
