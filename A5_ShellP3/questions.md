## 1. Forking and Waiting for Child Processes

**Question:**  
Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

**Answer:**  
Our implementation stores the process IDs (PIDs) of all forked child processes (one per piped command) in an array. After forking, the parent process iterates over this array and calls `waitpid()` for each child process, ensuring that it waits for each one to finish before accepting further input. This guarantees that the entire pipeline completes execution and that no child process is left running.  

If `waitpid()` were omitted, the terminated child processes would become zombie processes because their exit statuses would not be collected. Over time, these zombie processes could accumulate, potentially exhausting system resources and causing unpredictable behavior in the shell.

---

## 2. Closing Unused Pipe Ends After dup2()

**Question:**  
The `dup2()` function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling `dup2()`. What could go wrong if you leave pipes open?

**Answer:**  
After calling `dup2()`, the file descriptor that is duplicated is still open. It is important to close these unused pipe ends to prevent resource leaks and to ensure that the end-of-file (EOF) conditions are correctly detected. For example, if the write end of a pipe remains open in a process that does not need it, the reading process may block indefinitely waiting for EOF, since the system will not signal EOF until all write ends are closed. Additionally, leaving unused pipe descriptors open can eventually lead to running out of available file descriptors.

---

## 3. Built-in Command Implementation of `cd`

**Question:**  
Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require `execvp()`. Why is `cd` implemented as a built-in rather than an external command? What challenges would arise if `cd` were implemented as an external process?

**Answer:**  
The `cd` command is implemented as a built-in because it must change the working directory of the shell process itself. If `cd` were executed as an external command, it would run in a child process, and any directory change would affect only that child process; the parent shell's working directory would remain unchanged. This would render `cd` ineffective for altering the shell environment. Implementing `cd` as a built-in allows the shell to directly call `chdir()`, ensuring the directory change persists across subsequent commands.

---

## 4. Supporting an Arbitrary Number of Piped Commands

**Question:**  
Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

**Answer:**  
To support an arbitrary number of piped commands, I would replace the fixed-size array used for storing command buffers with a dynamically allocated array. This can be achieved by initially allocating a small array and then using `realloc()` to expand it as needed when more commands are encountered. Alternatively, a linked list could be used to store each command as it’s parsed.

**Trade-offs to consider include:**
- **Memory Overhead:** Dynamic allocation introduces overhead, and careful management is needed to avoid memory fragmentation.
- **Code Complexity:** The implementation becomes more complex, with added error checking and management of dynamic memory.
- **Performance Impact:** Frequent reallocations may affect performance, so a strategy like doubling the size of the array might be necessary to amortize the cost.
- **Simplicity vs. Flexibility:** A fixed-size array is simpler and faster but limits the number of commands, whereas dynamic allocation provides flexibility at the cost of increased code complexity.

