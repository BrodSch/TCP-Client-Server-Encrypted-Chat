# TCP CLIENT & SERVER ENCRYPTED CHAT

## Table of Contents:

- [Video Demo](#video-demo)
- [Description](#desc)
- [Process](#process)
- [Systems](#systems)
- [Usage](#usage)
- [Files](#files)
- [Sources](#sources)

##

### Video Demo:  <https://youtu.be/g6FLdsqq35w>

##

### Description:

A simple TCP Client-Server Chat platform which allows the server and the client to have conversations with each other. Socket Programming is a heavily-used concept in the field of cybersecurity, and as such, it is very important to use these projects as a way to learn and delve into certain topics.

C was mainly used for this project, since it is a language that is minimalistic while handling a lot of operations at the same time.

The chat log will be recorded in a seperate .csv file, where you will see the date, time, sender, receiver and message. Fret not, the message is entirely encrypted depending on which program you choose to run. The message is also logged in a relational database, where you can see the who, the what and the when.

The encryption algorithms made in the sample files were half self-made, half imported. The self-made ones are only the lite and standard versions, while the official ones are the heavyhitters.

Please do not enter any sensitive information within the chat client (e.g. passwords, bank account details, personal information).

Thank you very much and enjoy.

##

### Process:

1. Compute server-side interface
2. Compute client-side interface
3. Connect both to allow interactivity
4. Input parsed into the command line will be encrypted via an algorithm
5. The standard message will be displayed in the command line.
6. The chat-log, which will record the entire chat in a seperate .csv file, will be encrypted and stored in a database.
7. Database will contain two columns: 1. Date + Time 2. Sender 3. Receiver 4. Encrypted Message

##

### Systems:

- The research
- The server-side (C)
- The client-side (C)
- The encryption algorithm (C)
- The .csv file (C)
- The transfer to db process (Python)
- The database (SQL)

##

### Usage:

1. Open split terminal windows

2. Type this command in one terminal window: used to compile the server file.

        gcc src/server.c samples/{directory containing your own configuration}/{configuration file}.c -o server

3. Type this command in the other terminal window: used to compile the client file.

        gcc src/client.c samples/{directory containing your own configuration}/{configuration file}.c -o client

4. Execute both compiled files:

        ./server {port-number}
        ./client {ip-address} {port-number}

    - For the port number, any port number from 0 < x < 65535, since port no. is 16-byte, 2^16 = 65536.
    - For the ip address, you can type "loopback" to refer to your own device.

5. Begin chatting

6. New file will appear named after the date and time of when the conversation initiated.

6. Enter into your terminal:

        sqlite3 database/logs.db

    Once you are in sqlite3, type .schema to see all the tables.

##

### Files:

- Database: Stores the chat logs.
- Samples: Pre-configured files for testing, each progressive layer has a stronger encryption algorithm.
- Source: Source files for the client, server and transfer interface.
- Template: Use template to create new sample files to testing more encryption algorithms.

##

### Sources:
- GeeksforGeeks. (2026, April 22). Socket programming in C. GeeksforGeeks. https://www.geeksforgeeks.org/c/socket-programming-cc/
- GeeksforGeeks. (2025, July 11). TCP and UDP server using select. GeeksforGeeks. https://www.geeksforgeeks.org/computer-networks/tcp-and-udp-server-using-select/
- Alex The Dev. (2024, September 11). Networking in C | Sockets [Video]. YouTube. https://www.youtube.com/watch?v=eBPtDUZbZK0
- Gpg. (n.d.). GitHub - gpg/libgcrypt: The GNU crypto library. NOTE: Maintainers are not tracking this mirror. Do not make pull requests here, nor comment any commits, submit them usual way   to bug tracker (https://www.gnupg.org/documentation/bts.html) or to the mailing list (https://www.gnupg.org/documentation/mailing-lists.html). GitHub. https://github.com/gpg/libgcrypt/tree/master
- Polfosol. (n.d.). GitHub - polfosol/micro-AES: The smallest readable implementation of AES algorithms in C. GitHub. https://github.com/polfosol/micro-AES/tree/master
- Openssl. (n.d.). GitHub - openssl/openssl: General purpose TLS and crypto library. GitHub. https://github.com/openssl/openssl/tree/master
- Google Gemini

##
