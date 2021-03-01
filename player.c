#include <errno.h>
#include <sys/sem.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <signal.h>

#include "global.h"
#include "master.h"

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

int queueInit(){
  int queue_id_pawn_player;
  queue_id_pawn_player=msgget(IPC_PRIVATE,IPC_CREAT|0600);
  TEST_ERROR;
  return queue_id_pawn_player;
}

/* send the final position that the pawn must have */
void msgSndP(int q_id, int type, int pid_pos_id){
  int i;
  struct msg_pos my_msg;
    
  /* Constructing the message */
  my_msg.mtype = type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mpos[i] = pid_pos_id;
  }
  /* now sending the message */ 
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  MSGSND_ERROR;
  /*printf("\t\tplayer (%ld) sent the message \"%d\" in QUEUE_ID=%d \n",(long)getpid(),pid_pos_id,q_id);*/
}

void msgSndId(int q_id, int type, int pawn_pid, int pid_pos_id){
  int i,j;
  struct msg_id my_msg;
    
  /* Constructing the message */
  my_msg.mtype = type;
  for(i=0;i<SO_NUM_P;i++){
    my_msg.id[i][0] = pid_pos_id;
    my_msg.id[i][1] = pawn_pid;  
  }
  /* now sending the message */ 
  printf("\t\tplayer (%ld) sent the message \"%d %d\" \n",(long)getpid(),my_msg.id[0][0],my_msg.id[0][1]);
  
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  
}

/* send a msg text */
void msgSnd(int q_id,int msg_type,char msg_text[MSG_MAX_SIZE]){
  int i;
  struct msg_m_p my_msg;
    
  /* Constructing the message */
  my_msg.mtype = msg_type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mtext[i] = msg_text[i];
  }
  /* now sending the message 
  */
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  MSGSND_ERROR;
  printf("\t\tPLAYER(%d) sent msg: \"%s\"\n",getpid(),my_msg.mtext);
}

void msgSndToPawn(int q_id,int msg_type,char msg_text[MSG_MAX_SIZE]){
  int i;
  struct msg_p_p my_msg;
    
  /* Constructing the message */
  my_msg.mtype = msg_type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mtext[i] = msg_text[i];
  }
  /* now sending the message */
  /*printf("\t\tPLAYER(%d) sent msg: \"%s\"\n",getpid(),my_msg.mtext);*/
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);

}

/* send to the master the position of the flag captured */
void msgSndFlagP(int q_id, long type, int flag_pos){
  int i;
  struct msg_pos my_msg;
    
  /* Constructing the message */
  my_msg.mtype = type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mpos[i] = flag_pos;
  }
  
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  printf("\n\n ------PLAYER(%d) sent msg FlagP in Q_ID=%d flag position=%d ------\n",getpid(),q_id,my_msg.mpos[0]);
}

/* receive this msg when pawn received its indication */
int msgRcv(int q_id, int rcv_type){
  struct msg_m_p my_msg;
  int num_bytes;
  int ret = false;

  num_bytes=msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);
  MSGSND_ERROR;
  if(num_bytes >= 0) {
    printf("\nPLAYER (%d) received the msg: Q_id=%d: msg type=%ld \"%s\" RECEIVED\n", getpid(), q_id, my_msg.mtype, my_msg.mtext);
    ret=true;
  }

  /*printf("%d\n",errno);
   now error handling */
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
  return true;
}

/* receive a message when pawns finish movements */
char * msgRcvText(int q_id,int rcv_type){
  struct msg_m_p my_msg;
  int num_bytes;
  char * ret = "ERROR";

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if (num_bytes >= 0) {
    /*printf("\nPLAYER (%d) received the msg: Q_id=%d: msg type=%ld \"%s\" RECEIVED\n", getpid(), q_id, my_msg.mtype, my_msg.mtext);*/
    ret = my_msg.mtext;
  }

  /*printf("%d\n",errno);
   now error handling */
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
  
  return ret;
}

/* receive the pawn's initial position */
int msgRcvP(int q_id,int rcv_type){
  struct msg_pos my_msg;
  int num_bytes, i;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);
  MSGSND_ERROR;
  if(num_bytes >= 0) {
    for(i=0;i<MSG_MAX_SIZE;i++){
      printf("\n.......PLAYER (%ld) received the msg Q_ID_=%d from pawn msg type=%ld the int is \"%d\" RECEIVED\n", (long)getpid(),q_id,my_msg.mtype,my_msg.mpos[i]);
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

/* receive the position of the flag that the pawn captured */
int msgRcvFlagP(int q_id,int rcv_type){
  struct msg_pos my_msg;
  int num_bytes, i;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if(num_bytes >= 0) {
    for(i=0;i<MSG_MAX_SIZE;i++){
      printf("\n......PLAYER (%ld) received the msg from pawn msg type=%ld the int is \"%d\" RECEIVED\n", (long)getpid(),my_msg.mtype,my_msg.mpos[i]);
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

int pawnPosition(int mem_id, int num_player){
  struct chessboardBox *chessboard= shmat(mem_id, NULL, 0);
  int pawn_position;
  int sem_id;
  int pawn_sem_val;

  pawn_position=rand()%SIZE;
  sem_id = chessboard[pawn_position].sem_pawn_id;
  pawn_sem_val = semctl(sem_id,0,GETVAL,0);
  /*printf("PLAYER (%d): SEM ID %d\n",getpid(),sem_id);
  printf("PLAYER (%d): trying to place the pawn in %d (sem value = %d)\n",getpid(),pawn_position,pawn_sem_val);*/
  if(pawn_sem_val == 0){ /*occupata*/
    /*printf("PLAYER (%d): OCCUPIED! in %d (sem value = %d)\n",getpid(),pawn_position,pawn_sem_val);*/
    pawnPosition(mem_id, num_player);
  } 
  if(pawn_sem_val==1){ 
    P; /* -1 al sem == occupato*/
    semop(sem_id,&sops,1);
    SEMOP_ERROR;
    printf("PAWN (%d): PLACED! in %d (sem value = %d)\n",getpid(),pawn_position,semctl(sem_id,0,GETVAL,0));
    chessboard[pawn_position].player = num_player+1; 
    return pawn_position;
  }
}

void setMovesTable(int *arr_pawns_pid, int *arr_positions, struct pidPos *pid_pos_matrix, int mem_id, int q_id, int num_player){
  int i,j,pawn_position=0;
  for(i=0;i<SO_NUM_P;i++){
    for(j=0; j<SO_NUM_G*SO_NUM_P; j++){     
      if(semctl(pid_pos_matrix[j].pid_pos_array[3], 0, GETVAL, 0)== 1){
        TEST_ERROR;
        printf("trtgrgerergrggrgrerge pid pawn = %d\n",arr_pawns_pid[i]);
        pid_pos_matrix[j].pid_pos_array[0]= arr_pawns_pid[i];
        pid_pos_matrix[j].pid_pos_array[1]= arr_positions[i];
        pid_pos_matrix[j].pid_pos_array[2]= -1;
        arr_pawns_pid[i]=pid_pos_matrix[j].pid_pos_array[0];
        P;
        semop(pid_pos_matrix[j].pid_pos_array[3],&sops,1);
        SEMOP_ERROR;
        printf("SEM VAL di pid_pos_matrix[%d][3]=%d --> val = %d \t",j,pid_pos_matrix[j].pid_pos_array[3],semctl(pid_pos_matrix[j].pid_pos_array[3], 0, GETVAL, 0));
        break;
      }
    }
  }
}

int * pawnsCreation(int *arr_positions, int pid_pos_id, int m_id, int queue_id_player_pawn, int queue_id_master_player, int mutex_id, int num_player, int *arr_pawns_pid, int arr_flag_id, int round_mutex_id){
  int i, j;
  pid_t fork_value;
  int pawn_position[SO_NUM_P];
  char pawn_position_str[3*sizeof(int)+1]; /* sufficiente per stringa m_id */
  char m_id_str[3*sizeof(int)+1];
  char num_p_str[3*sizeof(int)+1];
  char num_pawn_str[3*sizeof(int)+1];
  char pid_pos_id_str[3*sizeof(int)+1];
  char mutex_id_str[3*sizeof(int)+1];
  char flagPos_str[3*sizeof(int)+1];
  char queue_id_str[3*sizeof(int)+1];
  char round_mutex_id_str[3*sizeof(int)+1];

  char * args[11]={"./pawns", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};  

  sprintf(m_id_str, "%d", m_id);
  args[1]=m_id_str;
 
  sprintf(num_p_str, "%d", num_player);
  args[2]=num_p_str;
  
  sprintf(pid_pos_id_str, "%d", pid_pos_id);
  args[4]=pid_pos_id_str;

  sprintf(mutex_id_str, "%d", mutex_id);
  args[5]=mutex_id_str;
  
  sprintf(flagPos_str, "%d", arr_flag_id);
  args[6]=flagPos_str;

  sprintf(queue_id_str, "%d", queue_id_player_pawn);
  args[7]=queue_id_str;
  
  sprintf(round_mutex_id_str, "%d", round_mutex_id);
  args[9]=round_mutex_id_str;

  /*printf("PLAYER(%d) m_id_str: %s | num_p_str: %s | sem_flag_mem_id_str %s | pid_pos_id_str %s | mutex_id_str %s \n",getpid(), args[1], args[2], args[3], args[5], args[6]);*/
  for(i=0;i<SO_NUM_P;i++){  
    pawn_position[i]=pawnPosition(m_id,num_player);
    fork_value=fork();
    if(fork_value==0){ 
      printf("PLAYER(%d) created PAWN(%d), m_id_str: %s | num_p_str: %s | pid_pos_id_str %s | mutex_id_str %s | flagPos_str %s | queue_id_str %s \n",getppid(),getpid(), args[1], args[2], args[4], args[5], args[6],args[7]);
      
      sprintf(num_pawn_str, "%d", i);
      args[3]=num_pawn_str; 
      
      sprintf(pawn_position_str,"%d",pawn_position[i]);
      args[8]=pawn_position_str;
      
      if(execv(args[0], args)==-1){
        printf("FORK ERROR \n");
      }
      FORK_ERROR;
    }else if (fork_value!=0){
      arr_positions[i]=pawn_position[i];
      printf("!!!!!!!!!!! position = %d in FORK\n",arr_positions[0]);
      arr_pawns_pid[i]=fork_value;
      if(msgRcv(queue_id_player_pawn,PAWN_INITIAL_POS)==true){
        printf("msg [%d] RECEIVED \n",i);
      }
    }else if(fork_value==-1){
      TEST_ERROR;
    }
  } 

  return arr_positions;
}

int newDestination(struct flagPos *array_flag,int actual_position){
  int destination;

  int rand_pos = rand()%array_flag->length;
  if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=actual_position){
    destination=array_flag[rand_pos].flag_pos;
  }

  /*printf("destination = %d\t",destination);*/
  return destination;
}

/* void indications(struct pidPos *pid_pos,int num_p, struct flagPos *array_flag, int *arr_pawns_pid){
  int j=0,i,rand_pos;
 
  for(i=0; i<SO_NUM_P*SO_NUM_G;i++){
    for(j=0; j<SO_NUM_P;j++){
      if(pid_pos[i].pid_pos_array[0]==arr_pawns_pid[j]){
        rand_pos = rand()%array_flag->length;
        pid_pos[i].pid_pos_array[2]=array_flag[rand_pos].flag_pos;
        /*printf("PLAYER(%d): updated table of moves with the first indication %d\n",getpid(),pid_pos[i].pid_pos_array[2]);
      }
    }
  } 
}
        
void newIndications(int mem_id,int pid,struct pidPos *pid_pos,int arr_flag_id, int *arr_pawns_pid, int actual_pos){
  int rand_pos,i,j;
  struct chessboardBox *chessboard= shmat(mem_id, NULL, 0);
  struct flagPos *array_flag=shmat(arr_flag_id,NULL,0);
  TEST_ERROR;
  
  for(i=0; i<SO_NUM_P*SO_NUM_G;i++){
    for(j=0; j<SO_NUM_P;j++){
      if(pid_pos[i].pid_pos_array[0]==arr_pawns_pid[j]){
        pid_pos[i].pid_pos_array[2]=actual_pos;
        rand_pos = rand()%array_flag->length;
        if(array_flag[rand_pos].flag_pos!=-1){
          pid_pos[i].pid_pos_array[2]=array_flag[rand_pos].flag_pos;
          printf("PLAYER(%d): updated table of moves with NEW indication %d\n",getpid(),pid_pos[i].pid_pos_array[2]);
        }
      }
    }
  } 
   
  printf("new indications in player OK\n");
}
*/

int main(int argc, char * argv[]){
  int m_id, num_player, i=0,j, sem_flag_mem_id, arr_flag_id;
  struct pidPos *pid_pawn;
  char * txt;
  int flag_captured_pos;
  int sem_flag;
  int pid;
  int mutex_id,mu_id;
  int pid_pos_id;
  int arr_pawns_pid[SO_NUM_P];
  int arr_positions[SO_NUM_P];
  struct pidPos *pid_pos_matrix;
  int *pid_pawns_array;
  int actual_pos;
    /* initialization of the message queue*/
  int queue_id_master_player;
  int queue_id_player_pawn;
  int round_mutex_id;
  int * array_of_pos;
  char * pid_str;
  char * cmd;
  int destination ;
  struct flagPos *array_flag;
  TEST_ERROR;
  m_id = atoi(argv[1]);
  num_player = atoi(argv[2]);
  sem_flag_mem_id=atoi(argv[3]);
  arr_flag_id=atoi(argv[4]);
  mutex_id=atoi(argv[5]);
  mu_id=atoi(argv[6]);
  pid_pos_id=atoi(argv[7]);

  array_flag=shmat(arr_flag_id,NULL,0);
  queue_id_player_pawn=queueInit();
  queue_id_master_player=atoi(argv[8]);
  round_mutex_id=atoi(argv[9]);
  /*printf("PLAYER(%d): queue player-master ID %d, queue player-pawn ID: %d\n",getpid(),queue_id_master_player,queue_id_player_pawn);*/

  pid_pos_matrix = (struct pidPos *) shmat(pid_pos_id,NULL,0);
  TEST_ERROR;
  
  /* creation and positioning of pawns */
  array_of_pos=pawnsCreation(arr_positions,pid_pos_id,m_id,queue_id_player_pawn,queue_id_master_player,mutex_id,num_player, arr_pawns_pid,arr_flag_id,round_mutex_id);

  for(i=0;i<SO_NUM_P;i++){
    printf("PLAYER(%d) pawn(%d) position =%d\n",getpid(), arr_pawns_pid[i], array_of_pos[i]);
  }

  /* set table of moves */
  msgSnd(queue_id_master_player,MSG_PLACED,"I have created and placed my pawns!");
  /*setMovesTable(arr_pawns_pid,array_of_pos,pid_pos_matrix,m_id,queue_id_player_pawn,num_player);*/

  /* waiting to receive msg "the flags are placed" from MASTER */
  if(msgRcv(queue_id_master_player,MSG_FLAG)==true){
    for(i=0;i<SO_NUM_P;i++){
      TEST_ERROR;
      destination=newDestination(array_flag,array_of_pos[i]);
      msgSndP(queue_id_player_pawn,FINAL_DESTINATION,destination);
    }
  }

  /* waiting to receive msg "we have received indications" from PAWNS */
  i=0;
  while(i<SO_NUM_P){
    if(msgRcv(queue_id_player_pawn,PAWN_IND)==true){
      /*printf("!!!!!!!!!!!!PLAYER(%d): received message %d", getpid(), i);*/
      i++;  
    }
  }

  /* sending message to the MASTER */
  msgSnd(queue_id_master_player,OK_IND,"PLAYER said: my pawns know where to go");
  
  /* waiting to receive msg "you could start moving" from MASTER */
  if(msgRcv(queue_id_master_player, OK_MOVE)==true){
    for(i=0;i<SO_NUM_P;i++){
      msgSndToPawn(queue_id_player_pawn,MOVE,"msg from PLAYER: move!");
    }
  }
  
  /* waiting to receive msg about position of captured flag from PAWNS */
  while(true){
    for(i=0;i<SO_NUM_P;i++){
      flag_captured_pos=msgRcvP(queue_id_player_pawn,CAPTURED);
      printf("PLAYER(%d) captured flag position: %d\n",getpid(),flag_captured_pos);
      TEST_ERROR;
      /* sending the position of the captured flag to the MASTER */
      msgSndP(queue_id_master_player,PLAYER_PID,getpid());
      TEST_ERROR;
    }
  }
    /*
    for(i=0;i<semctl(sem_flag_mem_id,0,GETVAL,0);i++){
      while(msgRcvP(queue_id_player_pawn,CAPTURED)!=-1){
        flag_captured_pos=msgRcvP(queue_id_player_pawn,CAPTURED);
        printf("PLAYER(%d) flag %d captured by one of my pawns.\n",getpid(),flag_captured_pos);
        msgSndP(queue_id_master_player,FLAG_P,flag_captured_pos);
        TEST_ERROR;
      }
    }*/
     
    
    /*if(msgRcvP(queue_id_player_pawn,POS_STOP)>=0){
      actual_pos=msgRcvP(queue_id_player_pawn,POS_STOP);
      pid=msgRcvP(queue_id_player_pawn,PAWN_PID);
      for(i=0;i<SO_NUM_P;i++){
        if(pid==arr_pawns_pid[i]){
          printf("\nPLAYER: my pawn (%d) is stacked in %d. Now new indications!!!\t",pid,actual_pos);
          newIndications(m_id,pid,pid_pos_matrix,arr_flag_id,arr_pawns_pid,actual_pos);
        }
      }
    }*/
    /*V;
    semop(mu_id,&sops,1);
  }*/
 
}