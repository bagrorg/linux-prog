#pragma once

#include <string>
#include <stdexcept>
#include <memory>

struct PrivateException : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct PublicException : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

extern "C" {
void lol();
}

class TestClass
{
public:
    TestClass(std::string data);

    TestClass &operator=(TestClass &&cl) noexcept;

    TestClass(TestClass &&cl) noexcept;

    void something_public() const;

    ~TestClass();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
