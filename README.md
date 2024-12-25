# Concurrent File Transfer Application :bookmark_tabs:

This project implements a multi-threaded client-server application for file transfer using socket programming in C. The server splits requested files into segments and sends them concurrently, while the client reassembles the file and verifies its integrity using SHA256 checksums.

## Dependencies
Before you start, ensure the following dependencies are installed on your system:

- **GCC Compiler**: To compile the source code.
- **Make**: For building the project using the Makefile.
- **OpenSSL Library**: Required for SHA256 checksum computations.
  - Install OpenSSL on Ubuntu/Debian-based systems:
    
    ```bash
    sudo apt-get update
    sudo apt-get install libssl-dev   
    ```
  - For other systems, consult the OpenSSL installation guide for your package manager or platform.

## Repository Structure
```
.
├── LICENSE
├── Makefile
├── README.md
├── client.c
├── server.c
└── tests.sh
```
- ``client.c``: The client program for requesting and reassembling files.
- ``server.c``: The server program for handling file requests.
- ``tests.sh``: A script for running automated tests.
## Installation and Usage

1. **Clone the Repository**
  To get started, clone the repository and navigate to the project directory:
    ```bash
    git clone https://github.com/samiyaalizaidi/Multithreaded-Client-Server-in-C
    cd Multithreaded-Client-Server-in-C
    ```

2. **Build the Source Code**
   Compile the source files using the ``Makefile``:
    ```bash
    make build
    ```
3. **Run the Server**
   Start the server to listen for incoming file transfer requests:
   ```bash
   ./server
   ```
4. **Run the Client**
   In a separate terminal, run the client to request a file from the server:
   ```bash
   ./client <file-name> <thread-count>
   ```
   - ``<file-name>``: The name of the file you want to transfer.
   - ``<thread-count>``: The number of threads to use for concurrent file transfer.
     
5. **Run Automated Tests**
   To execute all automated tests, use the following command:
   ```bash
   make run
   ```

6. **Clean the Build Files**
   Remove compiled binaries and other temporary files:
    ```bash
    make clean
    ```
## Features
1. **Multi-Threaded File Transfer**: The server uses threads to transfer file segments concurrently.
2. **Data Integrity Checks**: The application computes SHA256 checksums to ensure data integrity during the transfer.
3. **Automated Testing**: Includes a script to test the application with various scenarios and file types.
   
## Limitations
- Occasionally, the client and server may hang during long or repeated runs. If this happens, restart both programs and try again.
- Ensure that the OpenSSL library is correctly installed, as it's critical for checksum computation.

## Resources Referenced
- Socket programming tutorial: [GeeksforGeeks](https://www.geeksforgeeks.org/socket-programming-cc/)
- Multi-threading concepts: [YouTube Video](https://www.youtube.com/watch?v=Pg_4Jz8ZIH4)
- SHA256 computation using OpenSSL EVP API: [StackOverflow](https://stackoverflow.com/questions/34289094/alternative-for-calculating-sha256-to-using-deprecated-openssl-code) and [GitHub Issue](https://github.com/gluster/glusterfs/issues/2916)

## Contributions
Code written by [Samiya Ali Zaidi](https://github.com/samiyaalizaidi).
