#pragma once

#include "snapshot.hpp"

struct Frame final
{
  float    timestamp;
  Snapshot snapshot;

  Frame() noexcept = default;

  Frame(const float timestamp_, Snapshot snapshot_) noexcept;
};
