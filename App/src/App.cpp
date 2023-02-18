#include <RabBit.h>

class App : public RabBit::Application
{
public:
	App()
	{

	}

	~App()
	{

	}

	int TestValue() override
	{
		return 69;
	}
};

RabBit::Application* RabBit::CreateApplication()
{
	return new App();
}