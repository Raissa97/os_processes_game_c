#include "master.h"

#include <sys/sem.h>
#include <errno.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h> 
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

void removeIPCS(int * arr){
  printf("Destroying Shared Memory....\n");
  /* chessboard */
  shmctl(arr[0],IPC_RMID,NULL);
  /* sem_flag */
  if(msgctl(arr[1], IPC_RMID, NULL)==-1) printf("%d", errno);
  /* array_flag */
  shmctl(arr[2],IPC_RMID,NULL);
  /* mutex */
  semctl(arr[3],0,IPC_RMID);
  /* pid_pos */
  shmctl(arr[4],IPC_RMID,NULL);
}

void f(int signal) {
  printf("TIME FINISHED\n");
  exit(0);
}

void setTimeout(void (*handler)(int), int seconds, int microseconds, int atomic) {
  /* struttura di handler del segnale*/
  struct sigaction s;
  struct itimerval timer;
  s.sa_handler = handler;
  if (atomic){
    sigfillset(&s.sa_mask);
    TEST_ERROR;
  }
  else{
    sigemptyset(&s.sa_mask);
    TEST_ERROR;
  }
  /* setta l'handler per il segnale */
  sigaction(SIGALRM, &s, NULL);
  TEST_ERROR;
  /* setta il prossimo scattare del timer */
  timer.it_value.tv_sec = seconds;
  timer.it_value.tv_usec = microseconds;
  /* questi sono per dire se bisogna ripetere il timer (setInterval) */
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  /* 
  starta il timer
  REAL per dire di aspettare il tempo reale
  http://man7.org/linux/man-pages/man2/setitimer.2.html 
  */
  setitimer(ITIMER_REAL, &timer, NULL);

  printf("\n\t\t\t\t ...TIMER STARTED...\n");
}

void msgRcv(int rcv_type, int q_id){
  struct msg_m_p my_msg;
  int num_bytes;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if (num_bytes >= 0) {
    /* received a good message (possibly of zero length) */
    printf("\nMASTER received the msg: (PID=%d): Q_id=%d: msg type=%ld \"%s\" RECEIVED\n", getpid(), q_id, my_msg.mtype, my_msg.mtext);
  }
  
   /*printf("%d\n",errno);
 now error handling */
  if (errno == EINTR) {
    fprintf(stderr, "(PID=%d): interrupted by a signal while waiting for a message of type %d on Q_ID=%d. Trying again\n",
      getpid(), rcv_type, q_id);
  }
  if (errno == EIDRM) {
    printf("(PID=%d): interrupted by an invalid argument message of type %d on Q_ID=%d. Try again\n",getpid(), rcv_type, q_id);
    exit(0);
  }
  if (errno == ENOMSG) {
    printf("No message of type=%d in Q_ID=%d. Not going to wait\n", rcv_type, q_id);
    exit(0);
  }    
  
}

int msgRcvFlagP(int q_id,int rcv_type){
  struct msg_pos my_msg;
  int num_bytes, i;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if(num_bytes >= 0) {
    for(i=0;i<MSG_MAX_SIZE;i++){
      printf("\n.......MASTER (%ld) received the msg Q_ID_=%d from pawn msg type=%ld the int is \"%d\" RECEIVED\n", (long)getpid(),q_id,my_msg.mtype,my_msg.mpos[i]);
      return my_msg.mpos[i];
    }
  }

  printf("%d\n",errno);
  /* now error handling */
  if (errno == EINTR) {
    fprintf(stderr, "(PID=%d): interrupted by a signal while waiting for a message of type %d on Q_ID=%d. Trying again\n",
      getpid(), rcv_type, q_id);
  }
  if (errno == EIDRM) {
    printf("The Q_ID=%d was removed. Let's terminate\n", q_id);
    exit(0);
  }
  if (errno == ENOMSG) {
    printf("No message of type=%d in Q_ID=%d. Not going to wait\n", rcv_type, q_id);
    exit(0);
  }

}

void msgSnd(int q_id,int msg_type,char msg_text[MSG_MAX_SIZE]){ 
  int i;
  struct msg_m_p my_msg;
    
  /* Constructing the message */
  my_msg.mtype = msg_type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mtext[i] = msg_text[i];
  }
  /* now sending the message */
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  MSGSND_ERROR;
  /*printf("master sent message on the queue(%d) to the player: %s\n",q_id,my_msg.mtext);*/

}

void msg_print_stats(int fd, int q_id) {
  struct msqid_ds my_q_data;
  int ret_val;
  
  while (ret_val = msgctl(q_id, IPC_STAT, &my_q_data)) {
    TEST_ERROR;
  }
  dprintf(fd, "\n\n--- IPC Message Queue ID: %8d, START ---\n", q_id);
  dprintf(fd, "---------------------- Time of last msgsnd: %ld\n",
    my_q_data.msg_stime);
  dprintf(fd, "---------------------- Time of last msgrcv: %ld\n",
    my_q_data.msg_rtime);
  dprintf(fd, "---------------------- Time of last change: %ld\n",
    my_q_data.msg_ctime);
  dprintf(fd, "---------- Number of messages in the queue: %ld\n",
    my_q_data.msg_qnum);
  dprintf(fd, "- Max number of bytes allowed in the queue: %ld\n",
    my_q_data.msg_qbytes);
  dprintf(fd, "----------------------- PID of last msgsnd: %d\n",
    my_q_data.msg_lspid);
  dprintf(fd, "----------------------- PID of last msgrcv: %d\n",
    my_q_data.msg_lrpid);  
  dprintf(fd, "--- IPC Message Queue ID: %8d, END -----\n\n\n", q_id);
}

static void shm_print_stats(int fd, int m_id) {
	struct shmid_ds my_m_data;
	int ret_val;
	
	while (ret_val = shmctl(m_id, IPC_STAT, &my_m_data)) {
		TEST_ERROR;
	}
  printf("\n");
	dprintf(fd, "--- IPC Shared Memory ID: %8d, START ---\n", m_id);
	dprintf(fd, "---------------------- Memory size: %ld\n",
		my_m_data.shm_segsz);
	dprintf(fd, "---------------------- Time of last attach: %ld\n",
		my_m_data.shm_atime);
	dprintf(fd, "---------------------- Time of last detach: %ld\n",
		my_m_data.shm_dtime); 
	dprintf(fd, "---------------------- Time of last change: %ld\n",
		my_m_data.shm_ctime); 
	dprintf(fd, "---------- Number of attached processes: %ld\n",
		my_m_data.shm_nattch);
	dprintf(fd, "----------------------- PID of creator: %d\n",
		my_m_data.shm_cpid);
	dprintf(fd, "----------------------- PID of last shmat/shmdt: %d\n",
		my_m_data.shm_lpid);
	dprintf(fd, "--- IPC Shared Memory ID: %8d, END -----\n", m_id);
  printf("\n");
}

void printState(int mem_id){
  struct chessboardBox *chessboard;
  int rows, cols, i, j, k, sem_id;

  printf("\nPrinting chessboard in sharedmemory \nval_flag,flag-sem_pawn-player\t1=free,0=occupied");
  printf("\n");

  cols = SO_BASE;
  rows = SO_ALTEZZA;
  chessboard = shmat(mem_id, NULL, 0);
  k = 0;

  for(i=1; i<=rows; i++){
    printf("|");
    for(j=1; j<=cols; j++){
      if((j%cols)==0){
        sem_id = chessboard[k].sem_pawn_id;
        printf("%d%d-%d-%d|",chessboard[k].flag_val,chessboard[k].flag, semctl(sem_id, 0, GETVAL,0), chessboard[k].player);
        printf("\n"); 
      }
      else {
        sem_id = chessboard[k].sem_pawn_id;
        printf("%d%d-%d-%d|",chessboard[k].flag_val,chessboard[k].flag, semctl(sem_id, 0, GETVAL,0), chessboard[k].player);
      }
      k += 1;
    }
  }
  printf("\n");
}

int * init(int *arr,struct chessboardBox *chessboard, struct pidPos *pid_pos_array) {
 	int mem_id, sem_id, sem_chessboard_id,sem_flag_mem_id,array_flag_id,mutex_id;
  int pid_pos_id,i,j;
  int sem_array_id;
  int round_mutex_id;

  /* Create a shared memory area for chessboard*/
	mem_id = shmget(IPC_PRIVATE, SIZE*sizeof(*chessboard), 0600);
	TEST_ERROR;
  arr[0]=mem_id;

  /* Create a shared memory area for moves' table*/
  pid_pos_id=shmget(IPC_PRIVATE,SO_NUM_P*SO_NUM_G*sizeof(*pid_pos_array),0600);
  TEST_ERROR;
  arr[4]=pid_pos_id;

	/* Attach the shared memory to chessboard pointer */
	chessboard = shmat(mem_id, NULL, 0);
  /*printf("MASTER: chessboard_id %d\n",mem_id);*/
	TEST_ERROR;
	
  /* Attach the shared memory to chessboard pointer */
  pid_pos_array=shmat(pid_pos_id,NULL,0);
	TEST_ERROR;

  /* initialization of chessboard*/
  for(i=0;i<SIZE;i++){    
    sem_id = semget(IPC_PRIVATE, 1, 0600);
    TEST_ERROR;
    /* setting semaphores to 1 FREE */
    semctl(sem_id, 0, SETVAL, 1); 
    TEST_ERROR;

    /*
    sem_chessboard_id = semget(IPC_PRIVATE, 1, 0600);
    TEST_ERROR;
    semctl(sem_chessboard_id, 0, SETVAL, 1); 
    TEST_ERROR;*/

    chessboard[i].sem_pawn_id=sem_id;
    chessboard[i].flag=1;
    chessboard[i].flag_val=0;
    chessboard[i].player=0; 
    /*printf("MASTER: sem_id is: %d, sem_id_value is: %d |\n", sem_id, semctl(sem_id, 0, GETVAL, 0));*/
  }

  /* initialization of moves' talbe*/
  for(i=0;i<SO_NUM_P*SO_NUM_G;i++){
    sem_array_id=semget(IPC_PRIVATE, 1,0600);
    TEST_ERROR;
    /* setting semaphores to 1 FREE */
    semctl(sem_array_id,0,SETVAL,1);
    TEST_ERROR;

    for(j=0;j<2;j++){
      pid_pos_array[i].pid_pos_array[j]=0;
      /*printf("pid pos array [%d] = %d\n",j,pid_pos_array[i].pid_pos_array[j]);*/
    }
    pid_pos_array[i].pid_pos_array[j]=sem_array_id;
    /*printf("pid pos array [%d] = %d\n",j,pid_pos_array[i].pid_pos_array[j]);*/
  }
  /*printf("pid pos array id = %d\n",pid_pos_id);*/


  sem_flag_mem_id=semget(IPC_PRIVATE, 1,0600);
  TEST_ERROR;
  arr[1]=sem_flag_mem_id;

  array_flag_id=shmget(IPC_PRIVATE,sizeof(struct flagPos)*5,0600);
  TEST_ERROR;
  arr[2]=array_flag_id;
  

  mutex_id=semget(IPC_PRIVATE,1,0600);
  TEST_ERROR;
  arr[3]=mutex_id;
  
  semctl(mutex_id,0,SETVAL,1);

  round_mutex_id=semget(IPC_PRIVATE,1,0600);
  TEST_ERROR;
  arr[5]=round_mutex_id;
  printf("ID MUTEX ROUND = %d\n",round_mutex_id);
  

  printf("\nInitialization OK\n");
  return arr; 
}

int initMatrix(struct capturedFlagMatrix *captured_flag_matrix){
  int flag_captured_id,sem_array_flag_id,i,j;
/* Create a shared memory area for flags captured table*/
  flag_captured_id=shmget(IPC_PRIVATE,SO_NUM_G*sizeof(*captured_flag_matrix),0600);
  TEST_ERROR;

  /* Attach the shared memory to chessboard pointer */
  captured_flag_matrix=shmat(flag_captured_id,NULL,0);
	TEST_ERROR;

    /* initialization of moves' talbe*/
  for(i=0;i<SO_NUM_G;i++){
    sem_array_flag_id=semget(IPC_PRIVATE, 1,0600);
    TEST_ERROR;
    /* setting semaphores to 1 FREE */
    semctl(sem_array_flag_id,0,SETVAL,1);
    TEST_ERROR;

    for(j=0;j<1;j++){
      captured_flag_matrix[i].array[j]=-1;
    }
    captured_flag_matrix[i].array[j]=sem_array_flag_id;
  }

}

int initQueue(){
  int i,q_id;
  /*
  * Create a message queue if it does not exist. Just get its
  * ID if it exists already. The key MY_KEY is shared via a
  * common #define
  */  
  
  q_id= msgget(IPC_PRIVATE, IPC_CREAT | 0600);/*from master to player*/
  TEST_ERROR; 
  /*printf("MASTER(%d): queue initialization ID %d\n",getpid(),q_id[i]);*/
  
  
  return q_id;
}

int numberFlagCreator(int sem_flag_mem_id){
  int number_of_flags,num;
  int a = SO_FLAG_MIN;
  int b = SO_FLAG_MAX;
  
  sops.sem_num=0;    
  sops.sem_flg=0;  

  if(a==b){
    number_of_flags=a;
  } else {
    number_of_flags = a + rand() %(b-a);
  }

  sops.sem_op=number_of_flags;  
  semop(sem_flag_mem_id,&sops,1);

  printf("inizializzato semaforo con ID = %d \t num bandierine = %d\n",sem_flag_mem_id,semctl(sem_flag_mem_id,0,GETVAL,0));
  return number_of_flags;
}

int flagPlacer(int mem_id, int sem_flag_mem_id,int array_flag_id){
  struct chessboardBox *chessboard = shmat(mem_id, NULL, 0);
  int num_of_flags=semctl(sem_flag_mem_id,0,GETVAL,0);
  int i=0,sem_pawn,val,flag_pos;
  struct flagPos *array_flag=shmat(array_flag_id,NULL,0);
  TEST_ERROR;
  
  val = (SO_ROUND_SCORE/num_of_flags);
  printf("Master is placing the %d flags\n",num_of_flags);

  srand(time(NULL));
  /* se la casella e' libera da pedine e bandierine*/
  while(i<num_of_flags){
    array_flag[i].length=num_of_flags;
    flag_pos= (rand()%SIZE)+1;
    sem_pawn=semctl(chessboard[flag_pos].sem_pawn_id,0,GETVAL,0);
    TEST_ERROR;

    if(sem_pawn==1 && chessboard[flag_pos].flag==1){
      /*printf("SEM PAWN = %d, chessboard[%d].flag = %d\t",sem_pawn,flag_pos,chessboard[flag_pos].flag);*/
      chessboard[flag_pos].flag=0;
      chessboard[flag_pos].flag_val=val;
      array_flag[i].flag_pos=flag_pos;
      printf("arr_flag[%d] = %d\n",i,array_flag[i].flag_pos);
      i++;
    }
  }
  printf("\t%d flags placed! OK\n",array_flag->length);

  return val;
}

pid_t * playerCreation(int arr_id[4],pid_t *pid, int mu_id,int queue_of_player){
  /* Preparing command-line arguments for child's execve */
  struct chessboardBox *chessboard;
  int status, i;
  pid_t fork_value;

  int mem_id=arr_id[0];
  int sem_flag_id=arr_id[1];
  int array_flag_id=arr_id[2];
  int mutex_id=arr_id[3];
  int pid_pos_id=arr_id[4];
  int round_mutex_id=arr_id[5];

  char m_id_str[3*sizeof(mem_id)+1]; /* sufficiente per stringa m_id */
  char num_player_str[3*1+1]; /* sufficiente per stringa m_id */
  char sem_flag_id_str[3*sizeof(sem_flag_id)+1];
  char array_flag_id_str[3*sizeof(array_flag_id)+1];
  char mutex_id_str[3*sizeof(int)+1];
  char mu_id_str[3*sizeof(int)+1];
  char pid_pos_str[3*sizeof(int)+1];
  char q_id_str[3*sizeof(int)+1];
  char round_mutex_str[3*sizeof(int)+1];

  char * args[11] = {"./player", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  sprintf(m_id_str, "%d", mem_id);
  args[1]=m_id_str;
  chessboard = shmat(mem_id, NULL, 0);
  sprintf(sem_flag_id_str, "%d", sem_flag_id);
  args[3]=sem_flag_id_str;
  sprintf(array_flag_id_str, "%d", array_flag_id);
  args[4]=array_flag_id_str;
  sprintf(mutex_id_str, "%d", mutex_id);
  args[5]=mutex_id_str;
  sprintf(mu_id_str, "%d", mu_id);
  args[6]=mu_id_str;
  sprintf(pid_pos_str, "%d", pid_pos_id);
  args[7]=pid_pos_str;
  sprintf(round_mutex_str, "%d", round_mutex_id);
  args[9]=round_mutex_str;

  for(i=0;i<SO_NUM_G;i++){
    fork_value=fork();
    if (fork_value==-1){
        TEST_ERROR;
    }else{
      if (fork_value==0){
        sprintf(num_player_str, "%d", i);
        args[2]=num_player_str;

        sprintf(q_id_str, "%d", queue_of_player);
        args[8]=q_id_str;
        printf("\nMASTER created PLAYER (%d) M_ID %d ... QUEUE_ID %d ... SEM_FLAG_MEM_ID %d... PID_POS_ID  ... MUTEX_ID %d.. \n",getpid(),mem_id, queue_of_player, sem_flag_id, mutex_id);
        if(execv(args[0], args) == -1){
          perror("ERROR fork error\n");
        }
        TEST_ERROR;
      }else{
        /*printf("pid = %d\n",fork_value);*/
        pid[i]=fork_value;
        msgRcv(MSG_PLACED,queue_of_player);
      }
    }
  }
  return pid;
}

int checkArray(struct flagPos *array_flag, int flag_pos_captured){
  int i;
  for(i=0;i<array_flag->length; i++){
    if(array_flag[i].flag_pos == flag_pos_captured){
      return i;
    } 
  }
}

int main(int argc, char * argv[]){
  struct chessboardBox chessboard[SIZE];
  struct pidPos pid_pos_array[SO_NUM_P*SO_NUM_G];
  struct capturedFlagMatrix *captured_flag_matrix;
  struct msg_m_p msg_queue;
  pid_t *pid_players, pid_p[SO_NUM_P];
  struct flagPos *array_flag;
  int arr[6]; /*array of ID*/
  int * ret = init(arr,chessboard, pid_pos_array);
  int mem_id=ret[0];
  int sem_flag_id=ret[1];
  int round_counter=1;
  int i=0,j;
  int pid_player,num_of_captured_flags[SO_NUM_G];
  int value_of_flags;
  int array_flag_id=ret[2];
  int round_mutex_id=ret[5];
  int sem_flag;
  int flag_count;
  int num_of_total_flags;
  int q_id_master_player;
  int score[SO_NUM_G];
  int tot_score[SO_NUM_G];
  int number_of_flags=5;
  int condition =true;
  int pos;
  int id_of_flag_captured_matrix;
  int mu_id=semget(IPC_PRIVATE,sizeof(int),0600);
  char * id_of_flag_captured_matrix_str;
  TEST_ERROR;
  semctl(mu_id,0,SETVAL,1);
  
  array_flag=shmat(array_flag_id,NULL,0);
  TEST_ERROR;

  q_id_master_player=initQueue();


  /* forking new players and saving their pids */
  pid_players=playerCreation(ret,pid_p,mu_id,q_id_master_player); /* & pawns & pawns'placement*/
    
  /*round loop*/
  while(round_counter<=2){

    semctl(sem_flag_id,0,SETVAL,0);
    printf("ID MUTEX ROUND = %d\n",round_mutex_id);
    semctl(round_mutex_id,0,SETVAL,0);
    TEST_ERROR;

    printf("semctl(round_mutex_id,0,GETVAL,0) =%d",semctl(round_mutex_id,0,GETVAL,0));
    printf("\n\n\t\t\t..............Starting of the round %d.................\n\n",round_counter);
    
    /* creating num_of_total_flags flags and placing them on the chessboard */
    num_of_total_flags=0; value_of_flags=0;
    num_of_total_flags=numberFlagCreator(sem_flag_id);
    value_of_flags=flagPlacer(mem_id,sem_flag_id,array_flag_id);

    /*printState(mem_id);*/

    /* sending message to the PLAYERS */
    for(i=0;i<SO_NUM_G;i++){
      msgSnd(q_id_master_player,MSG_FLAG,"MASTER said: all flags placed!!");
    }
    
    /* receiving message that all the pawns of all the players know where to go from all the PLAYERS */
    for(i=0;i<SO_NUM_G;i++){
      msgRcv(OK_IND,q_id_master_player);
    } 

    /* only when all the indications are ok we set the TIMER */
    setTimeout(f, SO_MAX_TIME, 0, 1);
    
    /* sending message to all the players that the round is started */
    for(i=0;i<SO_NUM_G;i++){
      msgSnd(q_id_master_player,OK_MOVE,"msg from MASTER: u could start moving");
      /*printf("\nMASTER SENT MESSAGE i(%d) on queue (%d)\n",i,q_id);*/
    }

    /* waiting to receive the message of the captured flag */

    number_of_flags=num_of_total_flags;
    for(i=0;i<SO_NUM_G;i++){
      num_of_captured_flags[i]=0;
      score[i]=0;
    }
    
    for(j=0;j<num_of_total_flags;j++){
      pid_player=msgRcvFlagP(q_id_master_player,PLAYER_PID);
      TEST_ERROR;
      for(i=0;i<SO_NUM_G;i++){
        if(pid_player==pid_players[i]){
          num_of_captured_flags[i]++;
        }
      }
    }
    for(i=0;i<array_flag->length;i++){
      printf("MASTER: array_flag[%d].flag_pos=%d\n",i,array_flag[i].flag_pos);
    }

    for(i=0;i<SO_NUM_G;i++){
      score[i]=num_of_captured_flags[i]*value_of_flags;
      printf("MASTER: the score for the player %d is %d!!!!\n",i+1,score[i]);
    }
    
    /*msg_print_stats(1, q_id_master_player[j]);*/


    /*printState(mem_id);*/

    printf("\n...END of the round %d...\n",round_counter);
    V;
    semop(round_mutex_id,&sops,1);
    printf("semctl(round_mutex_id,0,GETVAL,0) = %d\n",semctl(round_mutex_id,0,GETVAL,0));
    printf("the score for this round is: \n");
    for(i=0;i<SO_NUM_G;i++){
      printf("PLAYER(%d) the score is: %d => %d flags captured\n",pid_players[i],score[i],num_of_captured_flags[i]);
      tot_score[i]=tot_score[i]+score[i];
    }
      
    round_counter++;
    
  }

}