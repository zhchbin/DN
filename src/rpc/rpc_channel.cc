// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/rpc_channel.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "proto/rpc_message.pb.h"

namespace rpc {

RpcChannel::RpcChannel() {

}

RpcChannel::~RpcChannel() {
}

void RpcChannel::CallMethod(const pb::MethodDescriptor* method,
                            pb::RpcController* controller,
                            const pb::Message* request,
                            pb::Message* response,
                            pb::Closure* done) {
  scoped_ptr<RpcMessage> message(new RpcMessage());
  message->set_id(1);  // FIXME: Generate ID.
  message->set_type(RpcMessage::REQUEST);
  message->set_service(method->service()->full_name());
  message->set_method(method->name());
  message->set_request(request->SerializeAsString());

  // Connect to Rpc server.
  // Send the serialized data out.
  // Wait for the response.
  // Call done->Run().
}

}  // namespace rpc
