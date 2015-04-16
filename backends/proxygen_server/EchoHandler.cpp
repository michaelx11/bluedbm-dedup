/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "EchoHandler.h"

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include "EchoStats.h"

using namespace proxygen;

pthread_mutex_t ds_lock;

namespace EchoService {

EchoHandler::EchoHandler(EchoStats* stats): stats_(stats) {
  if (pthread_mutex_init(&ds_lock, NULL) != 0)
  {
    printf("\n mutex init failed\n");
  }
}

void processGetRequest(std::unique_ptr<HTTPMessage> headers, EchoStats* stats) noexcept {
  printf("GET REQUEST\n");
  std::string url = headers->getURL();
  std::string path = headers->getPath();
  Storage *instance = stats->getStorage();
  // Check if the request is a "load"
  if (caseInsensitiveEqual(path, "/load")) {
    std::cout << "LOAD" << std::endl;
    std::cout << "url: " << url << std::endl;
    // If so, except certain query parameters
    if (headers->hasQueryParam("filename") &&
        headers->hasQueryParam("path")) {
      std::string filename = headers->getQueryParam("filename");
      std::string path = headers->getQueryParam("path");
      int size = headers->getIntQueryParam("size");
      std::cout << "( filename: " << filename << ", path: " << path << " )" << std::endl;

      // Load and Move
      instance->loadAndMoveFile(filename, path);

    } else {
      std::cout << "missing query parameters!" << std::endl;;
    }


  } else if (caseInsensitiveEqual(path, "/store")) {
    std::cout << "STORE" << std::endl;
    std::cout << "url: " << url << std::endl;
    // If so, except certain query parameters
    if (headers->hasQueryParam("filename") &&
        headers->hasQueryParam("path") &&
        headers->hasQueryParam("size")) {
      std::string filename = headers->getQueryParam("filename");
      std::string path = headers->getQueryParam("path");
      int size = headers->getIntQueryParam("size");
      std::cout << "( filename: " << filename << ", path: " << path << ", size: " << size << " ) " << std::endl;

      // Upload!
      instance->uploadFile(filename, path);
    } else {
      std::cout << "missing query parameters!" << std::endl;;
    }
  }
}

void processPostRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  printf("POST REQUEST\n");
  std::string url = headers->getURL();
  std::string path = headers->getPath();
  if (caseInsensitiveEqual(path, "/store")) {
    std::cout << "STORE" << std::endl;
    std::cout << "url: " << url << std::endl;
    // If so, except certain query parameters
    if (headers->hasQueryParam("filename") &&
        headers->hasQueryParam("path") &&
        headers->hasQueryParam("size")) {
      std::string filename = headers->getQueryParam("filename");
      std::string path = headers->getQueryParam("path");
      int size = headers->getIntQueryParam("size");
      std::cout << "( filename: " << filename << ", path: " << path << ", size: " << size << " ) " << std::endl;

    } else {
      std::cout << "missing query parameters!" << std::endl;;
    }
  }
}

void EchoHandler::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  HTTPMethod method = headers->getMethod().get();
  if (caseInsensitiveEqual(methodToString(method), "GET")) {
    processGetRequest(std::move(headers), stats_);
  } else if (caseInsensitiveEqual(methodToString(method), "POST")) {
    processPostRequest(std::move(headers));
  }
  stats_->recordRequest();
}

void EchoHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
  printf("hello!\n");
  std::unique_ptr<folly::IOBuf> buffer = body.get()->clone();
  folly::fbstring buffString = buffer.get()->moveToFbString();
  printf("content: %s\n", buffString.c_str());
  std::unique_ptr<folly::IOBuf> newBuf = folly::IOBuf::copyBuffer("SUPER COOL HOHOHO\n");
  if (body_) {
    body_->prependChain(std::move(body));
  } else {
//    body_ = std::move(body);
    body_ = std::move(newBuf);
  }
}

void EchoHandler::onEOM() noexcept {
  ResponseBuilder(downstream_)
    .status(200, "OK")
    .header("Request-Number",
            folly::to<std::string>(stats_->getRequestCount()))
    .body(std::move(body_))
    .sendWithEOM();
}

void EchoHandler::onUpgrade(UpgradeProtocol protocol) noexcept {
  // handler doesn't support upgrades
}

void EchoHandler::requestComplete() noexcept {
  delete this;
}

void EchoHandler::onError(ProxygenError err) noexcept {
  delete this;
}

}
