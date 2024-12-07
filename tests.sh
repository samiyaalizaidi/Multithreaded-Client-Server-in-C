#!/bin/bash
# tests.sh - Automated testing for server-client file transfer

# Start the server in the background
./server & 
SERVER_PID=$!
sleep 1  # Give server time to start

# NOTE: rename to original file names before submitting!!!

# Test 1: Small text file with 1 thread
echo "Test 1: Small text file with 1 thread"
./client testfile.txt 1

# Test 2: Small text file with 5 threads
echo "Test 2: Small text file with 5 threads"
./client testfile.txt 5

# Test 3: Large text file with 10 threads
echo "Test 3: Large text file with 10 threads"
./client largefile.txt 10

# Test 4: Image file with 5 threads
echo "Test 4: Image file with 5 threads"
./client image.jpg 5

# Test 5: Large image file with 10 threads
echo "Test 5: Large image file with 10 threads"
./client largeimage.jpg 10

# Test 6: Small video file with 10 threads
echo "Test 6: Small video file with 10 threads"
./client video.mp4 10

# Test 7: Medium video file with 20 threads
echo "Test 7: Medium video file with 20 threads"
./client mediumvideo.mp4 20

# Test 8: Compressed zip file with 5 threads
echo "Test 8: Compressed zip file with 5 threads"
./client archive.zip 5

# Test 9: Corrupted file handling
echo "Test 9: Corrupted file handling"
./client corruptedfile.bin 5

# Stop the server
kill $SERVER_PID