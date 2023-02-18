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

	void Update() override
	{

	}

	int TestValue() override
	{
		return 69420;
	}
};

RabBit::Application* RabBit::CreateApplication()
{
	return new App();
}