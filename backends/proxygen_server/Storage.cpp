#include "Storage.h"


Storage::Storage(const string &directory_) {
  memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
  memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
  key[0] = 0xac;
  iv[0] = 0xfb;
  directoryPath = directory_;
}

Storage::Storage(const string &directory_, const byte key_[], const byte iv_[]) {
  memcpy(key, key_, CryptoPP::AES::DEFAULT_KEYLENGTH);
  memcpy(iv, iv_, CryptoPP::AES::BLOCKSIZE );
  directoryPath = directory_;
}

int Storage::uploadFile(string filename, string srcpath) {
  vector<uint32_t> result;
  // Read file
  char block[BLOCK_SIZE];
  memset(block, 0, BLOCK_SIZE);

  int prev = globalId;
  
  // check if file exists
  struct stat buffer;   
  if (stat (srcpath.c_str(), &buffer) != 0) {
    printf("File %s does not exist!\n", srcpath.c_str());
    return -1;
  }

  ifstream file (srcpath.c_str(), ios::binary);
  file.seekg(0, file.end);
  size_t size = file.tellg();
  // First block is size
  result.push_back((uint64_t)size);
  printf("file size: %zu\n", size);
  file.seekg(0, file.beg);
  printf("Beginning to read!\n");

  int counter = 0;

  int numBlocks = size / BLOCK_SIZE + (size % BLOCK_SIZE == 0 ? 0 : 1);

  /*
  char * big_buffer = (char *)malloc(size + 10 * BLOCK_SIZE);
  memset(big_buffer, 0, size + 10 * BLOCK_SIZE);
  file.read(big_buffer, size);
  printf("num blocks: %d\n", numBlocks);
  for (int i = 0; i < numBlocks; i++) {
//    memcpy(block, big_buffer + i * BLOCK_SIZE, BLOCK_SIZE);
    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(&big_buffer[i * BLOCK_SIZE]), key, iv);
//    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(block), key, iv);
    result.push_back(blockId);
//    memset(block, 0, BLOCK_SIZE);
    counter++;
  }
  */

  while (file.read(block, BLOCK_SIZE)) {
    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(block), key, iv);
    result.push_back(blockId);
    memset(block, 0, BLOCK_SIZE);
    counter++;
  }
  if (file.gcount() > 0) {
    printf("GCOUNT > 0\n");
    uint32_t blockId = processBlock(reinterpret_cast<unsigned char*>(block), key, iv);
    result.push_back(blockId);
    counter++;
  }

  printf("Total blocks needed: %d\n", counter);
  printf("Blocks created: %d\n", globalId - prev);
  printf("Blocks deduped: %d\n", counter - (globalId - prev));
  countUploadedBlocks += counter;
  printf("Totals: created: %d, uploaded: %d\n", globalId + 1, countUploadedBlocks);

  fileMap.emplace(filename, result);
//  free(big_buffer);
  return 0;
}

int Storage::loadAndMoveFile(string filename, string dstpath) {

  if (fileMap.count(filename) <= 0) {
    printf("File %s not found!\n", filename.c_str());
    return -1; // filename not found
  }

  vector<uint32_t> fileRep = fileMap[filename];
  size_t fileSize = fileRep[0];

  unsigned char * buffer = (unsigned char *)malloc(((fileSize / BLOCK_SIZE) + 1) * BLOCK_SIZE);
  for (int i = 1; i < fileRep.size(); i++) {
    uint32_t blockId = fileRep[i];
    memcpy(buffer + (i - 1) * BLOCK_SIZE, blockList[blockId], BLOCK_SIZE);
  }

  FILE* pfile;
  pfile = fopen(dstpath.c_str(), "w");
  fwrite(buffer, sizeof(unsigned char), fileSize, pfile);
  free(buffer);
  fclose(pfile);
  return 0;
}

string Storage::listFiles() {
  return "{filenames: [\"hello\", \"sup\"]}";
}

Storage::~Storage() {};

uint32_t Storage::addBlock(unsigned char block[BLOCK_SIZE]) {
  unsigned char * newBlock = (unsigned char *)malloc(BLOCK_SIZE);
  memcpy(newBlock, block, BLOCK_SIZE);
  blockList.push_back(newBlock);
  globalId++;
  if (globalId % 100000 == 0) {
    printf("tick: %d\n", globalId);
  }
  return globalId;
}

uint32_t Storage::processBlock(unsigned char block[BLOCK_SIZE], byte key[], byte iv[]) {
  string blockStr(reinterpret_cast<char*>(block));
  string hash = computeSHA256(blockStr);
  auto search = blockMap.equal_range(hash);
  int32_t resultBlockId = -1;

  // hash already exists as key
  for (auto item = search.first; item != search.second; item++) {
    uint32_t blockId = item->second;
    if (!memcmp(blockList[blockId], block, BLOCK_SIZE)) {
      resultBlockId = blockId;
      break;
    }
  }
  /*
  if (blockMap.count(hash)) {
    resultBlockId = blockMap.find(hash)->second;
//    printf("dedup: %d\n", resultBlockId);
  } else {
    resultBlockId = addBlock(block);
    blockMap.insert(make_pair(hash, resultBlockId));
//    blockMap.emplace(hash, resultBlockId);
  }
  */
  if (resultBlockId < 0) {
    resultBlockId = addBlock(block);
    blockMap.insert(make_pair(hash, resultBlockId));
  }
  return resultBlockId;
}

string Storage::encryptChunkAES(string plaintext, byte key[], byte iv[]) {
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

string Storage::decryptChunkAES(string ciphertext, byte key[], byte iv[]) {
  string decryptedtext;
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();

  return decryptedtext;
}

string Storage::computeSHA256(string input) {
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
