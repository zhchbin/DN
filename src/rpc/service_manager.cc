// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "rpc/service_manager.h"

#include "base/logging.h"
#include "base/stl_util.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/service.h"
#include "thread/ninja_thread.h"

namespace rpc {

// static
ServiceManager* ServiceManager::GetInstance() {
  CHECK(NinjaThread::CurrentlyOn(NinjaThread::RPC));
  static ServiceManager* instance = new ServiceManager();
  return instance;
}

ServiceManager::ServiceManager() {
  STLDeleteContainerPairSecondPointers(
      service_map_.begin(), service_map_.end());
}

ServiceManager::~ServiceManager() {
  STLDeleteContainerPairSecondPointers(
      service_map_.begin(), service_map_.end());
}

void ServiceManager::RegisterService(google::protobuf::Service* service) {
  // Don't Register same service twice.
  DCHECK(service_map_.find(service->GetDescriptor()->full_name()) ==
         service_map_.end());
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
