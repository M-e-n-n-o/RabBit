#include <RabBit.h>

class App : public RB::Application
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

RB::Application* RB::CreateApplication()
{
	return new App();
}