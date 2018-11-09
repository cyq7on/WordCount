//
// Created by cyq7on on 18-11-5.
//

#include<ctype.h>
#include<string.h>
#include <fcntl.h>             // 提供open()函数
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>            // 提供目录流操作函数
#include <string.h>
#include <sys/stat.h>        // 提供属性操作函数
#include <sys/types.h>         // 提供mode_t 类型
#include <stdlib.h>
#include <pthread.h>

#define Word_Max 100
#define File_Max 50
#define Thread_Count 3

typedef struct {
    char w[Word_Max];
    int count;
}Word;
Word a[1000];


//已经在读或者读完的文件
char haveReadFile[File_Max][128];
int fileCount = 0;

pthread_mutex_t mutex ;

/*
 * k是为了增加结构体的数量，每增加一个新的单词就加1.
 * flag是为了区别是否新读的单词在结构体里存在不存在，存在就count增1，不存在就纳入新的结构体并且count设置为1，
 * 在循环结束后改为0
*/
int flag = 0,k = 0;

void my_lock(){
    if (pthread_mutex_lock(&mutex) != 0){
        puts("lock error!\n");
    }
}

int getWordCount(const char* file_path) {
    FILE *fp;
    char words[10];
    char ch;
    /*
    1. i是控制读文件的起点，每读到一个英文字符都进一位，若不是英文字母则将起点设为0，重新开始读，另外进入else部分。
    2. j是为了检验结构体是否含有刚读的单词设置的起点，作用是循环结构体。
    */
    int i = 0,j = 0;

    if((fp = fopen(file_path,"rb")) == NULL){
        printf("Thread exit");
        pthread_exit(NULL);
    }
    while(!feof(fp)){
//        ch = tolower(fgetc(fp));
        ch = fgetc(fp);
        if(isalpha(ch)){
            //if中的isalpha是判断是不是英文字母的库，头文件是ctype
            my_lock();
            words[i] = ch;
            pthread_mutex_unlock(&mutex);
            i ++;
        }else{
            my_lock();
            words[i] = '\0';
            for(j = 0;j <= k;j ++){
                if(strcmp(a[j].w,words) == 0){
                    a[j].count ++;
                    flag = 1;
                    break;
                }
            }
            if(flag == 0){
                a[k].count = 1;
                strcpy(a[k].w,words);
                k ++;
            }
            flag = 0;
            i = 0;
            pthread_mutex_unlock(&mutex);

        }
    }

    /*my_lock();
    for(i = 0;i < k;i ++){
        printf("%s %d\n",a[i].w,a[i].count);
    }
    pthread_mutex_unlock(&mutex);*/
    printf("%s count by Thread-%lu\n",file_path,pthread_self());
    return 0;
}

//检测文件是否未读，name是带有上级目录的文件名
int checkFile(const char* name){
//    printf("Current Thread is %lu\n",pthread_self());
//    printf("now file is %s , while haveReadFile total %d: " ,name ,fileCount);
    /*my_lock();
    for (int i = 0; i < fileCount; i++) {
        printf("%s\t",haveReadFile[i]);
    }
    pthread_mutex_unlock(&mutex); // 给互斥体变量解除锁
    printf("\n");*/
    int canRead = 1;
    my_lock();
    for(int i = 0;i < fileCount;i ++){
        if (!strcmp(name,haveReadFile[i])) {
            canRead = 0;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

//    printf("checkFile is %d\n",canRead);
    return canRead;
}

void setFile(char* name){
    my_lock(); // 给互斥体变量加锁

    strcpy(haveReadFile[fileCount++],name);

    pthread_mutex_unlock(&mutex); // 给互斥体变量解除锁
}

void scanDir(char *dir, int depth) {// 定义目录扫描函数
    DIR *dp;                      // 定义子目录流指针
    struct dirent *entry;         // 定义dirent结构指针保存后续目录
    struct stat statbuf;          // 定义statbuf结构保存文件属性

    if((dp = opendir(dir)) == NULL) {// 打开目录，获取子目录流指针，判断操作是否成功
        puts("can't open dir");
        puts(dir);
        printf("Current Thread is %lu\n",pthread_self());
        return;
    }
//    chdir (dir);                     // 切换到当前目录
    while((entry = readdir(dp)) != NULL) {// 获取下一级目录信息，如果未否则循环
        char fullname[128];
        sprintf(fullname,"%s/%s",dir,entry->d_name);
        lstat(fullname, &statbuf); // 获取下一级成员属性

//        lstat(entry->d_name, &statbuf); // 获取下一级成员属性
        if(S_IFDIR &statbuf.st_mode) {// 判断下一级成员是否是目录
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;

//            printf("%*s%s/\n", depth, "", entry->d_name);  // 输出目录名称
            scanDir(fullname, depth + 4);              // 递归调用自身，扫描下一级目录的内容
        } else{
//            printf("%*s%s\n", depth, "", entry->d_name);  // 输出属性不是目录的成员
            if (checkFile(fullname)){
                setFile(fullname);
                getWordCount(fullname);
//                printf("%s统计完毕\n",fullname);
            }
        }

    }
//    chdir("..");                                                  // 回到上级目录
//    closedir(dp);                                                 // 关闭子目录流
}

void *thread(void *arg) {
//    int flag = 0,k = 0;
    scanDir((char*)arg,0);
    printf("Thread-%lu count end\n",pthread_self());
}

int main(int argc, char *argv[]){
    if (argc != 2){
        printf("no directory input\n");
        return -1;
    }

    pthread_mutex_init(&mutex, NULL); //按缺省的属性初始化互斥体变量mutex
    pthread_t pthread[Thread_Count] ;
    for (int i = 0; i < Thread_Count; i++) {
        pthread[i] = (unsigned long int)i;
        if (pthread_create(&pthread[i], NULL, thread, argv[1])) {
            printf("pthread%d create fail\n",i);
        } else{
            printf("pthread%d create success\n",i);
        }
    }

    for (int i = 0;i < Thread_Count; i++) {
        pthread_join(pthread[i],NULL);
    }

    pthread_mutex_destroy(&mutex);

    printf("Main Thread sum up：\n");
    for(int i = 0;i < k;i ++){
        printf("%s %d\n",a[i].w,a[i].count);
    }
}
