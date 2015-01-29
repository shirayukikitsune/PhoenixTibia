#include "ComponentManager.h"
#include "Component.h"
#include "NetworkManager.h"
#include "Settings.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <thread>
#include <vector>
#include <clocale>

int main(int argc, char* argv[])
{
	std::cout << "Phoenix Tibia Server" << std::endl
	          << "Kernel Version 0.1 - compiled on " << __DATE__ << " " __TIME__ << std::endl
	          << std::endl;

	std::string configFile = "config.lua";
	if (argc > 1) configFile = argv[1];

	std::shared_ptr<Settings> settings(new Settings(configFile));
	std::cout << "> Loading settings from " << configFile << std::endl;
	if (!settings->load())
		return 1;

	setlocale(LC_ALL, settings->getString("locale").c_str());

	std::shared_ptr<ComponentManager> manager(new ComponentManager);
	std::shared_ptr<NetworkManager> network(new NetworkManager(manager));
	manager->registerDefaultComponents();
	manager->loadComponents(settings, network);

	std::cout << "> Creating workers";
	unsigned int threadCount = settings->getUnsigned("workerCount");
	if (threadCount == 0) {
		threadCount = std::thread::hardware_concurrency();
		std::cout << " - notice: invalid number for workerCount, setting to the processor count (" << threadCount << ")";
	}
	std::cout << std::endl;

	std::vector<std::thread> threads(threadCount);

	for (auto &thread : threads)
		thread = std::thread([network]() { network->getIoService().run(); });

	manager->OnBeforeNetworkStart();

	network->start();

	std::cout << "> Running" << std::endl;

	manager->OnServerReady();

	for (auto &thread : threads)
		thread.join();

	std::cout << "> Terminating" << std::endl;

	manager->OnServerTerminating();

	network.reset();

	manager->unloadComponents();
	manager.reset();
	settings.reset();

	std::cout << "> Thanks!" << std::endl;
	return 0;
}
