#include <stdint.h>

#include "kernel/task.h"
#include "lib/syscalls.h"

extern uint32_t _shell_part_start;
extern uint32_t _shell_stack_top;
extern uint32_t _shell_part_end;

extern uint32_t _shell_data_start;
extern uint32_t _shell_data_end;
extern uint32_t _shell_data_load;

#define LINE_SIZE 32
#define NAME_SIZE 16 //sys_image_name accepts a capacity of 1..16 only (6.5)

static char line[LINE_SIZE]; //requirement ask to accept max 31 characters
static char program_name[NAME_SIZE]; //buffer to store the program name
static void shell_task(void);

/* There is no C library (-nostdlib), so the few string helpers the shell
 * needs are written here. */
static unsigned str_len(const char *s){
    unsigned n = 0;
    while (s[n] != '\0'){
        n++;
    }
    return n;
}

static int str_eq(const char *a, const char *b){
    while (*a != '\0' && *a == *b){
        a++;
        b++;
    }
    return *a == *b;
}

static int str_starts_with(const char *s, const char *prefix){
    while (*prefix != '\0'){
        if (*s != *prefix){
            return 0;
        }
        s++;
        prefix++;
    }
    return 1;
}

void task_project_configure(void){ //called automatically by kernel before the first task
    task_configure_shell("shell", shell_task, &_shell_stack_top, (uint32_t)&_shell_part_start, (uint32_t)&_shell_part_end, &_shell_data_start, &_shell_data_end, &_shell_data_load);
}

static void shell_write(const char *text){
    unsigned remaining = str_len(text);

    while(remaining > 0){
        int written = sys_write(text, remaining);

        if(written > 0){
            text += written;
            remaining -= written;
        }
    }
}

static void shell_read_line(void){
    unsigned length = 0; // counts the number of characters read

    while (1) {
        int c = sys_read(); //reads a character from the UART

        if (c<0){ // waits if no character is available
            continue;
        }

        if (c == '\r' || c == '\n'){ //a CR or an LF prints one newline and ends the line (6.6)
            shell_write("\n");
            break;
        }

        if (length < LINE_SIZE -1){
            line[length] = (char)c; //store the character in the line buffer

            while(sys_write(&line[length], 1)==0){

            }
            length ++; 
        }
    }

    line[length] = '\0';
}

static void shell_list_programs(void){ // function called if ls command is entered
    int count = sys_image_count();  //get the number of programs in the system

    if (count < 0){
        shell_write ("error: no valid image\n");
        return;
    }

    for (int i=0; i<count; i++){
        if (sys_image_name(i, program_name, NAME_SIZE) < 0) {
            shell_write ("error: unable to get program name\n");
            return;
        }
        shell_write(program_name);
        shell_write("\n");
    }
}

static void shell_run_program(const char *name){
    int task_id = sys_program_run(name); 

    if (task_id == -2){
        shell_write ("error: program not found\n");
        return;
    } else if (task_id == -3){
        shell_write ("error: no free slot\n");
        return;
    } else if (task_id < 0){
        shell_write ("error: program cannot be loaded\n");
        return;
    }

    sys_program_wait(task_id); //wait for the program to finish

}


static void shell_task(void){
    while(1){ //keeps the shell running while system in execution
        shell_write("scratch> ");  //prints the prompt before any command
        shell_read_line(); // waits for input

        if (str_eq(line, "help")){
            shell_write ("help - show commands\n"); 
            shell_write("ls - list programs\n"); 
            shell_write("run <name> - run a program\n"); 
        } else if (str_eq(line, "ls")){
            shell_list_programs();
        } else if (str_starts_with(line, "run ") && line[4] != '\0'){ //checks if the command starts with "run " and has a program name after it
            shell_run_program(line + 4); //run the program with the name after "run "
        } else {
            shell_write ("error: unknown command\n");
        }
    }
}
