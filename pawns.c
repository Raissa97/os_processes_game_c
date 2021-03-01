#include "global.h"
#include "master.h"

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
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

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
  int queue_id_p_p;
  /*queue_id_p_p = msgget(, IPC_CREAT | 0600);/* master - player - pawn*/
  QUEUE_ERROR

  return queue_id_p_p;
}

void printState(struct chessboardBox *chessboard){

  int rows, cols, i, j, k, sem_id;

  printf("\nPrinting chessboard in sharedmemory \nval_flag,flag-sem_pawn-player\t1=free,0=occupied");
  printf("\n");

  cols = SO_BASE;
  rows = SO_ALTEZZA;

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

void msgSnd(int q_id,int msg_type,char msg_text[MSG_MAX_SIZE]){
  int i;
  struct msg_m_p my_msg;
    
  /*printf("PAWN(%d) Q_ID = %d\n",getpid(),q_id);*/
  /* Constructing the message */
  my_msg.mtype = msg_type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mtext[i] = msg_text[i];
  }
  /* now sending the message */
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  MSGSND_ERROR
  printf("msg from PAWN sent: %s\n",my_msg.mtext);
}

void msgSndP(int q_id,int msg_type, int pos){
  int i;
  struct msg_pos my_msg;

  /* Constructing the message */
  my_msg.mtype = msg_type;
  for(i=0;i<MSG_MAX_SIZE;i++){
    my_msg.mpos[i] = pos;
  }

  /* now sending the message */
  msgsnd(q_id, &my_msg, MSG_MAX_SIZE, 0);
  printf("\t\t PAWN (%ld) has captured the flag in \"%d\" and sent msg to the player\n",(long)getpid(),my_msg.mpos[0]);
}

int msgRcv(int q_id,int rcv_type){
  struct msg_p_p my_msg;
  int num_bytes;
  int ret = false;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if (num_bytes >= 0) {
    printf("\nPAWN (%d) received the msg: Q_id=%d: msg type=%ld \"%s \" RECEIVED\n", getpid(), q_id, my_msg.mtype, my_msg.mtext);
    ret = true;
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

int msgRcv2(int q_id,int rcv_type){
  struct msg_pos my_msg;
  int num_bytes;
  int ret=false;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_type, 0);

  if (num_bytes >= 0) {
    ret = true;
  }

  if (errno == EINTR) {
    fprintf(stderr, "(PID=%d): interrupted by a signal while waiting for a message for pid %d on Q_ID=%d. Trying again\n",
      getpid(), rcv_type, q_id);
  }
  if (errno == EIDRM) {
    printf("The Q_ID=%d was removed. Let's terminate\n", q_id);
    exit(0);
  }
  if (errno == ENOMSG) {
    printf("No message for pid =%d in Q_ID=%d. Not going to wait\n", rcv_type, q_id);
    exit(0);
  }

  return ret;
}

int msgRcvPos(int q_id,int rcv_pid){
  struct msg_pos my_msg;
  int num_bytes;
  int new_pos=0;

  num_bytes = msgrcv(q_id, &my_msg, MSG_MAX_SIZE, rcv_pid, 0);

  if (num_bytes >= 0) {
    printf("\tPAWN received the msg: (PID=%d): Q_id=%d: PID=%ld \"%d\" RECEIVED\n", getpid(), q_id,my_msg.mtype,my_msg.mpos[0]);
    new_pos = my_msg.mpos[0];
  }

  if (errno == EINTR) {
    fprintf(stderr, "(PID=%d): interrupted by a signal while waiting for a message for pid %d on Q_ID=%d. Trying again\n",
      getpid(), rcv_pid, q_id);
  }
  if (errno == EIDRM) {
    printf("The Q_ID=%d was removed. Let's terminate\n", q_id);
    exit(0);
  }
  if (errno == ENOMSG) {
    printf("No message for pid =%d in Q_ID=%d. Not going to wait\n", rcv_pid, q_id);
    exit(0);
  }

  return new_pos;
}

int howManyFlags(struct flagPos *array_flag){
  int i,captured_flags_counter=0;
  
  /*printf("PAWN(%d) howMany captured Flags???? ",getpid());*/
  for(i=0;i<array_flag->length;i++){
    if(array_flag[i].flag_pos==-1){
      captured_flags_counter++;
      /*printf("array_flag[%d].flag_pos=%d\t",i,array_flag[i].flag_pos);*/
    }
    /*printf("number of captured flags = %d\n",captured_flags_counter);*/
  }
  /*printf(" = %d\n",captured_flags_counter);*/
  if(captured_flags_counter==array_flag->length){
    return true;
  }
  else{
    return false;
  }
}

void matrixMovement(int m_id, int q_id,  int mutex_id, int flagPos_id, int num_player, int num_pawn,int actual_position, int destination, int round_mutex_id){
  struct chessboardBox *chessboard = shmat(m_id,NULL,0);
  int riga_p,colonna_p,riga_f,colonna_f;
  int num_mosse=0;
  int pos=actual_position;
  int f_pos;
  int sem_id,new_id;
  int old_pos=-1;
  int changed_destination=true;
  int rand_pos,i,j;
  int new_destination;
  int block_counter=0;
  int captured_flags_counter=false;
  struct flagPos *array_flag=shmat(flagPos_id,NULL,0);
  TEST_ERROR;
  f_pos=destination;
  
  captured_flags_counter=howManyFlags(array_flag);

  /*printf("num di flags = %d \n",num_chessboard_flags);
  printf("\nPAWN(%d): indications received! im starting to move! there are %d flags\n",getpid(),semctl(sem_flag_mem_id,0,GETVAL,0));*/
  riga_p=pos/SO_BASE;
  if(pos==0){
    colonna_p=pos;
  }else{
    colonna_p=pos%SO_BASE;
  }

  while(pos>=0 && pos<=SIZE && f_pos>=0 && f_pos<=SIZE && num_mosse<SO_N_MOVES && block_counter<SO_N_MOVES && captured_flags_counter==false && f_pos!=pos && semctl(round_mutex_id,0,GETVAL,0)==0){   
    
    changed_destination=true;
    
    riga_f=f_pos/SO_BASE;
    if(f_pos==0){
      colonna_f=f_pos;
    }else{
      colonna_f=f_pos%SO_BASE;
    }
    /*printf("semctl(round_counter_id,0,GETVAL,0) = %d\n",semctl(round_mutex_id,0,GETVAL,0));
    printf("\n\nPAWN(%d): OLD pos %d | ACTUAL pos [%d][%d]=%d | NEXT pos[%d][%d]=%d | semctl(round_mutex_id,0,GETVAL,0) = %d\t",getpid(),old_pos,riga_p,colonna_p,pos,riga_f,colonna_f,f_pos,semctl(round_mutex_id,0,GETVAL,0));*/

    while(riga_p-riga_f>0 && pos-SO_BASE>=0 && pos-SO_BASE<=SIZE && changed_destination==true){/* su */
      sem_id=chessboard[pos].sem_pawn_id;
      /*printf("PAWN(%d): my pos[%d] --next pos-->[%d]--sem id->[%d]--sem val--[%d]\t",getpid(), pos, pos-SO_BASE, chessboard[pos-SO_BASE].sem_pawn_id,semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0));*/
      if(semctl(chessboard[pos-SO_BASE].sem_pawn_id,0,GETVAL,0) == -1){
        printf("PAWN(%d): ERROR", getpid());
      }
      if(semctl(chessboard[pos-SO_BASE].sem_pawn_id,0,GETVAL,0)==1){
        new_id=chessboard[pos-SO_BASE].sem_pawn_id;
        /*printf("|PAWN %d|(%d) [%d][%d]  --->   ",getpid(),pos,riga_p,colonna_p);*/
        
        V; /* +1 al sem_id */
        semop(sem_id,&sops,1);
        chessboard[pos].player=0;

        P; /* -1 al new_sem_id */
        semop(new_id,&sops,1);

        chessboard[pos-SO_BASE].player=num_player+1;
        old_pos=pos;
        pos=pos-SO_BASE;

        num_mosse++;

        riga_p=pos/SO_BASE;
        colonna_p=pos%SO_BASE;
        /*printf("[%d][%d] (%d) is there a flag? %d\t",riga_p,colonna_p,pos,chessboard[pos].flag);      */
        
        /*printf("PAWN(%d) num mossa: %d\n",getpid(),num_mosse);*/
        nanosleep(0,SO_MIN_HOLD_NSEC);
        if(chessboard[pos].flag==0){
          /*printf("\nFLAGS ====%d\n",semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("flagPos_id==%d\n",flagPos_id);*/
          /*printf("PRIMA MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          
          P;
          semop(mutex_id,&sops,1);
          W;
          semop(mutex_id,&sops,1);
          /*printf("DENTRO MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          semop(mutex_id,&sops,1);
          chessboard[pos].flag=1;
          chessboard[pos].flag_val=0;
          for(i=0;i<array_flag->length;i++){
            if(array_flag[i].flag_pos==pos){
              array_flag[i].flag_pos=-1;
            }
          }          
          msgSndP(q_id,CAPTURED,pos);
          V;
          semop(mutex_id,&sops,1);
          
          /*printf("!!!!!!!!!!!!!!!PAWN(%d) ho catturato la flag in %d. numero mosse: %d. mancano %d flags!!!!!!!!!!!!!!!!!!!!!!!\n",getpid(),pos,num_mosse,semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("DOPO MUTEX %d",semctl(mutex_id,0,GETVAL,0));
          f_pos=changeDestination(m_id,flagPos_id,pos,old_pos);*/
          
          rand_pos = rand()%array_flag->length;
          printf("rand_pos = %d\t",rand_pos);
          if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
            printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);
            new_destination=array_flag[rand_pos].flag_pos;
            printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);
            f_pos=new_destination;
          }
          
          changed_destination=false;
          
          nanosleep(0,SO_MIN_HOLD_NSEC);

        }   
      }
      if(semctl(chessboard[pos-SO_BASE].sem_pawn_id,0,GETVAL,0)==0){
        /*printf("|PAWN %d| I got stuck in %d\n",getpid(),pos);*/
        /*printf("PAWN(%d) is changing destination\t",getpid());*/
        block_counter++;
        rand_pos = rand()%array_flag->length;
        /*printf("rand_pos = %d\t",rand_pos);*/
        if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
          new_destination=array_flag[rand_pos].flag_pos;
          f_pos=new_destination;
        }
        
        changed_destination=false;
      }
    }
  
    while(riga_p-riga_f<0 && pos+SO_BASE>=0 && pos+SO_BASE<=SIZE && changed_destination==true){/* giu */
      sem_id=chessboard[pos].sem_pawn_id;
      /*printf("PAWN(%d): my pos[%d] --next pos-->[%d]--sem id->[%d]--sem val--[%d]\t",getpid(), pos, pos+SO_BASE, chessboard[pos+SO_BASE].sem_pawn_id,semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0));*/
      if(semctl(chessboard[pos+SO_BASE].sem_pawn_id,0,GETVAL,0) == -1){
        printf("PAWN(%d): ERROR", getpid());
      }
      if(semctl(chessboard[pos+SO_BASE].sem_pawn_id,0,GETVAL,0)==1){
        new_id=chessboard[pos+SO_BASE].sem_pawn_id;
        /*printf("|PAWN %d|(%d) [%d][%d]  --->   ",getpid(),pos,riga_p,colonna_p);*/
         
        V;
        semop(sem_id,&sops,1);
        chessboard[pos].player=0;

        P;
        semop(new_id,&sops,1);

        chessboard[pos+SO_BASE].player=num_player+1;
        old_pos=pos;
        pos=pos+SO_BASE;

        num_mosse++;

        riga_p=pos/SO_BASE;
        colonna_p=pos%SO_BASE;
      
        /*printf("[%d][%d] (%d) is there a flag? %d\t",riga_p,colonna_p,pos,chessboard[pos].flag);     */
        
        /*printf("PAWN(%d) num mossa: %d\n",getpid(),num_mosse);*/
        nanosleep(0,SO_MIN_HOLD_NSEC);  
        if(chessboard[pos].flag==0){
          /*printf("\nFLAGS ====%d\n",semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("flagPos_id==%d\n",flagPos_id);*/
          /*printf("PRIMA MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          
          P;
          semop(mutex_id,&sops,1);
          W;
          semop(mutex_id,&sops,1);
          /*printf("DENTRO MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          semop(mutex_id,&sops,1);
          chessboard[pos].flag=1;
          chessboard[pos].flag_val=0;
          for(i=0;i<array_flag->length;i++){
            if(array_flag[i].flag_pos==pos){
              array_flag[i].flag_pos=-1;
            }
          }   
          msgSndP(q_id,CAPTURED,pos);
          V;
          semop(mutex_id,&sops,1);
          
          /*printf("!!!!!!!!!!!!!!!PAWN(%d) ho catturato la flag in %d. numero mosse: %d. mancano %d flags!!!!!!!!!!!!!!!!!!!!!!!\n",getpid(),pos,num_mosse,semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("DOPO MUTEX %d",semctl(mutex_id,0,GETVAL,0));
          f_pos=changeDestination(m_id,flagPos_id,pos,old_pos);*/
          
          rand_pos = rand()%array_flag->length;
          /*printf("rand_pos = %d\t",rand_pos);*/
          if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
            /*printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);*/
            new_destination=array_flag[rand_pos].flag_pos;
            /*printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);*/
            f_pos=new_destination;
          }
          
          changed_destination=false;
          
          nanosleep(0,SO_MIN_HOLD_NSEC); 
        }         
      }
      if(semctl(chessboard[pos+SO_BASE].sem_pawn_id,0,GETVAL,0)==0){
        /*printf("|PAWN %d| I got stuck in %d\n",getpid(),pos);*/
        /*printf("PAWN(%d) is changing destination\t",getpid());*/
        block_counter++;
        rand_pos = rand()%array_flag->length;
        /*printf("rand_pos = %d\t",rand_pos);*/
        if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
          new_destination=array_flag[rand_pos].flag_pos;
          f_pos=new_destination;
        }
        
        changed_destination=false;
      }
    }

    while(colonna_p-colonna_f>0 && pos-1>=0 && pos-1<=SIZE && changed_destination==true){/* sx */
      sem_id=chessboard[pos].sem_pawn_id;
      /*printf("PAWN(%d): my pos[%d] --next pos-->[%d]--sem id->[%d]--sem val--[%d]\t",getpid(), pos, pos-1, chessboard[pos-1].sem_pawn_id,semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0));*/
      if(semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0) == -1){
        printf("PAWN(%d): ERROR", getpid());
      }
      if(semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0)==1){
        new_id=chessboard[pos-1].sem_pawn_id;
        /*printf("|PAWN %d|(%d) [%d][%d]  --->   ",getpid(),pos,riga_p,colonna_p);*/
         
         V;
        semop(sem_id,&sops,1);
        chessboard[pos].player=0;

        P;
        semop(new_id,&sops,1);
 
        chessboard[pos-1].player=num_player+1;
        old_pos=pos;
        pos=pos-1;

        num_mosse++;

        riga_p=pos/SO_BASE;
        colonna_p=pos%SO_BASE;


        /*printState(m_id);*/

        /*printf("[%d][%d] (%d) is there a flag? %d\t",riga_p,colonna_p,pos,chessboard[pos].flag);   */
        
        /*printf("PAWN(%d) num mossa: %d\n",getpid(),num_mosse);*/
        nanosleep(0,SO_MIN_HOLD_NSEC);     
        if(chessboard[pos].flag==0){
          /*printf("\nFLAGS ====%d\n",semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("flagPos_id==%d\n",flagPos_id);*/
          /*printf("PRIMA MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          
          P;
          semop(mutex_id,&sops,1);
          W;
          semop(mutex_id,&sops,1);
          /*printf("DENTRO MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          semop(mutex_id,&sops,1);
          chessboard[pos].flag=1;
          chessboard[pos].flag_val=0;
          for(i=0;i<array_flag->length;i++){
            if(array_flag[i].flag_pos==pos){
              array_flag[i].flag_pos=-1;
            }
          }   
          msgSndP(q_id,CAPTURED,pos);
          V;
          semop(mutex_id,&sops,1);
          
          /*printf("!!!!!!!!!!!!!!!PAWN(%d) ho catturato la flag in %d. numero mosse: %d. mancano %d flags!!!!!!!!!!!!!!!!!!!!!!!\n",getpid(),pos,num_mosse,semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("DOPO MUTEX %d",semctl(mutex_id,0,GETVAL,0));
          f_pos=changeDestination(m_id,flagPos_id,pos,old_pos);*/
          
          rand_pos = rand()%array_flag->length;
          /*printf("rand_pos = %d\t",rand_pos);*/
          if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
            /*printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);*/
            new_destination=array_flag[rand_pos].flag_pos;
            /*printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);*/ 
            f_pos=new_destination;
          }
         
          changed_destination=false;
          
          nanosleep(0,SO_MIN_HOLD_NSEC);     
        }      
      }
      if(semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0)==0){
        /*printf("|PAWN %d| I got stuck in %d\n",getpid(),pos);*/
        /*printf("PAWN(%d) is changing destination\t",getpid());*/
        block_counter++;
        rand_pos = rand()%array_flag->length;
        /*printf("rand_pos = %d\t",rand_pos);*/
        if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
          /*printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);*/
          new_destination=array_flag[rand_pos].flag_pos;
          /*printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);*/
          f_pos=new_destination;
        }
        
        changed_destination=false;
      }
    }

    while(colonna_p-colonna_f<0 && pos+1>=0 && pos+1<=SIZE && changed_destination==true){/* dx */
      sem_id=chessboard[pos].sem_pawn_id;
      /*printf("PAWN(%d): my pos[%d] --next pos-->[%d]--sem id->[%d]--sem val--[%d]\t",getpid(), pos, pos+1, chessboard[pos+1].sem_pawn_id,semctl(chessboard[pos-1].sem_pawn_id,0,GETVAL,0));*/
      if(semctl(chessboard[pos+1].sem_pawn_id,0,GETVAL,0)==-1){
        printf("PAWN(%d): ERROR", getpid());
      }
      if(semctl(chessboard[pos+1].sem_pawn_id,0,GETVAL,0)==1){
        new_id=chessboard[pos+1].sem_pawn_id;
        /*printf("|PAWN %d|(%d) [%d][%d]  --->   ",getpid(),pos,riga_p,colonna_p);*/
        
        V;
        semop(sem_id,&sops,1);
        chessboard[pos].player=0;

        P;
        semop(new_id,&sops,1);

        chessboard[pos+1].player=num_player+1;
        old_pos=pos;
        pos=pos+1;

        num_mosse++;

        riga_p=pos/SO_BASE;
        colonna_p=pos%SO_BASE;

        /*printState(m_id);*/
        /*printf("[%d][%d] (%d) is there a flag? %d\t",riga_p,colonna_p,pos,chessboard[pos].flag);    */
        /*printf("PAWN(%d) num mossa: %d\n",getpid(),num_mosse);*/
        nanosleep(0,SO_MIN_HOLD_NSEC);       
        if(chessboard[pos].flag==0){
          /*printf("\nFLAGS ====%d\n",semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("flagPos_id==%d\n",flagPos_id);*/
          /*printf("PRIMA MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          
          P;
          semop(mutex_id,&sops,1);
          W;
          semop(mutex_id,&sops,1);
          /*printf("DENTRO MUTEX %d",semctl(mutex_id,0,GETVAL,0));*/
          semop(mutex_id,&sops,1);
          chessboard[pos].flag=1;
          chessboard[pos].flag_val=0;
          for(i=0;i<array_flag->length;i++){
            if(array_flag[i].flag_pos==pos){
              array_flag[i].flag_pos=-1;
            }
          }   
          msgSndP(q_id,CAPTURED,pos);
          V;
          semop(mutex_id,&sops,1);
          
          /*printf("!!!!!!!!!!!!!!!PAWN(%d) ho catturato la flag in %d. numero mosse: %d. mancano %d flags!!!!!!!!!!!!!!!!!!!!!!!\n",getpid(),pos,num_mosse,semctl(sem_flag_mem_id,0,GETVAL,0));
          printf("DOPO MUTEX %d",semctl(mutex_id,0,GETVAL,0));
          f_pos=changeDestination(m_id,flagPos_id,pos,old_pos);*/
          
          rand_pos = rand()%array_flag->length;
          /*printf("rand_pos = %d\t",rand_pos);*/
          if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
            /*printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);*/
            new_destination=array_flag[rand_pos].flag_pos;
            /*printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);*/
            f_pos=new_destination;
          }
          
          changed_destination=false;
          
          nanosleep(0,SO_MIN_HOLD_NSEC);    
        }   
      }
      if(semctl(chessboard[pos+1].sem_pawn_id,0,GETVAL,0)==0){
        /*printf("|PAWN %d| I got stuck in %d\n",getpid(),pos);*/
        /*printf("PAWN(%d) is changing destination\t",getpid());*/
        block_counter++;
        rand_pos = rand()%array_flag->length;
        /*printf("rand_pos = %d\t",rand_pos);*/
        if(array_flag[rand_pos].flag_pos!=-1 && array_flag[rand_pos].flag_pos!=old_pos){
          /*printf("IN THE IF: array_flag[rand_pos].flag_pos = %d\t",array_flag[rand_pos].flag_pos);*/
          new_destination=array_flag[rand_pos].flag_pos;
          /*printf("PAWN(%d): updated table of moves with NEW DESTINATION %d\n",getpid(),new_destination);*/
          f_pos=new_destination;
        }
        
        changed_destination=false;
      }
    }
  }
 
}

int main(int argc, char * argv[]){
  int m_id = atoi(argv[1]);
  int q_id, destination, actual_position, i=0;
  int num_player = atoi(argv[2]);
  int num_pawn = atoi(argv[3]);
  int pid_pos_id=atoi(argv[4]);
  int condition=true;
  int mutex_id=atoi(argv[5]);
  int flagPos_id = atoi(argv[6]);
  int initial_position=atoi(argv[8]);
  int round_mutex_id=atoi(argv[9]);
  int num_chessboard_flags;
  int stop;
  char *pid_str;
  char *cmd;
  struct pidPos *pid_pos=shmat(pid_pos_id,NULL,0);
  SHMAT_ERROR;
  /*pawn_position = atoi(argv[1]);*/
  q_id=atoi(argv[7]);
  /*printf("PAWN(%d): m_id_str: %d | num_p_str: %d | pid_pos_id_str %d | mutex_id_str %d | flagPos_str %d | queue_id_str %d \n",getpid(), atoi(argv[1]), atoi(argv[2]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));*/

  /* sending message to the PLAYER */
  printf("PAWN(%d) POSITIONS = %d num = %d\n",getpid(),initial_position,num_pawn);
  msgSnd(q_id,PAWN_INITIAL_POS,"OK INITIAL POSITIONS");
  
  destination=msgRcvPos(q_id,FINAL_DESTINATION);
  /* sending message to the player */
  msgSnd(q_id,PAWN_IND,"pawn has received indications");
  
  /* waiting to receive message "MOVE!!" from PLAYER and starting to move */
  if(msgRcv2(q_id,MOVE)){
    /*printf("PAWN(%d) start to move\n",getpid());*/
    matrixMovement(m_id,q_id,mutex_id,flagPos_id,num_player,num_pawn,initial_position, destination,round_mutex_id);
  }
  printf("PAWN(%d) semctl(round_mutex_id,0,GETVAL,0)=%d\n",getpid(),semctl(round_mutex_id,0,GETVAL,0));
}