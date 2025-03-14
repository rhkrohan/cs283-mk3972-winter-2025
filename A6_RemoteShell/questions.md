# 1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

**Answer:**  
In a remote shell protocol, the server sends an end-of-stream marker (often a special character like 0x04 or `\0`) to signify that it has finished sending the output for a particular command. The client then reads data in a loop until it detects that special marker. Because TCP is a stream protocol, data may arrive in multiple chunks (or “partial reads”), so the client needs to continually `recv()` until it sees that marker.  

Techniques to ensure complete message transmission include:
1. **Looping on `recv()`** until the end-of-message character is encountered.  
2. **Buffering** partial chunks until the complete message or marker is found.  
3. **Using a length‐prefixed protocol** (optional), where the server sends the size of the message first, so the client knows how many bytes to expect.

---

# 2. TCP and Message Boundaries

**Question:** This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

**Answer:**  
Because TCP does not preserve discrete “message” boundaries, a networked shell must explicitly define its own markers or length fields to separate commands and responses. A common approach is to:
- **Null-terminate** each command sent by the client (`\0` at the end).  
- **Use a special EOF marker** for the server’s output (e.g., 0x04).  
- **Loop** reading from the socket until the marker is found.

If boundaries are not explicitly handled, data may be split or coalesced arbitrarily. This can lead to the client reading incomplete commands or mixing multiple commands together, causing errors such as partial commands, concatenated inputs, or confusion about when one command or response ends and the next begins.

---

# 3. Differences Between Stateful and Stateless Protocols

**Answer:**  
- **Stateful protocols** remember past transactions or information for the duration of a session or connection. For example, they can associate client requests with stored session data on the server (e.g., an authenticated session, open file handles, or a pipeline of commands).  
- **Stateless protocols** treat each request as independent. They do not rely on stored session context, so each request contains everything the server needs to process it. HTTP in its simplest form is an example of a stateless protocol (though modern usage can involve session cookies).

---

# 4. UDP is "unreliable". Why use it?

**Answer:**  
Although UDP is termed “unreliable” because it does not guarantee delivery, ordering, or duplicate suppression, it has advantages such as:
1. **Low overhead** and **faster transmission** for small messages.  
2. **No handshake**—great for real-time applications like streaming, VoIP, or gaming, where occasional packet loss is tolerable, and latency is critical.  
3. **Multicasting** and **broadcasting** features not supported by TCP in the same way.

In short, we use UDP when speed and low latency are more important than guaranteed delivery.

---

# 5. Operating System Interface/Abstraction for Network Communications

**Answer:**  
Most operating systems provide **sockets** as the primary abstraction for network communication. Sockets allow applications to send and receive data across various network protocols (TCP, UDP, etc.) using a unified interface. This encapsulates the lower-level details of opening network interfaces, handling IP addresses, and managing packet sends/retransmissions so that applications can work with “read” and “write” style functions on socket file descriptors.
