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

void EchoHandler::processLoadRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  printf("load request\n");
  if (headers->hasQueryParam("filename") &&
      headers->hasQueryParam("path")) {
    std::string filename = headers->getQueryParam("filename");
    std::string path = headers->getQueryParam("path");
    std::cout << "( filename: " << filename << ", path: " << path << " )" << std::endl;

    // Load and Move
    Storage *instance = stats_->getStorage();
    instance->loadAndMoveFile(filename, path);

  } else {
    std::cout << "missing query parameters!" << std::endl;;
  }
}

void EchoHandler::processStoreRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  if (headers->hasQueryParam("filename") &&
      headers->hasQueryParam("path") &&
      headers->hasQueryParam("size")) {
    std::string filename = headers->getQueryParam("filename");
    std::string path = headers->getQueryParam("path");
    int size = headers->getIntQueryParam("size");
    std::cout << "( filename: " << filename << ", path: " << path << ", size: " << size << " ) " << std::endl;

    // Upload!
    Storage *instance = stats_->getStorage();
    instance->uploadFile(filename, path);
  } else {
    std::cout << "missing query parameters!" << std::endl;;
  }
}

void EchoHandler::processListRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  Storage *instance = stats_->getStorage();
  std::string fileListJSON = instance->listFiles();
  std::unique_ptr<folly::IOBuf> newBuf = folly::IOBuf::copyBuffer(fileListJSON);
  body_ = std::move(newBuf);
}

void EchoHandler::processGetRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
  printf("GET REQUEST yoohoo\n");
  std::string url = headers->getURL();
  std::string path = headers->getPath();
  std::cout << "path: " << path << std::endl;

  if (caseInsensitiveEqual(path, "/load")) {
    std::cout << "LOAD" << std::endl;
    std::cout << "url: " << url << std::endl;
    processLoadRequest(std::move(headers));

  } else if (caseInsensitiveEqual(path, "/store")) {
    std::cout << "STORE" << std::endl;
    std::cout << "url: " << url << std::endl;
    processStoreRequest(std::move(headers));

  } else if (caseInsensitiveEqual(path, "/list")) {
    std::cout << "LIST" << std::endl;
    std::cout << "url: " << url << std::endl;
    processListRequest(std::move(headers));
  }
}

void EchoHandler::processPostRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
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
    processGetRequest(std::move(headers));
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
