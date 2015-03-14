#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <vector>

#include "cryptopp/modes.h"
#include "cryptopp/aes.h"
#include "cryptopp/sha.h"
#include "cryptopp/filters.h"
#include "cryptopp/hex.h"

using namespace std;

string encryptChunkAES(string plaintext, byte key[], byte iv[]) {
  string ciphertext;

  cout << "Plain Text (" << plaintext.size() << " bytes)" << endl;
  cout << plaintext;
  cout << endl << endl;

  CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
  stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.length() + 1 );
  stfEncryptor.MessageEnd();

  std::cout << "Cipher Text (" << ciphertext.size() << " bytes)" << std::endl;

  for( int i = 0; i < ciphertext.size(); i++ ) {

      std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(ciphertext[i])) << " ";
  }

  std::cout << std::endl << std::endl;

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

  cout << "hash: " << output << endl;
  return reinterpret_cast<const char *>(digest);
}

int main(int argc, char* argv[]) {
  byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
  byte iv[CryptoPP::AES::BLOCKSIZE];
  memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
  memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
  key[0] = 0xac;
  iv[0] = 0xfb;
  
  string plaintext = "testing this thingy with some random chunk of text hohohohohohohohoo";
  string ciphertext = encryptChunkAES(plaintext, key, iv);
  cout << "ciphertext: " << ciphertext << endl;
  string decryptedtext = decryptChunkAES(ciphertext, key, iv);
  cout << "decryptedtext: " << decryptedtext << endl;

  string hash = computeSHA256(plaintext);
}

