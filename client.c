#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <openssl/evp.h>

#define PORT 8080 // port for communication

/*
 this function calculates the hash for the newly created file using SHA-256.
 Old librarires were deprecated which was causing various warnings in the code,
 so i referred to these links to find an alternative which is implemented in the
 function.
 1. https://github.com/gluster/glusterfs/issues/2916
 2. https://stackoverflow.com/questions/34289094/alternative-for-calculating-sha256-to-using-deprecated-openssl-code
*/
void calculate_hash(char* file_name, unsigned char* hash_out, unsigned int* hash_len)  {
    FILE* file = fopen(file_name, "rb");
    if (!file) {
        perror("File not found for hashing");
        exit(EXIT_FAILURE);
    }

    // OpenSSL's EVP interface
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        fprintf(stderr, "Could not create EVP_MD_CTX\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    const EVP_MD* md = EVP_sha256(); // using SHA256
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        fprintf(stderr, "Could not initialize digest\n");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    unsigned char buffer[1024];
    size_t bytes_read;
    // update hash
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
            fprintf(stderr, "Error updating digest\n");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(file)) {
        perror("Could not read the file");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);

    if (EVP_DigestFinal_ex(mdctx, hash_out, hash_len) != 1) {
        fprintf(stderr, "Could not finalize the digest\n");
        EVP_MD_CTX_free(mdctx);
        exit(EXIT_FAILURE);
    }

    // free the context
    EVP_MD_CTX_free(mdctx);
}

// to pass arguments to the thread function
typedef struct threadArgument {
    int socket;
    FILE* file;
} threadArgument;

// MUTEX lock
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/*
This thread function receives file segments from the server threads
and writes those bytes into the output file.
*/
void* receive_file_segment(void* arg) {
    threadArgument* thread_arg = (threadArgument*)arg;
    int socket = thread_arg->socket;
    FILE* file = thread_arg->file;
    char buffer[1024];
    ssize_t bytes_received;
    pthread_mutex_lock(&mtx);
    while ((bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
        // lock only around the critical section
        fwrite(buffer, 1, bytes_received, file);
        pthread_mutex_unlock(&mtx);
        printf("received: %s\n", buffer);
        pthread_mutex_lock(&mtx);
    }
    pthread_mutex_unlock(&mtx);
    if (bytes_received < 0) {
        perror("could not receive bytes");
    }

    pthread_exit(NULL);
}

/*
The connection with the server is established in the main, data integrity checks are also
implemented here
*/
int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Please enter two command line arguments.\n");
        return -1;
    }

    char* file_name = argv[1];
    int num_threads = atoi(argv[2]);

    // local host IP address
    char* server_ip = "127.0.0.1";

    int client_fd;
    struct sockaddr_in serv_addr;

    // creating the TCP socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create the socket.");
        return -1;
    }

    // configure server address
    serv_addr.sin_family = AF_INET; // any address
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // establish connection to the server
    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Could not establish the connection");
        return -1;
    }

    // send the command line arguments to the server
    send(client_fd, file_name, strlen(file_name) + 1, 0);
    printf("Client: Sending num_threads = %d\n", num_threads); // debug
    send(client_fd, &num_threads, sizeof(num_threads), 0);

    unsigned char server_hash[EVP_MAX_MD_SIZE];
    recv(client_fd, server_hash, 32, 0);
    printf("Client: Received SHA-256 hash from server.\n");

    // creating the output file
    char output_file_name[256];
    snprintf(output_file_name, sizeof(output_file_name), "rcv_%s", file_name); // rename

    FILE* output_file = fopen(output_file_name, "wb");
    if (output_file == NULL) {
        perror("File creation failed");
        close(client_fd);
        return -1;
    }

    pthread_t thread;
    threadArgument thread_arg = {client_fd, output_file};
    // create the thread
    pthread_create(&thread, NULL, receive_file_segment, &thread_arg);

    // wait for child thread to complete the execution
    pthread_join(thread, NULL);

    printf("File received and reassembled as '%s'.\n", output_file_name);

    fclose(output_file);
    close(client_fd);

    // now that the file has been closed we can compute the checksum

    unsigned char client_hash[32];
    unsigned int hash_len = 0;
    calculate_hash(output_file_name, client_hash, &hash_len);

    // compare the hashes
    if (memcmp(server_hash, client_hash, 32) == 0) {
        printf("File integrity verified: SHA-256 hash matches!\nServer: ");
    } 
    else {
        printf("File integrity compromised: SHA-256 hash mismatch!\nServer: ");
    }

    for (int i = 0; i < 32; i++) {
        printf("%02x", server_hash[i]);
    }
    printf("\nClient: ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", client_hash[i]);
    }
    printf("\n");

    return 0;
}
