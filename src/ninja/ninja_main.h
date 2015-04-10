// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#ifndef  NINJA_NINJA_MAIN_H_
#define  NINJA_NINJA_MAIN_H_

#include <set>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "ninja/dn_builder.h"
#include "third_party/ninja/src/build.h"
#include "third_party/ninja/src/build_log.h"
#include "third_party/ninja/src/deps_log.h"
#include "third_party/ninja/src/disk_interface.h"
#include "third_party/ninja/src/state.h"

namespace master {
class MasterMainRunner;
}  // namespace master

namespace ninja {

struct NinjaMain : public BuildLogUser {
 public:
  explicit NinjaMain(const BuildConfig& config);
  virtual ~NinjaMain() {}

  bool InitFromManifest(const std::string& input_file,
                        std::string* error,
                        bool rebuild_manifest);
  void InitForSlave();

  /// Open the build log.
  /// @return false on error.
  bool OpenBuildLog(bool recompact_only = false);

  /// Open the deps log: load it, then open for writing.
  /// @return false on error.
  bool OpenDepsLog(bool recompact_only = false);

  /// Ensure the build directory exists, creating it if necessary.
  /// @return false on error.
  bool EnsureBuildDirExists();

  /// Rebuild the manifest, if necessary.
  /// Fills in \a err on error.
  /// @return true if the manifest was rebuilt.
  bool RebuildManifest(const char* input_file, string* err);

  virtual bool IsPathDead(StringPiece s) const {
    Node* n = state_.LookupNode(s);
    // Just checking n isn't enough: If an old output is both in the build log
    // and in the deps log, it will have a Node object in state_.  (It will also
    // have an in edge if one of its inputs is another output that's in the deps
    // log, but having a deps edge product an output thats input to another deps
    // edge is rare, and the first recompaction will delete all old outputs from
    // the deps log, and then a second recompaction will clear the build log,
    // which seems good enough for this corner case.)
    // Do keep entries around for files which still exist on disk, for
    // generators that want to use this information.
    return (!n || !n->in_edge()) && disk_interface_.Stat(s.AsString()) == 0;
  }

  State& state() { return state_; }
  RealDiskInterface*  disk_interface() { return &disk_interface_; }

  void GetAllEdges(std::set<Edge*>* edges);

  /// Get the Node for a given command-line path, handling features like
  /// spell correction.
  Node* CollectTarget(const char* cpath, string* err);

  /// CollectTarget for all command-line arguments, filling in \a targets.
  bool CollectTargetsFromArgs(int argc, char* argv[],
                              vector<Node*>* targets, string* err);

  bool RunBuild(std::vector<Node*> targets, master::MasterMainRunner* runner);

  ninja::DNBuilder* builder();

 private:
  // Build configuration set from flags (e.g. parallelism).
  BuildConfig config_;

  // Loaded state (rules, nodes).
  State state_;

  // Functions for accesssing the disk.
  RealDiskInterface disk_interface_;

  /// The build directory, used for storing the build log etc.
  std::string build_dir_;

  BuildLog build_log_;
  DepsLog deps_log_;

  scoped_ptr<ninja::DNBuilder> builder_;

  DISALLOW_COPY_AND_ASSIGN(NinjaMain);
};

}  // namespace ninja

#endif  // NINJA_NINJA_MAIN_H_
