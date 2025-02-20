1. **Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?**

   **Answer:**  
   `execvp` replaces the current process image with a new program. If we were to call `execvp` directly, the shell itself would be replaced by the new process, meaning we’d lose our interactive shell prompt. By using `fork`, we create a child process that runs the new command (via `execvp`), while the parent process (the shell) continues running and can wait for the child to finish. In short, `fork` allows the shell to remain active and control process execution.

2. **What happens if the fork() system call fails? How does your implementation handle this scenario?**

   **Answer:**  
   If `fork()` fails, it returns -1, and no child process is created. Our implementation checks for this failure (i.e. a negative return value) and, in that case, prints an error message using `perror("fork")` and continues the main loop. This prevents the shell from crashing and handles the error gracefully.

3. **How does execvp() find the command to execute? What system environment variable plays a role in this process?**

   **Answer:**  
   `execvp()` searches for the command in the directories listed in the `PATH` environment variable. It uses the colon-separated list in `PATH` to locate the executable file for the command, executing the first matching binary it finds.

4. **What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?**

   **Answer:**  
   Calling `wait()` (or `waitpid()`) in the parent process causes the shell to wait for the child process to finish. This is important because it lets the shell retrieve the child’s exit status and prevents the accumulation of zombie processes. Without `wait()`, terminated child processes would remain in the process table, eventually consuming system resources.

5. **In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?**

   **Answer:**  
   `WEXITSTATUS(status)` extracts the exit status value from the status returned by `wait()`/`waitpid()`. This value shows whether the child process terminated successfully (commonly 0) or with an error (nonzero), which is crucial for error handling and for implementing features like a built-in command to report the last command’s return code.

6. **Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?**

   **Answer:**  
   The `build_cmd_buff()` function scans the input line and, when it encounters double quotes (`"`), it toggles a flag that tells it to treat everything inside the quotes as a single argument. This means that spaces inside the quotes are preserved rather than used as delimiters. This is necessary because many commands require arguments with spaces (like file names or messages) that should not be split into separate tokens.

7. **What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?**

   **Answer:**  
   In the previous assignment, the parsing logic handled multiple commands and used a command list structure. For Part 2, I refactored the logic to use a single command buffer (`cmd_buff_t`) without support for pipes. I focused on trimming extra spaces and handling quoted strings correctly. One challenge was ensuring that spaces within quotes were preserved exactly while extra spaces outside the quotes were removed. This refactoring taught me a lot about modular design and the importance of clear parsing rules.

8. **Research on Linux signals**

   - **What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?**

     **Answer:**  
     Signals provide a mechanism for asynchronous communication between processes. They allow a process or the kernel to notify a process that a specific event has occurred (such as an interrupt or a termination request). Unlike other IPC methods (like pipes or shared memory) that exchange data, signals only deliver a small integer (the signal number) and are used for simple notifications and control.

   - **Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?**

     **Answer:**  
     - **SIGKILL:** Forces a process to terminate immediately. It cannot be caught or ignored, and is used as a last resort to stop a process.
     - **SIGTERM:** Requests a process to terminate gracefully. It can be caught and handled, allowing the process to perform cleanup before exiting.
     - **SIGINT:** Typically generated when a user presses Ctrl+C in a terminal. It interrupts the process, and can be caught or ignored, enabling a graceful shutdown or cancellation.

   - **What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?**

     **Answer:**  
     SIGSTOP causes the process to pause its execution immediately. Unlike SIGINT, SIGSTOP cannot be caught, blocked, or ignored because it is intended to unconditionally suspend the process. This is critical for debugging and job control, and the system enforces this behavior so that a process cannot prevent itself from being stopped.
