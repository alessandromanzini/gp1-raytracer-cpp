#include <iostream>
#include <iomanip>
#include "Renderer.h"
#include "Scene.h"

using namespace dae;

struct LogInfo
{
	std::string sceneName;
	std::string status;
	bool shadows;
	bool ss;
	bool gi;
	float dFPS;
};

static void LogSceneInfo( float dFPS )
{
	std::cout << "dFPS: " << dFPS << std::endl;
}

static void LogSceneInfo( const LogInfo& logInfo )
{
	system( "cls" );
	std::cout << std::left << std::boolalpha;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Scene:     " << std::setw( 40 ) << logInfo.sceneName << "|" << std::endl;
	std::cout << "| dFPS:      " << std::setw( 40 ) << logInfo.dFPS << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Mode:      " << std::setw( 40 ) << logInfo.status << "|" << std::endl;
	std::cout << "| Shadows:   " << std::setw( 40 ) << logInfo.shadows << "|" << std::endl;
	std::cout << "| GI:        " << std::setw( 40 ) << logInfo.gi << "|" << std::endl;
	std::cout << "| SS:        " << std::setw( 40 ) << logInfo.ss << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
}
