// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_RPC_CHANNEL_H_
#define  RPC_RPC_CHANNEL_H_

#include "google/protobuf/service.h"

#include "base/macros.h"

namespace google {
namespace protobuf {

class MethodDescriptor;
class RpcController;
class Messae;
class Closure;

}  // namespace protobuf
}  // namespace google

namespace pb = google::protobuf;

namespace rpc {

class RpcChannel : public google::protobuf::RpcChannel {
 public:
  RpcChannel();
  virtual ~RpcChannel();

  // Call the given method of the remote service.  The signature of this
  // procedure looks the same as Service::CallMethod(), but the requirements
  // are less strict in one important way:  the request and response objects
  // need not be of any specific class as long as their descriptors are
  // method->input_type() and method->output_type().
  void CallMethod(const pb::MethodDescriptor* method,
                  pb::RpcController* controller,
                  const pb::Message* request,
                  pb::Message* response,
                  pb::Closure* done) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcChannel);
};

}  // namespace rpc

#endif  // RPC_RPC_CHANNEL_H_
