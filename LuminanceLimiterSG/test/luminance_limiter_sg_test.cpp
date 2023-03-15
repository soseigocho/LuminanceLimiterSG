#include "../src/luminance_limiter_sg.h"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace luminance_limiter_sg_test
{
	TEST_CLASS(TestHello)
	{
	public:
		TEST_METHOD(test_hello)
		{
			std::string hello = "Hello";
			Assert::AreEqual(hello, luminance_limiter_sg::test_hello());
		};
	};
}