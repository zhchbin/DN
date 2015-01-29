// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  RPC_SERVICE_MANAGER_H_
#define  RPC_SERVICE_MANAGER_H_

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"

namespace google {
namespace protobuf {
class Service;
class Closure;
}  // namespace protobuf
}  // namespace google

namespace rpc {

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

}  // namespace rpc

#endif  // RPC_SERVICE_MANAGER_H_
