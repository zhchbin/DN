// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
// Use of this source code is governed by the BSD license that can be
// found in the LICENSE file.

#include "ninja/ninja_builder.h"

#include <set>

#include "base/logging.h"
#include "base/stl_util.h"
#include "third_party/ninja/src/manifest_parser.h"

namespace {

// An implementation of ManifestParser::FileReader that actually reads
// the file.
struct RealFileReader : public ManifestParser::FileReader {
  virtual bool ReadFile(const string& path, string* content, string* err) {
    return ::ReadFile(path, content, err) == 0;
  }
};

void GetAllCommandsHelper(Edge* edge, set<Edge*>* seen,
                          std::vector<std::string>* commands) {
  if (!edge)
    return;
  if (!seen->insert(edge).second)
    return;

  for (vector<Node*>::iterator in = edge->inputs_.begin();
       in != edge->inputs_.end(); ++in)
    GetAllCommandsHelper((*in)->in_edge(), seen, commands);

  if (!edge->is_phony()) {
    commands->push_back(edge->EvaluateCommand());
  }
}

}  // namespace

namespace ninja {

NinjaBuilder::NinjaBuilder(const BuildConfig& config) : config_(config) {}

bool NinjaBuilder::InitFromManifest(const std::string& input_file,
                                    std::string* error,
                                    bool rebuild_manifest) {
  // Reference: https://github.com/martine/ninja/blob/8605b3daa6c68b29e4126e86193acdcfaf7cc2f1/src/ninja.cc#L1064-L1105
  RealFileReader file_reader;
  ManifestParser parser(&state_, &file_reader);
  if (!parser.Load(input_file, error))
    return false;
  if (!EnsureBuildDirExists()) {
    *error = "EnsureBuildDirExists returns error.";
    return false;
  }
  if (!OpenBuildLog() || !OpenDepsLog()) {
    *error = "OpenBuildLog or OpenDepsLog returns error.";
    return false;
  }
  if (rebuild_manifest && RebuildManifest(input_file.c_str(), error))
    return InitFromManifest(input_file, error, !rebuild_manifest);

  return true;
}

bool NinjaBuilder::OpenBuildLog(bool recompact_only) {
  std::string log_path = ".ninja_log";
  if (!build_dir_.empty())
    log_path = build_dir_ + "/" + log_path;

  std::string err;
  if (!build_log_.Load(log_path, &err)) {
    Error("loading build log %s: %s", log_path.c_str(), err.c_str());
    return false;
  }
  if (!err.empty()) {
    // Hack: Load() can return a warning via err by returning true.
    Warning("%s", err.c_str());
    err.clear();
  }

  if (recompact_only) {
    bool success = build_log_.Recompact(log_path, *this, &err);
    if (!success)
      Error("failed recompaction: %s", err.c_str());
    return success;
  }

  if (!config_.dry_run) {
    if (!build_log_.OpenForWrite(log_path, *this, &err)) {
      Error("opening build log: %s", err.c_str());
      return false;
    }
  }

  return true;
}

bool NinjaBuilder::OpenDepsLog(bool recompact_only) {
  string path = ".ninja_deps";
  if (!build_dir_.empty())
    path = build_dir_ + "/" + path;

  string err;
  if (!deps_log_.Load(path, &state_, &err)) {
    Error("loading deps log %s: %s", path.c_str(), err.c_str());
    return false;
  }
  if (!err.empty()) {
    // Hack: Load() can return a warning via err by returning true.
    Warning("%s", err.c_str());
    err.clear();
  }

  if (recompact_only) {
    bool success = deps_log_.Recompact(path, &err);
    if (!success)
      Error("failed recompaction: %s", err.c_str());
    return success;
  }

  if (!config_.dry_run) {
    if (!deps_log_.OpenForWrite(path, &err)) {
      Error("opening deps log: %s", err.c_str());
      return false;
    }
  }

  return true;
}

bool NinjaBuilder::EnsureBuildDirExists() {
  build_dir_ = state_.bindings_.LookupVariable("builddir");
  if (!build_dir_.empty() && !config_.dry_run) {
    if (!disk_interface_.MakeDirs(build_dir_ + "/.") && errno != EEXIST) {
      Error("creating build directory %s: %s",
            build_dir_.c_str(), strerror(errno));
      return false;
    }
  }
  return true;
}

bool NinjaBuilder::RebuildManifest(const char* input_file, string* err) {
  std::string path = input_file;
  unsigned int slash_bits;  // Unused because this path is only used for lookup.
  if (!CanonicalizePath(&path, &slash_bits, err))
    return false;
  Node* node = state_.LookupNode(path);
  if (!node)
    return false;

  Builder builder(&state_, config_, &build_log_, &deps_log_, &disk_interface_);
  if (!builder.AddTarget(node, err))
    return false;

  if (builder.AlreadyUpToDate())
    return false;  // Not an error, but we didn't rebuild.

  // Even if the manifest was cleaned by a restat rule, claim that it was
  // rebuilt.  Not doing so can lead to crashes, see
  // https://github.com/martine/ninja/issues/874
  return builder.Build(err);
}

void NinjaBuilder::GetAllCommands(std::vector<std::string>* commands) {
  commands->clear();
  std::string error;
  std::vector<Node*> nodes = state_.DefaultNodes(&error);
  std::set<Edge*> seen;
  for (vector<Node*>::iterator in = nodes.begin(); in != nodes.end(); ++in)
    GetAllCommandsHelper((*in)->in_edge(), &seen, commands);
}

}  // namespace ninja
