#pragma once

class IContext
{
protected:
  IContext() noexcept = default;

public:
  virtual ~IContext() noexcept = default;
};
