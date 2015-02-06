// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  NINJA_DN_BUILDER_H_
#define  NINJA_DN_BUILDER_H_

#include <list>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "third_party/ninja/src/build.h"

namespace master {
class MasterMainRunner;
}  // namespace master

namespace ninja {

/// DNBuilder wraps the build process: starting commands, updating status.
class DNBuilder {
 public:
  DNBuilder(State* state, const BuildConfig& config,
            BuildLog* build_log, DepsLog* deps_log,
            DiskInterface* disk_interface);
  ~DNBuilder();

  /// Clean up after interrupted commands by deleting output files.
  void Cleanup();

  Node* AddTarget(const string& name, string* err);

  /// Add a target to the build, scanning dependencies.
  /// @return false on error.
  bool AddTarget(Node* target, string* err);

  /// Returns true if the build targets are already up to date.
  bool AlreadyUpToDate() const;

  /// Start running the build. Returns false if failed to start.
  /// It is an error to call this function when AlreadyUpToDate() is true.
  bool Build(string* err, master::MasterMainRunner* runner);

  bool StartEdge(Edge* edge, string* err, bool run_in_local);

  /// Update status ninja logs following a command termination.
  /// @return false if the build can not proceed further due to a fatal error.
  bool FinishCommand(CommandRunner::Result* result, string* err);

  bool HasRemoteCommandRunLocally(Edge* edge);
  void BuildLoop();

 private:
  bool ExtractDeps(CommandRunner::Result* result, const string& deps_type,
                   const string& deps_prefix, vector<Node*>* deps_nodes,
                   string* err);
  void BuildFinished();

  State* state_;
  const BuildConfig& config_;
  Plan plan_;
  master::MasterMainRunner* command_runner_;
  scoped_ptr<BuildStatus> status_;
  DiskInterface* disk_interface_;
  DependencyScan scan_;

  base::WeakPtrFactory<DNBuilder> weak_factory_;

  int pending_commands_;

  typedef std::list<Edge*> OutstandingEdgeList;
  OutstandingEdgeList outstanding_edge_list_;

  DISALLOW_COPY_AND_ASSIGN(DNBuilder);
};

}  // namespace ninja

#endif  // NINJA_DN_BUILDER_H_
