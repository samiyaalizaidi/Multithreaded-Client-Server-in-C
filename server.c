#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define PORT 8080 // port for communication

typedef struct {
    int socket;
    char *file_name;
    long start_byte;
    long end_byte;
} threadArgument;

// lock to ensure concurrency
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 this function calculates the hash for the newly created file using SHA-256.
 Old librarires were deprecated which was causing various warnings in the code,
 so i referred to these links to find an alternative which is implemented in the
 function.
 1. https://github.com/gluster/glusterfs/issues/2916
 2. https://stackoverflow.com/questions/34289094/alternative-for-calculating-sha256-to-using-deprecated-openssl-code
*/
void calculate_hash(char* file_name, unsigned char* hash_out, unsigned int* hash_len) {
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


void* send_file_segment(void* arg) {
    threadArgument* thread_arg = (threadArgument*)arg;
    int socket = thread_arg->socket;
    char buffer[1024];
    FILE* file = fopen(thread_arg->file_name, "rb");

    if (file == NULL) {
        perror("File open failed");
        pthread_exit(NULL);
    }

    long bytes_to_send = thread_arg->end_byte - thread_arg->start_byte + 1;

    // accessing shared resource - lock required
    pthread_mutex_lock(&mtx);
    if (fseek(file, thread_arg->start_byte, SEEK_SET) != 0) {
        perror("fseek failed");
        fclose(file);
        pthread_mutex_unlock(&mtx);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&mtx);

    while (bytes_to_send > 0) {
        size_t to_read = (bytes_to_send < sizeof(buffer)) ? bytes_to_send : sizeof(buffer);
        // shared resournce - lock required
        pthread_mutex_lock(&mtx);
        size_t bytes_read = fread(buffer, 1, to_read, file);
        pthread_mutex_unlock(&mtx);

        if (bytes_read <= 0) {
            if (feof(file)) { // guards 
                printf("End of file.\n");
                break;
            } else {
                perror("Could not read the file");
                break;
            }
        }

        size_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t sent = send(socket, buffer + total_sent, bytes_read - total_sent, 0);

            if (sent <= 0) {
                perror("Could not send the bytes");
                fclose(file);
                pthread_exit(NULL);
            }
            total_sent += sent;
        }

        bytes_to_send -= bytes_read;
    }

    fclose(file);
    // to kepe track of segments
    printf("Segment transfer complete: [%ld - %ld]\n", thread_arg->start_byte, thread_arg->end_byte);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // creatign TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Could not create the socket.");
        exit(EXIT_FAILURE);
    }

    // configure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // binding socket 
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Could not bind the socket.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // start listening
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        printf("Waiting for a connection...\n");
        if ((client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connection established with client.\n");

        // receive file name and thread count from client
        char file_name[256] = {0};
        int num_threads;
        // recv(client_socket, file_name, sizeof(file_name), 0);
        if (recv(client_socket, file_name, sizeof(file_name), 0) <= 0) {
            perror("Failed to receive file name");
            close(client_socket);
            return 1;
        }
        if (recv(client_socket, &num_threads, sizeof(num_threads), 0) <= 0) {
            perror("Failed to receive number of threads");
            close(client_socket);
            return 1;
        }
        
        printf("Requested file: %s, Number of threads: %d\n", file_name, num_threads);

        FILE* file = fopen(file_name, "rb");
        if (file == NULL) {
            perror("File not found");
            close(client_socket);
            continue;
        }

        // calcualte hash and share with the client for comparison
        unsigned char file_hash[32];
        unsigned int hash_len = 0;
        calculate_hash(file_name, file_hash, &hash_len);

        // sending to client
        send(client_socket, file_hash, 32, 0);
        printf("Server: Sent SHA-256 hash to client.\n");

        // debug
        for (int i = 0; i < 32; i++) {
            printf("%02x", file_hash[i]);
        }
        printf("\n");

        // find the file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fclose(file);

        // calculate the segment size
        float x = (float)file_size / (float)num_threads;
        long segment_size = ceil(x);
        printf("file size: %ld, segment size: %ld\n", file_size, segment_size);

        pthread_t threads[num_threads];
        threadArgument thread_args[num_threads];

        // creating threads to send segments
        for (int i = 0; i < num_threads; i++) {
            thread_args[i].socket = client_socket;
            thread_args[i].file_name = file_name;
            thread_args[i].start_byte = i * segment_size;
            // condition to avoid seg fault
            thread_args[i].end_byte = (i == num_threads - 1) ? file_size - 1 : (i + 1) * segment_size - 1;
            printf("start: %ld, end: %ld\n", thread_args[i].start_byte, thread_args[i].end_byte);
            pthread_create(&threads[i], NULL, send_file_segment, &thread_args[i]);
        }

        // wait for threads to finish
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        printf("File transfer complete.\n");
        close(client_socket);
    }

    close(server_fd);
    return 0;
}
