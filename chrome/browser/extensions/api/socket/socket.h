// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_H_
#define CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_H_
#pragma once

#include <string>

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/extensions/api/api_resource.h"
#include "net/base/io_buffer.h"

namespace net {
class AddressList;
class IPEndPoint;
class Socket;
}

namespace extensions {

// A Socket wraps a low-level socket and includes housekeeping information that
// we need to manage it in the context of an extension.
class Socket : public APIResource {
 public:
  virtual ~Socket();

  // Returns net::OK if successful, or an error code otherwise.
  virtual int Connect(const std::string& address, int port) = 0;
  virtual void Disconnect() = 0;

  virtual int Bind(const std::string& address, int port) = 0;

  // Returns the number of bytes read into the buffer, or a negative number if
  // an error occurred.
  virtual int Read(scoped_refptr<net::IOBuffer> io_buffer,
                   int io_buffer_size) = 0;

  // Returns the number of bytes successfully written, or a negative error
  // code. Note that ERR_IO_PENDING means that the operation blocked, in which
  // case |event_notifier| (supplied at socket creation) will eventually be
  // called with the final result (again, either a nonnegative number of bytes
  // written, or a negative error).
  virtual int Write(scoped_refptr<net::IOBuffer> io_buffer,
                    int byte_count) = 0;

  virtual int RecvFrom(scoped_refptr<net::IOBuffer> io_buffer,
                       int io_buffer_size,
                       net::IPEndPoint *address) = 0;
  virtual int SendTo(scoped_refptr<net::IOBuffer> io_buffer,
                     int byte_count,
                     const std::string& address,
                     int port) = 0;

  virtual void OnDataRead(scoped_refptr<net::IOBuffer> io_buffer,
                          net::IPEndPoint *address,
                          int result);
  virtual void OnWriteComplete(int result);

  static bool StringAndPortToAddressList(const std::string& ip_address_str,
                                         int port,
                                         net::AddressList* address_list);
  static bool StringAndPortToIPEndPoint(const std::string& ip_address_str,
                                        int port,
                                        net::IPEndPoint* ip_end_point);
  static void IPEndPointToStringAndPort(const net::IPEndPoint& address,
                                        std::string* ip_address_str,
                                        int* port);

 protected:
  explicit Socket(APIResourceEventNotifier* event_notifier);

  const std::string address_;
  int port_;
  bool is_connected_;
};

}  //  namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SOCKET_SOCKET_H_
