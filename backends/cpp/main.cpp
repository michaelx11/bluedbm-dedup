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

#include "cryptopp/modes.h"
#include "cryptopp/aes.h"
#include "cryptopp/sha.h"
#include "cryptopp/filters.h"
#include "cryptopp/hex.h"

using namespace std;

//#define BLOCK_SIZE 128
//#define BLOCK_SIZE 4096
#define BLOCK_SIZE 8192

// increment when a new block is found
uint32_t globalId = -1;

// Map from block hashes to id's
unordered_multimap<string, uint32_t> blockMap;
vector<unsigned char *> blockList;

// Declarations
uint32_t processBlock(unsigned char block[BLOCK_SIZE], byte key[], byte iv[]);
string encryptChunkAES(string plaintext, byte key[], byte iv[]);
string computeSHA256(string input);
void flushBlocks(vector<uint32_t> fileRep);

uint32_t addBlock(unsigned char block[BLOCK_SIZE]) {
  unsigned char * newBlock = (unsigned char *)malloc(BLOCK_SIZE);
  memcpy(newBlock, block, BLOCK_SIZE);
  blockList.push_back(newBlock);
  globalId++;
}

uint32_t processBlock(unsigned char block[BLOCK_SIZE], byte key[], byte iv[]) {
  string blockStr(reinterpret_cast<char*>(block));
  string hash = computeSHA256(blockStr);
  auto search = blockMap.equal_range(hash);
  uint32_t resultBlockId = -1;

  // hash already exists as key
  for (auto item = search.first; item != search.second; item++) {
    uint32_t blockId = item->second;
    if (memcmp(blockList[blockId], block, BLOCK_SIZE)) {
      resultBlockId = blockId;
      break;
    }
  }
  
  // not found
  if (resultBlockId == -1) {
    resultBlockId = addBlock(block);
  }
  blockMap.emplace(hash, resultBlockId);
  return resultBlockId;
}

vector<uint32_t> uploadFile(string path, byte key[], byte iv[], uint32_t *sizeP) {
  vector<uint32_t> result;
  // Read file
  char block[BLOCK_SIZE];
  memset(block, 0, BLOCK_SIZE);

  int prev = globalId;

  ifstream file (path.c_str(), ios::binary);
  file.seekg(0, file.end);
  int size = file.tellg();
  *sizeP = size;
  file.seekg(0, file.beg);

  int counter = 0;

  while (file.read(block, BLOCK_SIZE)) {
    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(block), key, iv);
    result.push_back(blockId);
    memset(block, 0, BLOCK_SIZE);
    counter++;
  }
  if (file.gcount() > 0) {
    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(block), key, iv);
    result.push_back(blockId);
    counter++;
  }

  printf("Total blocks needed: %d\n", counter);
  printf("Blocks created: %d\n", globalId - prev);
  printf("Blocks deduped: %d\n", counter - (globalId - prev));

  // Break file into blocks
  return result;
}

void reconstructFile(string output_path, vector<uint32_t> fileArr, size_t fileSize) {
  unsigned char * buffer = (unsigned char *)malloc(((fileSize / BLOCK_SIZE) + 1) * BLOCK_SIZE);
//  unsigned char buffer[((fileSize / BLOCK_SIZE) + 1) * BLOCK_SIZE];
  for (int i = 0; i < fileArr.size(); i++) {
    uint32_t blockId = fileArr[i];
    memcpy(buffer + i * BLOCK_SIZE, blockList[blockId], BLOCK_SIZE);
  }

  FILE* pfile;
  pfile = fopen(output_path.c_str(), "wb");
  fwrite(buffer, sizeof(unsigned char), fileSize, pfile);
  free(buffer);
}

string encryptChunkAES(string plaintext, byte key[], byte iv[]) {
  string ciphertext;

  CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
  stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.length() + 1 );
  stfEncryptor.MessageEnd();

  /*
  cout << "Plain Text (" << plaintext.size() << " bytes)" << endl;
  cout << plaintext;
  cout << endl << endl;
  std::cout << "Cipher Text (" << ciphertext.size() << " bytes)" << std::endl;

  for( int i = 0; i < ciphertext.size(); i++ ) {

      std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(ciphertext[i])) << " ";
  }

  std::cout << std::endl << std::endl;
  */
  return ciphertext;
}

string decryptChunkAES(string ciphertext, byte key[], byte iv[]) {
  string decryptedtext;
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();

  return decryptedtext;
}

string computeSHA256(string input) {
  CryptoPP::SHA256 hash;
  byte digest[CryptoPP::SHA256::DIGESTSIZE];
  hash.CalculateDigest(digest, reinterpret_cast<const unsigned char*>(input.c_str()), input.length());

  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(digest, sizeof(digest));
  encoder.MessageEnd();

//  cout << "hash: " << output << endl;
  return reinterpret_cast<const char *>(digest);
}




int main(int argc, char* argv[]) {

  if (argc < 2) {
    cout << "missing file argument" << endl;
    return 0;
  }
  byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
  byte iv[CryptoPP::AES::BLOCKSIZE];
  memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
  memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
  key[0] = 0xac;
  iv[0] = 0xfb;

  
  string plaintext = "testing this thingy with some random chunk of text hohohohohohohohoo";
  string ciphertext = encryptChunkAES(plaintext, key, iv);
//  cout << "ciphertext: " << ciphertext << endl;
  string decryptedtext = decryptChunkAES(ciphertext, key, iv);
//  cout << "decryptedtext: " << decryptedtext << endl;

  string hash = computeSHA256(plaintext);

  string inputFile = argv[1];
  int beginIdx = inputFile.rfind('/');
  string name = inputFile.substr(beginIdx + 1);

  cout << "file: " << inputFile << endl;
  uint32_t sizeP = 0;
  vector<uint32_t> fileArr = uploadFile(inputFile, key, iv, &sizeP);
  printf("Total size: %d bytes\n", sizeP);
  // Prints file representation as ids
  printf("file: ");
  for (int i = 0; i < fileArr.size(); i++) {
    printf("%d ", fileArr[i]);
  }
  cout<<endl;

  string outputFile = "constructed/reconstructed-" + name;
  cout << "Output: " << outputFile << endl;

  reconstructFile(outputFile, fileArr, sizeP);
}

