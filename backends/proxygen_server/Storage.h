#ifndef __STORAGE__H
#define __STORAGE__H
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <sstream>
#include <sys/stat.h>

#include "cryptopp/modes.h"
#include "cryptopp/aes.h"
#include "cryptopp/sha.h"
#include "cryptopp/filters.h"
#include "cryptopp/hex.h"

//#define BLOCK_SIZE 128
//#define BLOCK_SIZE 4096
#define BLOCK_SIZE 8192

using namespace std;

uint32_t processBlock(unsigned char block[BLOCK_SIZE], byte key[], byte iv[]);
string encryptChunkAES(string plaintext, byte key[], byte iv[]);
string computeSHA256(string input);
void flushBlocks(vector<uint32_t> fileRep);

class Storage {
  public:
    // increment when a new block is found
    uint32_t globalId = -1;

    // Map from block hashes to id's
    unordered_multimap<string, uint32_t> blockMap;
    unordered_map<string, vector<uint32_t>> fileMap;
    vector<unsigned char *> blockList;
    byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
    byte iv[CryptoPP::AES::BLOCKSIZE];
    string directoryPath;

    // Create storage that writes data to files in directory
    Storage(const string &directory_);

    Storage(const string &directory_, const byte key_[], const byte iv_[]);

    // Destroys storage / cleanup
    ~Storage();

    // Upload a file, returns error code (< 0 for error)
    int uploadFile(string filename, string srcpath);

    // Load and move a file to a specific path
    int loadAndMoveFile(string filename, string dstpath);

  protected:
    uint32_t addBlock(unsigned char block[BLOCK_SIZE]);
    uint32_t processBlock(unsigned char block[BLOCK_SIZE], byte key[], byte iv[]);
    void reconstructFile(string output_path, vector<uint32_t> fileArr, size_t fileSize);
    string encryptChunkAES(string plaintext, byte key[], byte iv[]);
    string decryptChunkAES(string ciphertext, byte key[], byte iv[]);
    string computeSHA256(string input);
};

#endif
