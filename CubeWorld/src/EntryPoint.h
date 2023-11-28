#include "Application.h"

int main(int argc, char** argv)
{
	ApplicationSpecification spec;
	spec.Name = "CubeWorld";

	Application app(spec);
	if (!app.Init())
		return -1;

	app.Run();
	app.Shutdown();

	return 0;
}
