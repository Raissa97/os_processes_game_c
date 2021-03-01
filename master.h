#include "global.h"

#define MYKEY 123456

#define MSG_MAX_SIZE 128

/* definition of the type of messages*/
#define PAWN_INITIAL_POS 1
#define MSG_PLACED 2
#define MSG_FLAG 3
#define SET_TABLE 4
#define PAWN_IND 5
#define OK_IND 6
#define OK_MOVE 7
#define MOVE 8
#define CAPTURED 9
#define PLAYER_PID 10
#define FINAL_DESTINATION 11
#define ID_MATRIX 12

#define V      \
  sops.sem_num=0;   \
  sops.sem_op=1;    \
  sops.sem_flg=0;   \

#define P    \
  sops.sem_num=0;   \
  sops.sem_op=-1;    \
  sops.sem_flg=0;   \

#define W       \
  sops.sem_num=0;   \
  sops.sem_op=0;    \
  sops.sem_flg=0;   \

# define SEMOP_ERROR \
  if (errno == E2BIG) { \
  printf("E2BIG"); \
  } \
  if (errno == EACCES) { \
  printf("EACCES"); \
  } \
  if (errno == EAGAIN) { \
    printf("EAGAIN"); \
  }    \
  if (errno == EFAULT) {  \
    printf("EFAULT");\
  }  \
  if (errno == EFBIG) {\
    printf("EFBIG");\
  }  \
  if (errno == EIDRM) {\
    printf("EIDRM");\
  }  \
  if (errno == EINTR) {\
    printf("EINTR");\
  }  \
  if (errno == EINVAL) {\
    printf("EINVAL");\
  }\
  if (errno == ENOMEM) {\
    printf("ENOMEM");\
  }\
  if (errno == ERANGE) {\
    printf("ERANGE");\
  }\

#define SHMAT_ERROR \
if (errno == EACCES) { \
  printf("EACCES"); \
  } \
  if (errno == EIDRM) { \
  printf("EIDRM"); \
  } \
  if (errno == EINVAL) { \
    printf("EINVAL"); \
  }    \
  if (errno == ENOMEM) {  \
    printf("ENOMEM");\
  }  \

#define QUEUE_ERROR \
if (errno == EACCES) { \
  printf("EACCES"); \
  } \
  if (errno == EEXIST) { \
  printf("EEXIST"); \
  } \
  if (errno == ENOENT) { \
    printf("ENOENT"); \
  }    \
  if (errno == ENOMEM) {  \
    printf("ENOMEM");\
  }  \
  if (errno == ENOSPC) {  \
    printf("ENOSPC");\
  }  \

# define FORK_ERROR \
  if (errno == EACCES) { \
  printf("EACCES"); \
  } \
  if (errno == EFAULT) { \
  printf("EFAULT"); \
  } \
  if (errno == EINVAL) { \
    printf("EINVAL"); \
  }    \
  if (errno == EIO) {  \
    printf("EIO");\
  }  \
  if (errno == EISDIR) {\
    printf("EISDIR");\
  }  \
  if (errno == ELIBBAD) {\
    printf("ELIBBAD");\
  }  \
  if (errno == ELOOP) {\
    printf("ELOOP");\
  }  \
  if (errno == EMFILE) {\
    printf("EMFILE");\
  }\
  if (errno == ENAMETOOLONG) {\
    printf("ENAMETOOLONG");\
  }\
  if (errno == ENFILE) {\
    printf("ENFILE");\
  }\
  if (errno == ENOENT) {\
    printf("ENOENT");\
  }\
  if (errno == ENOEXEC) {\
    printf("ENOEXEC");\
  }\
  if (errno == ENOMEM) {\
    printf("ENOMEM");\
  }\
  if (errno == ENOTDIR) {\
    printf("ENOTDIR");\
  }\
  if (errno == EPERM) {\
    printf("EPERM");\
  }\
  if (errno == ETXTBSY) {\
    printf("ETXTBSY");\
  }\


# define MSGSND_ERROR \
  if (errno == EACCES) { \
  printf("EACCES"); \
  } \
  if (errno == EAGAIN) { \
  printf("EAGAIN"); \
  } \
  if (errno == EIDRM) { \
    printf("EIDRM"); \
  }    \
  if (errno == EINTR) {  \
    printf("EINTR");\
  }  \
  if (errno == EINVAL) {\
    printf("EINVAL");\
  }  \
  if (errno == ENOMEM) {\
    printf("ENOMEM");\
  }  \
  if (errno == E2BIG) {\
    printf("E2BIG");\
  }  \
  if (errno == EFAULT) {\
    printf("EFAULT");\
  }\
  if (errno == EIDRM) {\
    printf("EIDRM");\
  }\
  if (errno == EINTR ) {\
    printf("EINTR");\
  }\
  if (errno == ENOENT) {\
    printf("ENOENT");\
  }\
  if (errno == ENOMSG) {\
    printf("ENOMSG");\
  }\


struct sembuf sops;

struct chessboardBox{
  int flag;
  int flag_val;
  int sem_pawn_id;
  int player;
};

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short  *array;
};

struct msg_m_p{
  long mtype;                       /* type of message */
  char mtext[MSG_MAX_SIZE];         /* user-define message */
};
struct msg_p_p{
  long mtype;                       /* type of message */
  char mtext[MSG_MAX_SIZE];         /* user-define message */
};

struct msg_pos{
  long mtype;
  int mpos[MSG_MAX_SIZE];
};

struct pidPos{
  int pid_pos_array[4]; /* pawn's PID, position & new position */
};

struct flagPos{
  int length;
  int flag_pos;
};

struct msg_id{
  long mtype;
  int id[SO_NUM_P][2];
};

struct capturedFlagMatrix{
  int array[3]; /* player's PID, position of captured flag & sem */
};

struct msg_pawn_pos{
  long mtype;
  int p_pos[MSG_MAX_SIZE][SO_NUM_P];
};
