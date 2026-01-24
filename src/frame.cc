#include "frame.hpp"
#include "snapshot.hpp"

Frame::Frame(const float timestamp_, Snapshot snapshot_) noexcept
  : timestamp(timestamp_), snapshot(std::move(snapshot_)) {}
