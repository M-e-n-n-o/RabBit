#include <gtest/gtest.h>

using namespace testing;

int main(int argc, char** argv)
{
	InitGoogleTest(&argc, argv);

	int result = RUN_ALL_TESTS();

#if RB_CONFIG_DEBUG
	std::cout << std::endl << "Enter to quit" << std::endl;
	std::cin.get();
#endif

	return result;
}