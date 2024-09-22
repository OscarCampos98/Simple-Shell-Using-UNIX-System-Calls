#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <stdbool.h>
#define Bf_length 1000

#define WRITE_END 1
#define READ_END 0

// Function to handle piping between two commands (cmd1 | cmd2)
void handle_pipe(char *cmd1[], char *cmd2[])
{
    int pipefd[2]; // Pipe file descriptors: pipefd[0] for reading, pipefd[1] for writing
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1)
    {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the first child to handle cmd1
    pid1 = fork();
    if (pid1 == -1)
    {
        perror("Error forking first process");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0)
    {
        // In child process for cmd1 (left side of pipe)
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
        close(pipefd[0]);               // Close unused read end
        close(pipefd[1]);               // Close write end after redirecting

        execvp(cmd1[0], cmd1); // Execute first command
        perror("Error executing command 1");
        exit(EXIT_FAILURE);
    }

    // Fork the second child to handle cmd2
    pid2 = fork();
    if (pid2 == -1)
    {
        perror("Error forking second process");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0)
    {
        // In child process for cmd2 (right side of pipe)
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to read end of pipe
        close(pipefd[1]);              // Close unused write end
        close(pipefd[0]);              // Close read end after redirecting

        execvp(cmd2[0], cmd2); // Execute second command
        perror("Error executing command 2");
        exit(EXIT_FAILURE);
    }

    // In parent process, close both ends of the pipe
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Function to handle multiple piping (cmd1 | cmd2 | cmd3 | ...)
void handle_multiple_pipes(char *commands[][100], int num_commands)
{
    int pipefd[2]; // File descriptors for the pipe
    pid_t pid;
    int in_fd = 0; // Input file descriptor for the current command (starts as stdin)

    for (int i = 0; i < num_commands; i++)
    {
        pipe(pipefd); // Create a new pipe for the current command

        pid = fork();
        if (pid == 0)
        {
            // Child process
            dup2(in_fd, STDIN_FILENO); // Use the input from the previous command (or stdin)
            if (i < num_commands - 1)
            {
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the pipe for the next command
            }
            close(pipefd[0]); // Close unused read end of the pipe
            close(pipefd[1]); // Close write end of the pipe

            // execvp call with array of arguments
            execvp(commands[i][0], commands[i]); // Execute the current command
            perror("Error executing command");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        }

        // Parent process
        close(pipefd[1]);  // Close unused write end of the pipe
        in_fd = pipefd[0]; // Save the read end of the pipe for the next command
    }

    // Wait for the last command to finish
    while (wait(NULL) > 0)
        ;
}

// Function for handling input redirection (cmd < file)
void handle_input_redirection(char *cmd[], char *filename)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        // In child process
        int fd_in = open(filename, O_RDONLY); // Open file for reading
        if (fd_in == -1)
        {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd_in, STDIN_FILENO); // Redirect stdin to the file
        close(fd_in);              // Close file descriptor after redirecting

        execvp(cmd[0], cmd); // Execute command
        perror("Error executing command");
        exit(EXIT_FAILURE);
    }
    waitpid(pid, NULL, 0); // Wait for the child process to finish
}

// Function for handling output redirection (cmd > file)
void handle_output_redirection(char *cmd[], char *filename)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        // In child process
        int fd_out = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU); // Open file for writing
        if (fd_out == -1)
        {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd_out, STDOUT_FILENO); // Redirect stdout to the file
        close(fd_out);               // Close file descriptor after redirecting

        execvp(cmd[0], cmd); // Execute command
        perror("Error executing command");
        exit(EXIT_FAILURE);
    }
    waitpid(pid, NULL, 0); // Wait for the child process to finish
}

// Function to handle pipes with redirection
void handle_pipe_with_redirection(char *commands[][100], int num_commands, char *input_file, char *output_file)
{
    int pipefd[2];
    pid_t pid;
    int in_fd = 0; // Input file descriptor, starts as stdin
    int out_fd;    // Output file descriptor

    if (input_file != NULL)
    {
        // Open the input file for reading
        in_fd = open(input_file, O_RDONLY);
        if (in_fd < 0)
        {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_commands; i++)
    {
        if (i < num_commands - 1)
        {
            // Create a pipe for all but the last command
            pipe(pipefd);
        }

        pid = fork();
        if (pid == 0)
        {
            // Child process
            if (i == 0 && input_file != NULL)
            {
                // Redirect stdin to input file for the first command
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (i < num_commands - 1)
            {
                // Redirect stdout to the pipe for all but the last command
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            else if (output_file != NULL)
            {
                // Redirect stdout to the output file for the last command
                out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                if (out_fd < 0)
                {
                    perror("Error opening output file");
                    exit(EXIT_FAILURE);
                }
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

            // Execute the command
            execvp(commands[i][0], commands[i]);
            perror("Error executing command");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        }

        // Parent process
        if (i > 0)
        {
            // Close the input file descriptor from the previous pipe
            close(in_fd);
        }

        if (i < num_commands - 1)
        {
            // Update in_fd to be the read end of the current pipe for the next command
            in_fd = pipefd[0];
            close(pipefd[1]); // Close the write end of the pipe
        }
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0)
        ;
}

// Function to handle background processes (cmd &)
void handle_background_process(char *cmd[])
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process runs in the background
        execvp(cmd[0], cmd); // Execute the command
        perror("Error executing background command");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        printf("Process running in background with PID: %d\n", pid);
    }
    else
    {
        perror("Error forking process");
    }
}

// Function to split a command string into its arguments
void split_command(char *input, char *cmd[])
{
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL)
    {
        cmd[i++] = token;
        token = strtok(NULL, " ");
    }
    cmd[i] = NULL; // Null-terminate the argument list
}

// Function to detect invalid command usage and return errors
int detect_errors(char *input)
{

    // Make a copy of the input string to preserve the original
    char input_copy[1024];
    strcpy(input_copy, input);

    // Check if pipe exists without a valid command on both sides
    if (strstr(input, "|"))
    {
        char *left = strtok(input, "|");
        char *right = strtok(NULL, "|");

        if (left == NULL || right == NULL || strlen(right) == 0)
        {
            fprintf(stderr, "Error: Invalid pipe usage. Command missing after pipe.\n");
            return -1;
        }
    }

    // Check if output redirection is used incorrectly (e.g., `file.txt > more`)
    if (strstr(input, ">") && strtok(input, ">") == NULL)
    {
        fprintf(stderr, "Error: Invalid output redirection.\n");
        return -1;
    }

    // Check for combination of invalid input redirection with pipe
    if (strstr(input_copy, "|"))
    {
        char *cmd = strtok(input_copy, "|");
        int first_command = 1; // Flag for checking the first command
        while (cmd != NULL)
        {
            if (strstr(cmd, "<") && !first_command)
            {
                fprintf(stderr, "Error: Input redirection with pipe is invalid for commands after the first one.\n");
                return -1;
            }
            first_command = 0; // Reset flag after the first command
            cmd = strtok(NULL, "|");
        }
    }

    return 0; // No errors detected
}

// Command parser that handles multiple pipes, redirection, and background processes
void parse_command(char *input)
{
    char *commands[100][100]; // Array to store multiple commands and their arguments
    char *input_file = NULL, *output_file = NULL;
    int num_commands = 0;
    int is_background = 0;

    // Check for errors first
    if (detect_errors(input) < 0)
    {
        return; // Exit if an error was found
    }

    // Check if the command should run in the background (ends with '&')
    if (input[strlen(input) - 1] == '&')
    {
        is_background = 1;
        input[strlen(input) - 1] = '\0'; // Remove '&' from input
    }

    // Check for input redirection (<)
    char *in_redir = strstr(input, "<");
    if (in_redir != NULL)
    {
        *in_redir = '\0'; // Split the command at '<'
        in_redir += 1;
        input_file = strtok(in_redir, " "); // Get the input file
    }

    // Check for output redirection (>)
    char *out_redir = strstr(input, ">");
    if (out_redir != NULL)
    {
        *out_redir = '\0'; // Split the command at '>'
        out_redir += 1;
        output_file = strtok(out_redir, " "); // Get the output file
    }

    // Split the input by the pipe character (|)
    char *token = strtok(input, "|");
    while (token != NULL)
    {
        split_command(token, commands[num_commands]); // Split each command into arguments
        num_commands++;
        token = strtok(NULL, "|");
    }

    // If there are multiple commands (piping)
    if (num_commands > 1)
    {
        // Handle multiple pipes with optional redirection
        handle_pipe_with_redirection(commands, num_commands, input_file, output_file);
    }
    // If there's only one command
    else
    {
        if (input_file != NULL)
        {
            // Handle input redirection for a single command
            handle_input_redirection(commands[0], input_file);
        }
        else if (output_file != NULL)
        {
            // Handle output redirection for a single command
            handle_output_redirection(commands[0], output_file);
        }
        else if (is_background)
        {
            // Handle background process
            handle_background_process(commands[0]);
        }
        else
        {
            // Normal single command execution
            pid_t pid = fork();
            if (pid == 0)
            {
                execvp(commands[0][0], commands[0]); // Execute the command
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
            waitpid(pid, NULL, 0); // Wait for the command to finish
        }
    }
}

int main()
{
    char input[1024]; // Buffer to store user input

    while (1)
    {
        printf("OC_shell>> ");
        fgets(input, sizeof(input), stdin); // Get input from user
        input[strlen(input) - 1] = '\0';    // Remove newline character

        if (strcmp(input, "exit") == 0)
        {
            break; // Exit the shell
        }

        parse_command(input); // Parse and execute the command
    }

    return 0;
}
