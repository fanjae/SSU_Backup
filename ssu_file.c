#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ssu_file.h"

int compare_file(char *file, char *file2) // 파일 비교 함수
{
	struct stat file_info, file_info2; 

	if((access(file, F_OK) < 0) || (access(file2, F_OK) < 0)) { // 파일 존재 여부 확인.
		if(access(file, F_OK) < 0) { // 첫번째 파일이 없는 경우
			fprintf(stderr, "Error : File it's not exist : %s\n",file);
			return -1;
		}
		if(access(file2, F_OK) < 0) { // 두번째 파일이 없는 경우
			fprintf(stderr, "Error : File it's not exist : %s\n", file2);
			return -1;
		}
	}
	
	if(lstat(file, &file_info) < 0) { // 심볼릭 파일 처리
		fprintf(stderr, "Error : lstat error\n");
		return -1;
	}
	if(lstat(file2, &file_info2) < 0) { // 심볼릭 파일 처리
		fprintf(stderr, "Error : lstat error\n");
		return -1;
	}
	
	if(!S_ISREG(file_info.st_mode)) { // 일반 파일이 아닌 경우 오류
		fprintf(stderr, "Error : %s is not Regular file\n", file);
		return -1;
	}
	if(!S_ISREG(file_info2.st_mode)) { // 일반 파일이 아닌 경우 오류
		fprintf(stderr, "Error : %s is not Regular file\n", file2);
		return -1;
	}

	if((file_info.st_mtime == file_info2.st_mtime) && (file_info.st_size == file_info2.st_size)) { // mtime이 같고 파일이 크기가 같은 경우
		printf("%s, %s is same file\n", file,file2);
		return 1;
	}
	else if((file_info.st_mtime != file_info2.st_mtime) || (file_info.st_size != file_info2.st_size)) { // mtime이나 파일 크기가 다른 경우 (다른 파일임)
		printf("%s, %s is not same file\n", file, file2);
		printf("%s : mtime is %lu  %s : file : %lubytes.\n",file, file_info.st_mtime, file, file_info.st_size);
		printf("%s : mtime is %lu  %s : file : %lubytes.\n",file2, file_info2.st_mtime, file2, file_info2.st_size);
		return 0;
	}
	return -1;
}
