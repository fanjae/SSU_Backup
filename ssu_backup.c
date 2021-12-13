#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>
#include "ssu_file.h"

char *path_process;
char *pathname;
char *Log_path = "log.txt";
char directory_temp[1000][300]={0};
char d_temp[1000][300]={0};

int d_count = 0;
int d_count2 = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t p_thread[1024]={0};
typedef struct b_list
{ 
	char *path; // backup file path.
	int period; // period
	int	m_on;
	int n_on;
	int t_on;
	int delete_count;
	int delete_time;
	pthread_t pid; // thread_id
	time_t mtime;
	struct b_list *next; // next_node.
	
}backup_list;

backup_list *head; // linked list head;
backup_list *cur; // linked list cur;
backup_list *remove_cur; // linked list remove_cur;
backup_list *remove_next_cur;
backup_list *NODE; // linked list NODE;
backup_list *null_pointer = NULL;

void *add(void *arg); // 백업을 수행하는 쓰레드
void append(char *des, char c); // 문자열 끝에 문자를 추가하는 함수이다.
void list(void); // list를 출력하는 함수
void listdir(const char *name); // 디렉토리를 탐색하는 함수이다.
int integer_check(char *str, int len); // 정수 여부 확인.
int main(int argc, char *argv[])
{
	struct stat file_info;
	pthread_t tid;

	char input[1024];
	char *prompt = "20132334>";
	char *test_directory = "temp";
	char *option_list[4] = {"add","remove","compare","recover"};
	int thread_num = 0;
	if(argc > 2) { // ssu_backup 인자가 2개이상인 경우
		fprintf(stderr, "Error : Usage a File : %s\n", argv[0]);
		exit(1);
	}
	else if(argc == 1) { // 인자가 없는 경우(ssu_backup만 들어온 상태)
		if(access(test_directory, F_OK) < 0)
			mkdir(test_directory, 0755);

		pathname = (char *) malloc(1024);
		if(realpath(test_directory,pathname) == NULL) { // 절대 위치 경로 read.
			fprintf(stderr, "path error\n");
			exit(1);
		}
		
		path_process = (char *) malloc(1024);
		if(realpath(".",path_process) == NULL) { // 절대 위치 경로 read.
			fprintf(stderr, "path error\n");
			exit(1);
		}

	}
	else if(argc == 2) { // 인자가 1개 존재하는 경우
		if(access(argv[1], F_OK) < 0) { // 디렉토리를 찾을 수 있는가?
			fprintf(stderr, "Error : Usage a File : %s\n", argv[1]);
			exit(1);
		}
		else if(lstat(argv[1], &file_info) < 0) { // 심볼릭 파일 처리
			fprintf(stderr, "Error : lstat error\n");
			exit(1);
		}
		else if(!S_ISDIR(file_info.st_mode)) { // 디렉토리 파일인가?
			fprintf(stderr, "Error : Usage a File : %s\n", argv[1]);
			exit(1);
		}
		else if((access(argv[1], R_OK) < 0) || (access(argv[1], X_OK) < 0)) { // 읽기와 실행이 가능한가?
			fprintf(stderr, "Error : Usage a File : %s\n", argv[1]);
			exit(1);
		} 

		pathname = (char *) malloc(1024);
		if(realpath(argv[1],pathname) == NULL) { // 절대 경로 위치 read.
			fprintf(stderr, "path error\n");
			exit(1);
		}

		path_process = (char *) malloc(1024);
		if(realpath(".",path_process) == NULL) { // 절대 위치 경로 read.
			fprintf(stderr, "path error\n");
			exit(1);
		}
			
	}

	while(1) {
		char prompt_option[20][256]={0};

		int prompt_option_len[8];
		int option_state[5] = {0};
		int flag = 0;
		int num,length,count;
		num = length = count = 0;
		fputs(prompt,stdout);
		
		fgets(input,sizeof(input),stdin);
	
		if(strcmp(input, "\n") == 0) // '엔터'만 입력 받은 경우
			continue;
		
		if(strcmp(input, "exit\n") == 0) // exit를 입력 받은 경우
			exit(1);
		
		for(int i = 0; i < strlen(input); i++) { // 엔터나 띄어쓰기를 구분한다.
			if(input[i] == ' ' || input[i] == '\n') 
				count++;
			if(input[i] == '\n')
				break;	
		}
		if(count > 20) {
			fprintf(stderr, "Error : Input limit Over()\n");
			continue;
		}
		num = length = 0;
		for(int i = 0; i < strlen(input); i++) { // 엔터나 띄어쓰기 이전까지의 문자를 넣고 null문자 할당.
			if(input[i] == ' ' || input[i] == '\n') {
				prompt_option[num][length+1] = 0;
				length = 0;
				num++;
			}
			else 
				prompt_option[num][length++] = input[i];	
		}
		if(strcmp(prompt_option[0],"ls") == 0) { // ls를 입력 받은 경우
			system(input);
			continue;
		}
		else if(strcmp(prompt_option[0],"vi") == 0) { // vi를 입력 받은 경우
			system(input);
			continue;
		}
		else if(strcmp(prompt_option[0],"vim") == 0) { // vim을 입력 받은 경우
			system(input);
			continue;
		}
		else if(strcmp(prompt_option[0],"list") == 0) { // list 명령어를 입력 받은 경우
			list();
			continue;
		}
		else {
			for(int i = 0; i < 4; i++) { // prompt에 입력 받은 옵션과 option_list가 일치한 경우
				if(strcmp(prompt_option[0],option_list[i]) == 0) {
					option_state[i] = 1;
					flag = 1;
				}
			}
		}
		
		if(flag == 0) { // Option이 존재하지 않는 경우
			fprintf(stderr, "Error : Option it's not exist.\n");
			continue;
		}

		for(int i = 0; i < 4; i++) {

			if(option_state[i] == 1 && i == 0) // add 명령
			{
				struct stat add_file_info;
				int temp_len=0,temp_len2=0;
				char FILE_NAME[256]={0};			
	
				if((strcmp(prompt_option[1],"-d") != 0)) { // -d 옵션이 없는 경우
					temp_len = strlen(prompt_option[1]);
					realpath(prompt_option[1], FILE_NAME);
					temp_len2 = strlen(FILE_NAME);
				}
				else if((strcmp(prompt_option[1],"-d") == 0)) { // -d 옵션이 존재하는 경우
					temp_len = strlen(prompt_option[2]);
					realpath(prompt_option[2],FILE_NAME);
					temp_len2 = strlen(FILE_NAME);
				}	

				if(temp_len > 255 || temp_len2 > 255) { // Filename 초과
					fprintf(stderr, "Error : File name limit over\n");
					break;
				}
				if(count <= 2) { // FILENAME, PERIOD가 있는가? //
					if(count <= 1) { // FILENAME 없음.
						fprintf(stderr, "Error : FileName it's not exist\n");
						break;
					}
					else if(count <= 2) { // PERIOD 없음.
						fprintf(stderr, "Error : Period it's not exist\n");
						break;
					}
				}
				else if(count >= 3) {
					if((strcmp(prompt_option[1],"-m") == 0) || (strcmp(prompt_option[1],"-n") == 0) || (strcmp(prompt_option[1],"-t") == 0)) {
						fprintf(stderr, "Error : FileName it's not exist\n");
						break;
					}
					if((strcmp(prompt_option[2],"-m") == 0) || (strcmp(prompt_option[2],"-n") == 0) || (strcmp(prompt_option[2],"-t") == 0)) {
						fprintf(stderr, "Error : Period it's not exist\n");
						break;
					}	
				}
				if((strcmp(prompt_option[1],"-d") != 0)) { // -d 옵션인 경우 예외
					if(access(prompt_option[1],F_OK) < 0) { // File이 존재하지 않음.
						fprintf(stderr, "Error : File it's not exist.\n");
						break;
					}
					if(lstat(prompt_option[1], &add_file_info) < 0) { // add할 파일이 심볼릭 파일인가 확인.
						fprintf(stderr, "Error : lstat error\n");
						break;
					}
					if(!S_ISREG(add_file_info.st_mode)) { // 일반 파일인가 확인.
						fprintf(stderr, "Error : It's not Regular File\n");
						break;
					}
					if(pthread_create(&tid, NULL, add, input) != 0) { // Thread 생성.
						fprintf(stderr, "Error : pthread_create() error\n");
						break;
					}
				}
				else if((strcmp(prompt_option[1],"-d") == 0)) { // -d 옵션의 경우.
					if(strcmp(prompt_option[2],"") == 0) { // 디렉토리 입력을 하지 않음.
						fprintf(stderr, "Error : It's not exist directory\n");
						break;
					}
					if(strcmp(prompt_option[3],"") == 0) { // period를 입력하지 않음.
						fprintf(stderr, "Error : It's not exist period.\n");
						break;
					}
					if(integer_check(prompt_option[3],strlen(prompt_option[3])) == 0) {
						fprintf(stderr, "Error : Period it's not integer.\n");
						break;
					} 	
					if(access(prompt_option[2],F_OK) < 0) { // File이 존재하지 않음.
						fprintf(stderr, "Error : It's not find directory\n"); 
						break;
					}
					d_count = 0;
					listdir(FILE_NAME);
					for(int i = 0; i < d_count-1; i++) { // 읽어온 폴더 목록 정렬.
						for(int j = i +1; j < d_count; j++) {
							char sort_temp[300]={0};
							if(strcmp(directory_temp[i],directory_temp[j]) > 0) { // 디렉토리 오름차순 정렬
								strcpy(sort_temp,directory_temp[i]);
								strcpy(directory_temp[i],directory_temp[j]);
								strcpy(directory_temp[j],sort_temp);
							}
						}
					}
					for(int i = 0; i < d_count; i++) { // 읽어온 폴더 목록을 저장하여 쓰레드 생성.
						int d_num = 3;
						/* d옵션에 대해서는 다음과 같이 옵션을 넣어준다.*/
						strcat(d_temp[d_count2],prompt_option[0]);
						strcat(d_temp[d_count2]," ");
						strcat(d_temp[d_count2],directory_temp[i]);
						strcat(d_temp[d_count2]," ");
						strcat(d_temp[d_count2],prompt_option[3]);
						if(d_num == 3 && d_num == count) {
							strcat(d_temp[d_count2],"\n");
						}
						else {
							strcat(d_temp[d_count2]," ");
						}
						for(int j = 4; j < count ; j++) {
							if(j == count-1) {
								strcat(d_temp[d_count2],prompt_option[j]);
								strcat(d_temp[d_count2],"\n");
							}
							else {
								strcat(d_temp[d_count2],prompt_option[j]);
								strcat(d_temp[d_count2]," ");
							}
						}
						/* d옵션 처리 종료.*/
						if(pthread_create(&tid, NULL, add, d_temp[d_count2++]) != 0) { // Thread 생성.
							fprintf(stderr, "Error : pthread_create() error\n");
							break;
						}
					}
					for(int i = 0; i < d_count; i++) 
						directory_temp[i][0] = 0;		
				} // -d 명령 종료.
			} // add 명령 종료.
			else if(option_state[i] == 1 && i == 1) // remove 명령
			{
				char FILE_NAME[256]={0};
				if(strcmp(prompt_option[1],"-a") != 0) { // -a 옵션이 없는 경우 처리.
					realpath(prompt_option[1],FILE_NAME);
					if(strcmp(prompt_option[1],"") == 0) {
						fprintf(stderr, "FILENAME it's not exist\n");
						break;
					}
					if(access(FILE_NAME,F_OK) < 0) {
						fprintf(stderr, "Error : File it's not exist\n");
						break;
					}
					remove_cur = head;
					if(head == NULL) {
						fprintf(stderr, "Error : Data it's not exist\n");
						break;
					}
					else {
						struct tm *date;
						const time_t t = time(NULL);
						char *log_path = "log.txt";
						char log_temp[350]={0};
						char days[11]={0};
						char times[11]={0};
						int log_fd;
						int found = 0;
						date = localtime(&t);
						sprintf(days,"%02d%02d%02d",(date->tm_year+1900) % 100, date->tm_mon+1,date->tm_mday);
						sprintf(times,"%02d%02d%02d",date->tm_hour,date->tm_min,date->tm_sec);
		
						if((log_fd = open(log_path,O_WRONLY | O_CREAT | O_APPEND)) < 0) {
							fprintf(stderr, "Error : File Open Error\n");
							break;
						}
						if(remove_cur == head && (strcmp(remove_cur -> path, FILE_NAME) == 0)) { // head 값이 삭제 대상인 경우
							backup_list *temp_node = head;
							if(head -> next == NULL) { // head만 있는 경우
								sprintf(log_temp,"[%s %s] %s deleted\n",days,times,head->path);
								write(log_fd,log_temp,strlen(log_temp));
								pthread_cancel(head->pid); // 쓰레드 종료.
								free(head); // 메모리 회수
								head = null_pointer;
								close(log_fd);
								found = 1;
								break;
							}
							else if(head -> next != NULL) { // head만 있는 경우가 아닌 경우
								temp_node = head -> next;
								sprintf(log_temp,"[%s %s] %s deleted\n",days,times,head->path);
								write(log_fd,log_temp,strlen(log_temp));
								pthread_cancel(head->pid); // 쓰레드를 종료한다.
								free(head);
								head = temp_node;
								close(log_fd);
								found = 1;
								break;
							}
						}
						remove_cur = head;
						backup_list *next_cur = NULL;
						while(remove_cur != NULL) { // 중간 값 또는 마지막 노드가 삭제할 대상인 경우
							if(strcmp(remove_cur -> path, FILE_NAME) == 0) {
								next_cur -> next = remove_cur -> next;
								sprintf(log_temp,"[%s %s] %s deleted\n",days,times,remove_cur->path); 
								write(log_fd,log_temp,strlen(log_temp));
								pthread_cancel(remove_cur->pid);
								free(remove_cur);
								remove_cur = null_pointer;
								close(log_fd);
								found = 1;
								break;
							}
							next_cur = remove_cur;
							remove_cur = remove_cur -> next;
						}
						if(found == 0) {
							fprintf(stderr,"Error : File it's not exist in backup_list\n");
							close(log_fd);
							break;
						}
						close(log_fd);
					}
				}
				else if(strcmp(prompt_option[1],"-a") == 0) { // -a 옵션이 있는 경우
					struct tm *date;
					const time_t t = time(NULL);
					char *log_path = "log.txt";
					char log_temp[350] = {0};
					char days[11]={0};
					char times[11]={0};
					int log_fd;					
				
					date = localtime(&t);
					sprintf(days,"%02d%02d%02d",(date->tm_year+1900) % 100, date->tm_mon+1,date->tm_mday);
					sprintf(times,"%02d%02d%02d",date->tm_hour,date->tm_min,date->tm_sec);
	
					if(strcmp(prompt_option[2],"") != 0) { // 파일 입력을 입력 받은 경우 오류처리.
						fprintf(stderr,"Error : remove -a option it's not need filename.\n");
						break;
					}
					remove_cur = head;
					backup_list *temp_cur;
					temp_cur = head -> next;

					if((log_fd = open(log_path,O_WRONLY | O_CREAT | O_APPEND)) < 0) {
						fprintf(stderr, "Error : File Open Error\n");
						break;
					}

					if(head-> next == NULL) { // head만 있는경우 이다.
						sprintf(log_temp,"[%s %s] %s deleted\n",days,times,head->path);
						write(log_fd,log_temp,strlen(log_temp));
						pthread_cancel(head->pid);
						head = null_pointer;
						free(head);
					}
					else { // head 이외의 데이터가 여러개인 경우이다.
						while(temp_cur != NULL) {
							remove_cur = temp_cur;
							temp_cur = temp_cur->next;
							sprintf(log_temp,"[%s %s] %s deleted\n",days,times,remove_cur->path);
							write(log_fd,log_temp,strlen(log_temp));
							pthread_cancel(remove_cur->pid);
							remove_cur = NULL;
							free(remove_cur);
						}
						sprintf(log_temp,"[%s %s] %s deleted\n",days,times,head->path);
						write(log_fd,log_temp,strlen(log_temp));
						pthread_cancel(head->pid);
						head = null_pointer;
						free(head);	
					}
				}
			}	
			else if(option_state[i] == 1 && i == 2) // compare 명령
			{
				if(count != 3) {
					fprintf(stderr, "Error : Usage a File : %s\n", argv[0]);
					break;
				}	
				compare_file(prompt_option[1], prompt_option[2]);
				break;
			}
			else if(option_state[i] == 1 && i == 3) // recover 명령
			{
				struct stat recover_info;
				struct dirent **namelist;
				char file_list[500][256] ={0};
				char only_name[256]={0};
				char FILE_NAME[256]={0};
				char FILE_NAME2[256]={0};
				int file_count;
				int only_len;
				int only_index;
				int recover_count = 0;
				int recover_input;
				
				if((strcmp(prompt_option[1],"") == 0)) { // -n 옵션이 없으면서 파일 이름 입력을 안한 경우
					fprintf(stderr, "Error : Filename it's not exist\n");
					break;
				}
				if(access(prompt_option[1],F_OK) < 0) { // 파일이 없는 경우
					fprintf(stderr ,"Error : File it's not find\n");
					break;
				}
				if((strcmp(prompt_option[2],"-n") == 0) && (strcmp(prompt_option[3],"") == 0)) { // -n 옵션이 있으면서 파일 입력을 안한 경우
					fprintf(stderr,"Error : Filename it's not exist\n");
					break;
				}
				if((strcmp(prompt_option[2],"-n") == 0) && (access(prompt_option[3],F_OK) == 0)) { // -n 옵션이 있으면서 파일이 있는 경우
					fprintf(stderr, "Error : File it's exist\n");
					break;
				}
		
				realpath(prompt_option[1],FILE_NAME); // FILE_NAME을 읽어온다.
				if(strcmp(prompt_option[2],"-n") == 0)
					realpath(prompt_option[3],FILE_NAME2); // -n 옵션인 경우 덮어쓸 파일 경로도 읽음.
	
				only_len = strlen(FILE_NAME);
				for(int i = only_len-1; i >= 0; i--) {
					if(FILE_NAME[i] == '/')	{
						only_index = i+1;
						break;
					}
				}
				for(int i = only_index; i < only_len; i++) {
					append(only_name, FILE_NAME[i]);
				}
				if((file_count = scandir(pathname, &namelist, NULL, alphasort)) == -1) {
					fprintf(stderr, "Error : scandir Error\n");
					break;
				}
				for(int i = 0; i < file_count; i++) { // 파일 목록을 전부 돌아본다.
					char remove_temp[300]={0}; 
					char compare_temp[256]={0};
					char compare_temp2[256]={0};
					int compare_len = strlen(namelist[i]->d_name);
					int compare_index = 0;
					int compare_index2 = 0;

					for(int j = compare_len - 1; j >= 0; j--) { // 파일 이름을 추출하는 과정
						if(namelist[i]->d_name[j] == '_') {
							compare_index2 = j;
							break;
						}
						if(namelist[i]->d_name[j] == '/') {
							compare_index = j+1;
							break;
						}
						
					}
					for(int j = compare_index; j < compare_index2; j++) { // 파일 이름을 저장한다.
						if(namelist[i]->d_name[j] == 0)
							break;
						append(compare_temp,namelist[i]->d_name[j]);
					}
			
					strcat(remove_temp,pathname);
					strcat(remove_temp,"/");
					strcat(remove_temp,namelist[i]->d_name);
					if(strcmp(only_name,compare_temp) == 0) { // file_list와 비교하여 같으면 복사.
						strcpy(file_list[recover_count++],remove_temp);
					}
				}
				if(recover_count == 0) {
					fprintf(stderr, "It's not exist\n");
					break;
				}
				printf("0. exit\n");
				for(int i = 0; i < recover_count; i++) { // 파일 리스트를 출력한다.
					struct stat compare_info;
					char recover_temp[300]={0};
					int recover_index;
					int recover_len = strlen(file_list[i]);
					if(lstat(file_list[i],&compare_info) < 0) {
						fprintf(stderr,"lstat error\n");
						continue;
					}
		
					for(int j = recover_len - 1; j >= 0; j--) {
						if(file_list[i][j] == '_') {
							recover_index = j+1;
							break;
						}
					}
					for(int j = recover_index; j < recover_len; j++) {
						append(recover_temp,file_list[i][j]);
					}
					printf("%d. %s		%ldbytes\n",i+1,recover_temp,compare_info.st_size);
				}
				printf("Choose file to recover : ");
				scanf("%d",&recover_input);
				if(recover_input == 0) {
					getchar();
					break;
				}
				else if(recover_input > recover_count) { // 파일 목록보다 큰 값 입력한 경우
					fprintf(stderr, "It's not exist\n");
					getchar();
					break;
				}
				else {
					int load_fd;
					int write_fd;
					int search_file = 0;
					cur = head;
					if(cur != NULL) {
						if(strcmp(FILE_NAME,cur -> path) == 0)
							search_file = 1;
						cur = cur -> next;
					}
					if(search_file == 1) { // 백업 리스트에 있는 경우
						remove_cur = head;
						if(remove_cur == head && (strcmp(remove_cur -> path, FILE_NAME) == 0)) { // head 값이 삭제 대상인 경우
							backup_list *temp_node = head;
							if(head -> next == NULL) { // head만 있는 경우
								pthread_cancel(head->pid); // 쓰레드 종료.
								free(head); // 메모리 회수
								head = null_pointer;
							}
							else if(head -> next != NULL) { // head만 있는 경우가 아닌 경우
								temp_node = head -> next;
								pthread_cancel(head->pid); // 쓰레드를 종료한다.
								free(head);
								head = temp_node;
							}
						}
						remove_cur = head;
						backup_list *next_cur = NULL;
						while(remove_cur != NULL) { // 중간 값 또는 마지막 노드가 삭제할 대상인 경우
							if(strcmp(remove_cur -> path, FILE_NAME) == 0) {
								next_cur -> next = remove_cur -> next;
								pthread_cancel(remove_cur->pid);
								free(remove_cur);
								remove_cur = null_pointer;
								break;
							}
							next_cur = remove_cur;
							remove_cur = remove_cur -> next;
						}
					}
					if((load_fd = open(file_list[recover_input-1],O_RDONLY)) < 0) { // 파일 읽기 과정
						fprintf(stderr, "Error : File Open Error\n");
						break;
					}
					if(strcmp(prompt_option[2],"-n") == 0) { // -n 옵션이 존재하는 경우 파일 쓰는 경로 변경
						if((write_fd = open(FILE_NAME2,O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
							fprintf(stderr, "Error : File Open Error\n");
							break;
						}
					}
					else {
						if((write_fd = open(FILE_NAME,O_WRONLY | O_CREAT | O_TRUNC,0644)) < 0) { // 파일 쓰기
							fprintf(stderr, "Error : File Open Error\n");
							break;
						}
					}
					int write_count;
					char write_buffer[100];
					while((write_count = read(load_fd,write_buffer,100)) > 0) { // 파일 쓰는 과정
						write(write_fd, write_buffer, write_count);	
					}	
					printf("Recovery success\n");
					close(load_fd);
					close(write_fd);
					getchar();
				} // recover (파일 선택 종료.)
			} // recover 명령 종료.
		} // for문 종료.
			 
	} // while 문 종료
}

void *add(void *arg) 
{
	backup_list *NODE;
	struct stat backup_file_info;	
	struct stat	backup_directory_info; 
	struct tm *date;
	const time_t t = time(NULL);

	char *input = NULL;
	char add_option[20][256]={0};
	char *real_file_path;
	char backup_path[256]={0}; // 백업 파일 경로
	char *directory_path;
	char *log_path = "log.txt";
	char temp_path[256]={0};
	char temp_file[256]={0}; // 실제 파일 이름 담는 변수
	char days[10]={0}; // 날짜를 담는 변수
	char times[10]={0}; // 시간을 담는 변수

	int option_state[4]={0};
	int count = 0;
	int length = 0;
	int num = 0;
	int temp_file_len = 0;
	int index;
	int load_fd;
	int save_fd;
	int log_fd;	
	int period = 0;
	int	delete_count = 0;
	int	delete_time = 0; 
		
	input = arg;
	for(int i = 0; i < strlen(input); i++) { // 엔터나 띄어쓰기를 구분한다.
		if(input[i] == ' ' || input[i] == '\n') 
			count++;
		if(input[i] == '\n')
			break;	
	}

	num = length = 0;
	for(int i = 0; i < strlen(input); i++) { // 엔터나 띄어쓰기 이전까지의 문자를 넣고 null문자 할당.
		if(input[i] == ' ' || input[i] == '\n') {
			add_option[num][length+1] = 0;
			length = 0;
			num++;
		}
		else 
			add_option[num][length++] = input[i];	
	}
	strcpy(temp_path,add_option[1]);
	real_file_path = malloc(256);
	for(int i = 3; i < count; i++) {
		if(strcmp(add_option[i],"-m") == 0) { // -m 옵션인 경우
			option_state[0]++;
		}
		else if(strcmp(add_option[i],"-n") == 0) { // -n 옵션인 경우
			option_state[1]++;
			if(integer_check(add_option[i+1],strlen(add_option[i+1])) == 0) { // -n옵션 바로 뒤가 정수가 아닌 경우
				fprintf(stderr,"Error : It's not Integer.\n");	
				return NULL;
			}
			if((i + 1 >= 20) || (strcmp(add_option[i+1],"") == 0)) { // 숫자를 입력하지 않은 경우
				fprintf(stderr,"Error : It's not exist number\n");
				return NULL;
			}
			delete_count = atoi(add_option[i+1]);
			
		}
		else if(strcmp(add_option[i],"-t") == 0) { // -t 옵션인 경우
			option_state[2]++;
			if(integer_check(add_option[i+1],strlen(add_option[i+1])) == 0) { // -t옵션 바로 뒤가 정수가 아닌 경우
				fprintf(stderr,"Error : It's not Integer.\n");
				return NULL;
			}
			if((i+1 >= 20) || (strcmp(add_option[i+1],"") == 0)) { // 시간을 입력하지 않은 경우	
				fprintf(stderr,"Error : It's not exist time\n");
				return NULL;
			}
			delete_time = atoi(add_option[i+1]); 
		}
	}
	pthread_mutex_lock(&mutex);
	if(realpath(temp_path,real_file_path) == NULL) { // 파일 경로 추출
		fprintf(stderr, "Error : Path name Error\n");
		pthread_mutex_unlock(&mutex);
		return NULL;	
	}
	temp_file_len = strlen(real_file_path);
	for(int i = temp_file_len-1; i >= 0; i--) { // 파일 이름만 추출하기 위한 과정  
		if(real_file_path[i] == '/') {
			index = i+1;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	for(int i = index; i < temp_file_len; i++) // 실제 파일이름 append.
		append(temp_file,real_file_path[i]);							
	sprintf(backup_path,"%s/%s",pathname,temp_file); // 백업파일 경로 입력
	lstat(real_file_path,&backup_file_info);
		

	pthread_mutex_lock(&mutex);
	if((log_fd = open(log_path,O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) {
		fprintf(stderr, "Error : File Open Error\n");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	pthread_mutex_unlock(&mutex);
	if(head == NULL) { // 노드가 존재하지 않는 경우. (첫번째 노드로 넣는 경우)
		pthread_mutex_lock(&mutex);
		char log_temp[350]={0};	
		date = localtime(&t);
		head = (backup_list *) malloc(sizeof(backup_list));
		NODE = (backup_list *) malloc(sizeof(backup_list));
	
		NODE -> path = (char *) malloc(temp_file_len);	 
		strcpy(NODE -> path, real_file_path);
		NODE -> period = atoi(add_option[2]);
		NODE -> next = NULL;
		NODE -> pid = pthread_self();
		head = NODE;

		if(option_state[0] > 0) { // -m 옵션이 있는 경우
			NODE -> m_on = 1;
			NODE -> mtime = backup_file_info.st_mtime;
		}
		if(option_state[1] > 0) { // -n 옵션이 있는 경우
			NODE -> n_on = 1;
			NODE -> delete_count = delete_count;
		}
		if(option_state[2] > 0) { // -t 옵션이 있는 경우
			NODE -> t_on = 1;
			NODE -> delete_time = delete_time;
		}
	 /* log.txt에 시간 정보 기록 */		
		sprintf(days,"%02d%02d%02d",(date->tm_year+1900) % 100, date->tm_mon+1, date->tm_mday);
		sprintf(times,"%02d%02d%02d",date->tm_hour,date->tm_min,date->tm_sec);	
		sprintf(log_temp,"[%s %s] %s added\n",days,times,real_file_path);	
		write(log_fd,log_temp,strlen(log_temp));
		close(log_fd);			
		pthread_mutex_unlock(&mutex);
	}  
	else { // 노드가 이미 존재하는 경우
		pthread_mutex_lock(&mutex);
		char log_temp[300]={0};
		cur = head;
		date = localtime(&t);
		while(cur != NULL) {
			if(cur != NULL) {
				if(strcmp(real_file_path,cur->path) == 0) {
					fprintf(stderr, "File it's exist this file it's not added list.");
					pthread_mutex_unlock(&mutex);
					return NULL;
				}
			}
			if(cur -> next == NULL) { // 노드의 가장 끝에 도달한 경우 추가.
				NODE = (backup_list *) malloc(sizeof(backup_list));
			
				NODE -> path = (char *) malloc(temp_file_len);
				strcpy(NODE -> path, real_file_path);
				NODE -> period = atoi(add_option[2]);
				NODE -> next = NULL;
				NODE -> pid = pthread_self();
				cur -> next = NODE;
				
				if(option_state[0] > 0) { // -m 옵션이 있는 경우
					NODE -> m_on = 1;
					NODE -> mtime = backup_file_info.st_mtime;  
				}
				if(option_state[1] > 0) { // -n 옵션이 있는 경우
					NODE -> n_on = 1;
					NODE -> delete_count = delete_count;
				}
				if(option_state[2] > 0) { // -t 옵션이 있는 경우
					NODE -> t_on = 1;
					NODE -> delete_time = delete_time;
				}
				sprintf(days,"%02d%02d%02d",(date->tm_year+1900) % 100, date->tm_mon+1, date->tm_mday);
				sprintf(times,"%02d%02d%02d",date->tm_hour,date->tm_min,date->tm_sec);	
				sprintf(log_temp,"[%s %s] %s added\n",days,times,real_file_path);	
				write(log_fd,log_temp,strlen(log_temp));
				close(log_fd);
				pthread_mutex_unlock(&mutex);
				break;
			}
			else 
				cur = cur -> next;
		}
		pthread_mutex_unlock(&mutex);
	}
	while(1) { // 쓰레드 반복과정.
		struct stat temp_info;
		struct dirent **namelist;		
		pthread_mutex_unlock(&mutex);
		sleep(NODE->period); // PERIOD 마다 반복 한다.
		pthread_mutex_lock(&mutex);
		const time_t t2 = time(NULL);	
		date = localtime(&t2);		
		pthread_mutex_unlock(&mutex);

		char backup_file_name[300] = {0}; // 백업시 파일 명
		char log_temp[300] = {0};
		char ch_buffer[100]={0};		

		int ch_length;
		int file_count = 0;
		int del_count = 0;
		int m_timeon = 0;
		
		pthread_mutex_lock(&mutex);		
		strcat(backup_file_name,backup_path);
		strcat(backup_file_name,"_");
		strcat(backup_file_name,days);
		strcat(backup_file_name,times);
		pthread_mutex_unlock(&mutex);

		pthread_mutex_lock(&mutex);	
		if(NODE -> m_on == 1) {
			if(lstat(real_file_path, &temp_info) < 0) {
				fprintf(stderr,"Error : lstat error\n");
				continue;
			}
			if(temp_info.st_mtime == NODE -> mtime) { // mtime이 같으면 백업을 진행하지 않음. (기존 생성된 파일 지움.)
				
				m_timeon = 1;
			}
			else
				NODE -> mtime = temp_info.st_mtime; // mtime이 다르면 list에 mtime을 새로 갱신.
		}
		if(NODE -> n_on == 1 || NODE -> t_on == 1) { // -n이 활성화 된 경우, -t 옵션이 활성화 된 경우
			if((file_count = scandir(pathname, &namelist, NULL, alphasort)) == -1) {
				fprintf(stderr, "Error : scandir Error\n");
				pthread_mutex_unlock(&mutex);
				return NULL;
			}
			for(int i = 0; i < file_count; i++) { // 파일 목록을 전부 돌아본다.
				struct stat compare_info;
				char remove_temp[300]={0}; 
				char compare_temp[256]={0};
				int compare_len = strlen(namelist[i]->d_name);
				int compare_index = 0;

				for(int j = compare_len - 1; j >= 0; j--) { // 파일 이름을 추출하는 과정
					if(namelist[i]->d_name[j] == '/') {
						compare_index = j;
						break;
					}
				}
				for(int j = compare_index; j < compare_len; j++) { // 파일 이름을 저장한다.
					if(namelist[i]->d_name[j] == '_')
						break;
					append(compare_temp,namelist[i]->d_name[j]);
				}
				
				strcat(remove_temp,pathname);
				strcat(remove_temp,"/");
				strcat(remove_temp,namelist[i]->d_name);
				if((strcmp(compare_temp,temp_file) == 0)) 
					del_count++;
				if((strcmp(compare_temp,temp_file) == 0) && (NODE -> t_on == 1)) { // -t 옵션이 활성화 되어 있는 경우
					if(lstat(remove_temp, &compare_info) < 0) {
						fprintf(stderr,"Error : file lstat error\n");
						continue;
					}
					if(t2 >= (compare_info.st_mtime + NODE -> delete_time)) { // 생성된 시간이 주어진 시간보다 큰 경우
						remove(remove_temp); // 삭제
						if(NODE -> n_on == 1) 
							del_count--;
						continue;
					}
				}
			}		

			if(NODE -> n_on == 1) { // -n 옵션이 켜져 있는 경우 
				for(int i = 0; i < file_count; i++) {

					char remove_temp[300]={0};
					char compare_temp[256]={0};
					int compare_len = strlen(namelist[i]->d_name);
					int compare_index = 0;
								
					for(int j = compare_len - 1; j >= 0; j--) { // 파일 이름을 읽어들임.
						if(namelist[i]->d_name[j] == '/')
							compare_index = j;
							break;
					}
					for(int j = compare_index; j < compare_len; j++) { // 날짜 이름을 읽어들임.
						if(namelist[i]->d_name[j] == '_')
							break;
						append(compare_temp,namelist[i]->d_name[j]);
					}
					if(strcmp(compare_temp,temp_file) == 0) {
						strcat(remove_temp,pathname);
						strcat(remove_temp,"/");
						strcat(remove_temp,namelist[i]->d_name);
						if(del_count >= NODE -> delete_count) {
							remove(remove_temp);
							del_count--;
							continue;
						}
						else 
							break;
					}	
				}					
			}
			for(int i = 0; i < file_count; i++) { // memory 반환
				free(namelist[i]);
			}
			free(namelist); // memory 반환
		}
		if(m_timeon == 1) // mtime이 수정된 경우
			continue; 	
		if((log_fd == open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) { // log 기록.
			fprintf(stderr, "Error : File Open Error : \n"); 
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		/* log.txt를 쓴다. */
		sprintf(days,"%02d%02d%02d",(date->tm_year+1900) % 100, date->tm_mon +1, date -> tm_mday);
		sprintf(times,"%02d%02d%02d",date->tm_hour,date->tm_min,date->tm_sec);
		sprintf(log_temp,"[%s %s] %s generated\n",days,times,real_file_path);
		write(log_fd, log_temp, strlen(log_temp));		
		close(log_fd);

		if((load_fd = open(real_file_path,O_RDONLY)) < 0) { // 백업 할 파일에 대한 open.
			fprintf(stderr, "Error : open() error\n");
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
	
		if((save_fd = open(backup_file_name,O_WRONLY | O_CREAT,0644)) < 0) { // 백업하여 저장할 파일에 대한 open.
			fprintf(stderr, "Error : open() error\n");
			pthread_mutex_unlock(&mutex);
			return NULL;
		}

		while(1) {
			if((ch_length = read(load_fd, ch_buffer, 100)) > 0) // 백업을 진행한다.(복사하여)
				write(save_fd, ch_buffer, ch_length);
			else
				break;
		}
					
		close(load_fd); // 불러온 파일을 닫음.
		close(save_fd);	// 저장한 파일을 닫음.
	}
	
}
void append(char *des, char c) // 문자열 뒤에 문자를 추가한다.
{
	char *p = des;
	while (*p != '\0') p++;
	*p = c;
	*(p+1) = '\0';
}

void list(void) // list를 출력한다.
{
	if(head == NULL) {
		return ;
	}
	else {
		cur = head;
		while(cur != NULL) {
			printf("%s	%d",cur->path,cur->period);
			if(cur -> m_on == 1)  // -m 옵션이 있는 경우
				printf(" %s","-m");
			if(cur -> n_on == 1)  // -n 옵션이 있는 경우
				printf(" %s","-n"); 
			if(cur -> t_on == 1)  // -t 옵션이 있는 경우
				printf(" %s","-t");
			cur = cur -> next;
			
			printf("\n");
		}
	}	
}

int integer_check(char *str, int len) // 정수인가 확인하는 함수이다.
{
	for(int i = 0; i < len; i++) {
		if(str[i] >= '0' && str[i] <= '9') // 0 ~ 9 인가.(그렇지 맞으면 계속)
			continue;
		else
			return 0;
	}
	return 1;
}	

void listdir(const char *name) // 디렉토리를 읽어 들이는 과정.
{
	DIR *dir;
	struct dirent *list_dir;
		
		
	if(!(dir = opendir(name)))
		return ;

	while ((list_dir = readdir(dir)) != NULL) {
		if (list_dir -> d_type == DT_DIR) {
			char path[1024];
			if (strcmp(list_dir->d_name, ".") == 0 || strcmp(list_dir->d_name, "..") == 0) // '.'이나 '..'인 경우
				continue;
			snprintf(path, sizeof(path), "%s/%s", name,list_dir->d_name); 
			listdir(path);
		} 
		else {
			sprintf(directory_temp[d_count++],"%s/%s",name, list_dir->d_name); // list를 출력한다.
		}
	}
	closedir(dir);
}

