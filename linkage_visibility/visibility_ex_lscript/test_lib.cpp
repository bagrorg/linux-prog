#include "test_lib.hpp"
#include <iostream>
#include <cstdlib>

struct TestClass::Impl
{
    std::string private_field;
    void something_private() const;
};

extern "C" {
void lol() {
    printf("LOL");
}
}

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

void TestClass::something_public() const
{
    std::cout << "hello";
    pimpl->something_private();
}

void TestClass::Impl::something_private() const
{
    std::cout << "hello private";
    throw_fun();
}

TestClass::TestClass(std::string data) : pimpl(std::make_unique<Impl>())
{
    pimpl->private_field = std::move(data);
}

TestClass::~TestClass() = default;

TestClass &TestClass::operator=(TestClass &&cl) noexcept = default;

TestClass::TestClass(TestClass &&cl) noexcept = default;