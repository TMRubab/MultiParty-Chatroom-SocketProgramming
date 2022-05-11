# Multiuser-Chatroom-SocketProgramming

It requires GCC compiler and a linux based OS.

To run, just use the make command to generate the executable files server and client. Then use different terminals to create one server and multiple client programs.

Upon creation of a client program, you'll be prompted (by the server) to give a nickname, whose maximum size is 24 and minimum size is 1. If there is another person in the chatroom with the same nickname, you'll be prompted until you come up with a new nickname. Additionally, there is a limit of 50 users, so if the chatroom is full, you'll be asked to wait.

Once you get in the chatroom, you have the following 4 options:

1) type "EXIT" to exit from the chatroom: The other users will see that you have left

2) Type "WHO" to see who are active in the chatroom

3) Send direct message using the format "#<user>: <msg>": If you write someone's name that does not exist, you'll be notified of that

4) Input any message to send to everyone in the chatroom

On the server side, every time it receives some request from a client, it prints what's going on (e.g. who the client is, what's their request, what's the message, etc.
