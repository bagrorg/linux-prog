#pragma once

#include <string>
#include <stdexcept>
#include <memory>

#ifdef LIBEXPORT
#define EXPORT_API __attribute__((visibility("default")))
#else
#define EXPORT_API __attribute__((visibility("hidden")))
#endif

#define PRIVATE_API __attribute__((visibility("hidden")))

struct PrivateException : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct PublicException : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class TestClass
{
public:
    EXPORT_API TestClass(std::string data);

    TestClass &operator=(TestClass &&cl) noexcept;

    TestClass(TestClass &&cl) noexcept;

    EXPORT_API void something_public() const;

    EXPORT_API ~TestClass();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
