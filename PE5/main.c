/*PE5 InterProcessCommunication*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

/* Initalization */
char* generate_data(size_t size);
int unnamed_pipe(size_t size);
int named_pipe(size_t size);


enum {READ = 0, WRITE = 1};
unsigned long long int cumulative_bytes_read = 0;   /* cumulative number of bytes read - For task a.*/
unsigned long int bytes_read = 0;                   /* Bytes read since the last alarm.*/

// Signal handler
void sig_handler_B(int signum){
    // TASK B 
    printf("Bandwidth: %li\n", bytes_read);
    bytes_read = 0; // Reset for next second.
    alarm(1);
}

void sig_handler_C(int signum){
    // TASK C 
    printf("=====================================\n");
    printf("Total number of bytes sent so far: %llu\n", cumulative_bytes_read);
    printf("=====================================\n");
}

int main(int argc, char *argv[]){
    signal(SIGALRM, sig_handler_B); // Register signal handler for handling alarms
    signal(SIGUSR1, sig_handler_C);  // Signal handler for handling kill - 
    alarm(1);  // Scheduled the first alarm after 1 seconds
    
    size_t size = atoi(argv[1]);    // Byte size of each package/block
    
    // For testing task A, you need to uncomment marked code in unnamed_pipe()
    int status = unnamed_pipe(size);  /* For task A, B and C */
    //int status = named_pipe(size);      /* For task D */    
    
    ////////////////////////////////
    //////////// TASK B ////////////
    ////////////////////////////////
    /* Results of B block size testing */
    // Size (bytes)     ~   Bandwidth (unnamed)
    // 1                ~   4.000.000
    // 10               ~   25.000.000
    // 100              ~   160.000.000
    // 1.000            ~   215.000.000
    // 10.000           ~   250.000.000
    // 100.000          ~   280.000.000   ! Maximum bandwidth achieved on our system.
    // 1.000.000        ~   234.000.000
    // 10.000.000       ~   240.000.000
    // 100.000.000      ~   200.000.000   ! Largest block size supported by the system, somewhere in the range [100.000.000, 1.000.000.000]
    // 1.000.000.000    ~   0             ! One package is too big to be sent in 1 sec, still transfers data, just not every second.
    // 10.000.000.000   ~   0             
    /* Running multiple instances of the program ==> Drastically! reduced bandwidth.*/
    
    ////////////////////////////////
    //////////// TASK D ////////////
    ////////////////////////////////
    /* Results of D block size testing */
    // Size (bytes)     ~   Bandwidth (named)
    // 1                ~   2.000.000
    // 10               ~   27.000.000
    // 100              ~   130.000.600
    // 1.000            ~   172.000.000
    // 10.000           ~   245.000.000
    // 100.000          ~   282.000.000          ! Maximum bandwidth achieved on our system.
    // 1.000.000        ~   245.000.000
    // 10.000.000       ~   260.000.000
    // 100.000.000      ~   200.000.000   
    // 1.000.000.000    ~   0 ! One package is too big to be sent in 1 sec, still transfers data, just not every second.          
    // 10.000.000.000   ~   0            
    /* Running multiple instances of the program ==> Drastically! reduced bandwith.*/
    
    return 0;
}
// Establish unnamed pipe and read/write as fast as possible through the pipe.
int unnamed_pipe(size_t size){
    /*Print the parent process pid, to use in task C*/
    printf("================================\n");
    printf("UNNAMED PIPE\n");
    printf("================================\n");
    printf("Parent process PID: %i\n",getpid());
    printf("================================\n");
    /* Create Pipe and fork the process, establish communication */
    int res, fd[2]; /* child PID and descriptor */
    if (pipe (fd) == 0) {                   /* create the pipe */
        res = fork ();                      /* pipe created successfully*/
        if (res > 0) {                      /* parent process (Supposed to read)*/
            close (fd[WRITE]);              /* close writing side */
            dup2 (fd[READ], 0);             /* redirect stdin from pipe */
            int r = 0;                      /* status of read */
            char *buff = malloc(size);
            while(r != -1){
                r = read(fd[READ], buff, size);          /* Listen to the pipe */
                if (r == -1){
                    perror("ERROR: read failed");
                    return -1;
                }
                bytes_read += (int)size;
                cumulative_bytes_read += (int)size;
                /*We never use buff, and overwrite it each read as it is just fille with trash...*/
                /** TASK A
                 * Uncomment to see cumulative number of received bytes
                 */
                //printf("Bytes read: %llu\n", cumulative_bytes_read);
            }
            close (fd[READ]);                       /* release the descriptor */
            free(buff); // Free the trash!
        }
        else if (res == 0) {        /* child process (Supposed to write endlessly)*/
            close (fd[READ]);       /* close reading side */
            dup2 (fd[WRITE], 1);    /* redirect stdout to pipe */
            int w = 0;
            char *dummy_data = generate_data(size);
            while(w!=-1){
                w = write(fd[WRITE], dummy_data, size);
                if (w == -1){
                    perror("ERROR: write failed");
                    return -1;
                }
            }
            close (fd[WRITE]);      /* release the descriptor */
            free(dummy_data);
        }
        else if (res < 0){          /* if the forking failed*/
            perror("ERROR: Fork failed. \n");
            // Kill the faulty process, with exit signal EXIT_FAILURE
            exit(EXIT_FAILURE);
            return -1;
        }
    } else{
        /* Handle pipe error here  */
        perror("ERROR: Creating pipe failed. \n");
        return -1;
    }
    return 1;
}
// Establish named pipe and read/write as fast as possible through the pipe.
int named_pipe(size_t size){
    /* Create Pipe and fork the process, establish communication */
    
    /* Print the parent process pid, to use in task C */
    printf("================================\n");
    printf("NAMED PIPE\n");
    printf("================================\n");
    printf("Parent process PID: %i\n",getpid());
    printf("================================\n");
    
    const char* pipe_name = "/tmp/fifo_pipe";
    remove(pipe_name); // Remove the file, free up for a fifo-pipe
    
    int res;   /* child PID */
    int status = mkfifo(pipe_name, 0666);   /* Create a new file to be used as a pipe, for read/write */
    if (status == 0) {                      /* create the pipe-file successfully */
        res = fork ();                      /* pipe created successfully*/
        if (res > 0) {                      /* parent process (Supposed to read)*/
            
            int filedes = open(pipe_name, O_RDONLY);    /* open fifo pipe with read only */
            if (filedes == -1){
                perror("ERROR: file open failed");
                return -1;
            }
            int r = 0;
            char *buff = malloc(size);                  /* status of read */
            while(r != -1){
                r = read(filedes, buff, size);          /* Listen to the pipe */
                if (r == -1){
                    perror("ERROR: read failed");
                    return -1;
                }
                
                bytes_read += (int)size;               /* is reset every alarm */
                cumulative_bytes_read += (int)size;    /* cumulative no. bytes sent */
                /* we never use the buff, and overwrite it all the time, as it is filled with trash */
                
            }
            close (filedes);  /* release the descriptor */
            free(buff); // Free the trash!
        }
        else if (res == 0) {        /* child process (Supposed to write endlessly)*/
            int filedes = open(pipe_name, O_WRONLY);  /* open fifopipe with write only */
            if (filedes == -1){
                perror("ERROR: file open failed");
                return -1;
            }
            int w = 0;
            char* dummy_data = generate_data(size);
            while(w!=-1){
                w = write(filedes, dummy_data, size);
                if (w == -1){
                    perror("ERROR: write failed");
                    return -1;
                }
            }
            free(dummy_data);
            close (filedes);      /* release the descriptor */
        }
        else if (res < 0){          /* if the forking failed*/
            perror("ERROR: Fork failed. \n");
            // Kill the faulty process, with exit signal EXIT_FAILURE
            exit(EXIT_FAILURE);
            return -1;
        }
    } else{                                 /* failed to create  pipe-file*/
        /* Handle pipe error here  */
        perror("ERROR: Creating pipe-file failed. \n");
        return -1;
    }
    
    
    return 1;


}
/* Generate some random data to be communicated through a pipe */
char* generate_data(size_t size){
    char *p = malloc(size);
    for (int i=0; i<size;i++){
        p[i] = 'a';  // Arbritary data
    }
    return p;
}

