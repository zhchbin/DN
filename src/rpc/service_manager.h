// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_SERVICE_MANAGER_H_
#define  RPC_SERVICE_MANAGER_H_

#include <map>
#include <string>
#include <utility>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
class Service;
class Closure;
}  // namespace protobuf
}  // namespace google

namespace rpc {

struct RequestParameters {
  RequestParameters(int connection_id,
                    google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    uint64 request_id,
                    const std::string& service,
                    const std::string& method)
      : connection_id(connection_id),
        request(request),
        response(response),
        request_id(request_id),
        service(service),
        method(method) {
  }

  int connection_id;
  scoped_ptr<google::protobuf::Message> request;
  scoped_ptr<google::protobuf::Message> response;
  uint64 request_id;
  std::string service;
  std::string method;
};

class ServiceManager {
 public:
  typedef std::map<std::string, google::protobuf::Service*> ServiceMap;

  static ServiceManager* GetInstance();

  void RegisterService(google::protobuf::Service* service);
  void UnregisterService(google::protobuf::Service* service);
  google::protobuf::Service* FindService(const std::string& service_full_name);

 private:
  friend struct DefaultSingletonTraits<ServiceManager>;
  ServiceManager();
  ~ServiceManager();

  ServiceMap service_map_;

  DISALLOW_COPY_AND_ASSIGN(ServiceManager);
};

// The first one is the response message pointer, the second is the callback
// closure.
typedef std::pair<google::protobuf::Message*,
                  google::protobuf::Closure*> Response;
typedef std::map<uint32, Response> RequsetIdToResponseMap;

}  // namespace rpc

#endif  // RPC_SERVICE_MANAGER_H_
