#pragma once

#include <iostream>
#include <memory>

class IInterface
{
protected:
  IInterface() noexcept = default;

public:
  virtual ~IInterface() noexcept = default;

  virtual void dump(void) noexcept = 0;
};

class IInterfaceFactory
{
protected:
  IInterfaceFactory() noexcept = default;

public:
  virtual ~IInterfaceFactory() noexcept = default;

  virtual std::shared_ptr<IInterface> create_interface(void) const = 0;
};

class LinuxInterface final : public IInterface
{
public:
  void dump(void) noexcept override
  {
    std::cout << "LinuxInterface::dump()" << std::endl;
  }
};

class WindowsInterface final : public IInterface
{
public:
  void dump(void) noexcept override
  {
    std::cout << "WindowsInterface::dump()" << std::endl;
  }
};

class LinuxInterfaceFactory final : public IInterfaceFactory
{
public:
  std::shared_ptr<IInterface> create_interface(void) const override
  {
    return std::make_shared<LinuxInterface>();
  }
};

class WindowsInterfaceFactory final : public IInterfaceFactory
{
public:
  std::shared_ptr<IInterface> create_interface(void) const override
  {
    return std::make_shared<WindowsInterface>();
  }
};
