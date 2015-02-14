// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  MASTER_MASTER_RPC_H_
#define  MASTER_MASTER_RPC_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/timer/timer.h"
#include "rpc/rpc_socket_server.h"
#include "thread/ninja_thread_delegate.h"

namespace slave {
class RunCommandResponse;
class StatusResponse;
class SystemInfoResponse;
}  // namespace slave

namespace master {

class MasterMainRunner;

class MasterRPC : public NinjaThreadDelegate,
                  public rpc::RpcSocketServer::Observer {
 public:
  MasterRPC(const std::string& bind_ip,
            uint16 port,
            MasterMainRunner* master_main_runner);
  ~MasterRPC() override;

  // NinjaThreadDelegate implementations.
  void Init() override;
  void InitAsync() override;
  void CleanUp() override;

  // rpc::RpcSocketServer::Observer implementations.
  void OnConnect(rpc::RpcConnection* connection) override;
  void OnClose(rpc::RpcConnection* connection) override;

  typedef std::vector<std::string> Directories;
  void StartCommandRemotely(int connection_id,
                            const Directories& dirs,
                            const std::string& rspfile_name,
                            const std::string& rspfile_content,
                            const std::string& command,
                            uint32 edge_id);
  void QuitSlave(int connection_id, const std::string& reason);

  void OnRemoteCommandDone(int connection_id,
                           slave::RunCommandResponse* raw_response);
  void OnSlaveSystemInfoAvailable(int connection_id,
                                  slave::SystemInfoResponse* raw_response);
  void OnSlaveStatusUpdate(int connection_id,
                           slave::StatusResponse* raw_response);
  void GetSlavesStatus();

 private:
  std::string bind_ip_;
  uint16 port_;
  scoped_ptr<rpc::RpcSocketServer> rpc_socket_server_;
  MasterMainRunner* master_main_runner_;

  typedef std::map<int, rpc::RpcConnection*> ConnectionMap;
  ConnectionMap connections_;

  // Timer for checking slave status every two seconds.
  scoped_ptr<base::RepeatingTimer<MasterRPC> > timer_;

  DISALLOW_COPY_AND_ASSIGN(MasterRPC);
};

}  // namespace master

#endif  // MASTER_MASTER_RPC_H_
