# Concurrent File Transfer Application

For this assignment, in the socket programming part, I used the ``<arpa/inet.h>`` library. Because I did not have any knowledge of network programming, I had to refer to various resources on the internet. Here are a few of them:

- A really good article by [GeeksforGeeks](https://www.geeksforgeeks.org/socket-programming-cc/). Explained the details of basic functions from the ``<arpa/inet.h>`` library.

- A [YouTube](https://www.youtube.com/watch?v=Pg_4Jz8ZIH4) video where this person explained how to using threads for file transfer - however, he did not use multiple threads to transfer a file from a sever to the client like in our case, instead, he just explained how to create a new thread whenever a new client wants to join. Was very helpful in getting me started with the multi-threading part.

- I did not know anything about how to actually implement SHA256 or MD5 checksums, so had to google that too. From [StackOverflow](https://stackoverflow.com/questions/22880627/sha256-implementation-in-c) I found out about ``<openssl/sha.h>``, however, this library is deprecated and was giving warnings. I referred to MD5 too but faced the same problem there. So, after some searching, I found some useful information on a [GitHub issues pages](https://github.com/gluster/glusterfs/issues/2916) and [StackOverflow Forum](https://stackoverflow.com/questions/34289094/alternative-for-calculating-sha256-to-using-deprecated-openssl-code). Using these two resources, I was able to use the EVP API in ``<openssl/evp.h>`` to compute the SHA256 checksum that is used in both the client and server code.

## How to Run?
The directory contains the following files only:
```
├── Makefile
├── README.md
├── client.c
├── server.c
└── tests.sh
```
``tests.sh`` contains the script that can be used to run the automated tests. All compilation and test running can be performed using the ``Makefile`` as follows:

1. To build the assignment source code.
```bash
make build
```

2. To run the automated tests.
```bash
make run
```

3. To clean the build files.
```bash
make clean
```

## Limitations
Sometimes, while running the ``tests.sh`` file, the client and server get stuck and stop responding, especially, if you have been running for a while. In that case, killing both the programs and running again usually helps.