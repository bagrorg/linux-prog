#include "test_lib.hpp"
#include <iostream>
#include <cstdlib>

struct TestClass::Impl
{
    std::string private_field;
    PRIVATE_API void something_private() const;
};

static void throw_fun()
{
    int rand_var = std::rand() % 2;
    if (rand_var)
    {
        throw PrivateException("");
    }
    else
    {
        throw PublicException("");
    }
}

EXPORT_API void TestClass::something_public() const
{
    std::cout << "hello";
    pimpl->something_private();
}

PRIVATE_API void TestClass::Impl::something_private() const
{
    std::cout << "hello private";
    throw_fun();
}

EXPORT_API TestClass::TestClass(std::string data) : pimpl(std::make_unique<Impl>())
{
    pimpl->private_field = std::move(data);
}

EXPORT_API TestClass::~TestClass() = default;

TestClass &TestClass::operator=(TestClass &&cl) noexcept = default;

TestClass::TestClass(TestClass &&cl) noexcept = default;