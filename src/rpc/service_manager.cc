// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/service_manager.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/service.h"

namespace rpc {

// static
ServiceManager* ServiceManager::GetInstance() {
  DCHECK_EQ(base::MessageLoop::current()->type(), base::MessageLoop::TYPE_IO);
  return Singleton<ServiceManager>::get();
}

ServiceManager::ServiceManager() {
}

ServiceManager::~ServiceManager() {
  STLDeleteContainerPairSecondPointers(
      service_map_.begin(), service_map_.end());
}

void ServiceManager::RegisterService(google::protobuf::Service* service) {
  // Don't register same service twice.
  DCHECK(FindService(service->GetDescriptor()->full_name()) == NULL);
  service_map_[service->GetDescriptor()->full_name()] = service;
}

google::protobuf::Service* ServiceManager::FindService(
    const std::string& service_full_name) {
  ServiceMap::iterator it = service_map_.find(service_full_name);
  if (it == service_map_.end())
    return NULL;
  return it->second;
}

}  // namespace rpc
