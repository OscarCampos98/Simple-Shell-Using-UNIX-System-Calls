# Simple-Shell-Using-UNIX-System-Calls
#Overview
This project is a simple shell (command-line interpreter) written in C++ that mimics the basic functionality of a UNIX shell. It supports standard operations like command execution, input/output redirection, piping, and background processes. The shell is designed to handle multiple commands and includes error detection for invalid command sequences.

# Features

  - Single Command Execution: The shell can execute a single command with optional arguments.
      Example: ls -l

  - Piping: The shell supports multiple commands connected by pipes (|), where the output of one command becomes the input for the next.
      Example: ls | grep myfile | wc -l

  - Input/Output Redirection: It handles input (<) and output (>) redirection, allowing commands to read from files and write output to   files.
      Input Redirection Example: wc -l < file.txt
      Output Redirection Example: ls -l > output.txt

  - Background Processes: Commands ending with & will be executed in the background, allowing the shell to continue accepting new input without waiting for the background command to finish.
      Example: sleep 10 &

  - Error Detection: The shell detects and reports errors for invalid command sequences, such as missing commands after pipes or incorrect use of redirection.
      Example Errors:
      ls | → "Error: invalid pipe usage. Command missing after pipe."
      file.txt > more → "Error: invalid output redirection."
      ls | more < file.txt → "Error: input redirection with pipe is invalid for commands after the first one."

# System Calls Used
  - fork(): To create new processes.
  - wait(), waitpid(): To wait for processes to finish.
  - execvp(): To execute commands.
  - pipe(): To create pipes for inter-process communication.
  - dup2(): To redirect input/output file descriptors.
  - open(), close(): To open and close files for redirection.

# How to Compile
Navigate to the project directory.
Use g++ to compile the shell. Make sure you run the code using a linux interpreter. i used Ubuntu V20.04

g++ simpleShell.cpp -o myshell -pthread

# How to Use
Once the shell is compiled, run the executable:
./myshell
You will see the shell prompt (OC_shell>>), where you can type commands.

Examples:
    - Run a single command:
        OC_shell>> ls -l
    - Use pipes to connect commands:
        OC_shell>> ls | grep myfile | wc -l
    - Use input/output redirection:
        OC_shell>> cat < poem.txt | grep "are" > output.txt
    - Run a command in the background:
        OC_shell>> sleep 10 &
    - Error Handling
        The shell recognizes several error scenarios and will print an appropriate message:
            - Invalid Pipe Usage: If there is no command after the pipe (|), the shell prints:
            - Invalid Output Redirection: If the output redirection (>) is incorrectly used, such as file.txt > more, the shell prints:
            - Invalid Input Redirection with Pipe: If input redirection (<) is used after a pipe, such as in ls | more < file.txt, the shell prints:

# Known Limitations
The shell does not support advanced features like job control or complex command line parsing (e.g., multi-character separators).
Input and output redirection should not be used in combination with pipes unless the redirection is on the first or last command in the pipeline.


# Test Cases
Basic Commands:
    - date
    - ls -l
Piping:
    - ls | grep A1 | wc -l
Redirection:
    - ls -l > file.lst
    - wc -l < file.txt
Background Commands:
    - sleep 10 &

Error Detection:
    - file.txt > more
    - ls | more < file.txt
    - ls |

# Future Improvements
  - Add support for advanced command line parsing, such as handling multi-character operators and quoted strings.
  - Implement job control for background tasks (e.g., fg, bg commands).
  - Expand error detection to cover more complex cases.

# References
University of Calgary, CPSC 457 - Operating Systems Assignment​(Assign1).
Linux manual pages: man fork, man execvp, man pipe, man dup2.
