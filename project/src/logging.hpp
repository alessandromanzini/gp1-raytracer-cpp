#include <iostream>
#include <iomanip>
#include "Scene.h"

using namespace dae;

struct LogInfo
{
	const Scene* pScene;
	int lightingMode;
	int shadowMode;
	bool gi;
	float dFPS;
};

static std::string lightingModeMap[]{
	"Observed Area",
	"Radiance",
	"BRDF" ,
	"Combined" 
};

static std::string shadowModeMap[]{
	"Hard",
	"Soft",
	"None"
};

static void LogSceneInfo( float dFPS )
{
	std::cout << "dFPS: " << dFPS << std::endl;
}

static void LogSceneInfo( const std::string& state )
{
	system( "cls" );
	std::cout << std::left << std::boolalpha;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Scene:                                             |" << std::endl;
	std::cout << "| dFPS:                                              |" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Mode:      " << std::setw( 40 ) << state << "|" << std::endl;
	std::cout << "| Shadows:                                           |" << std::endl;
	std::cout << "| GI:                                                |" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
}

static void LogSceneInfo( const LogInfo& logInfo )
{
	system( "cls" );
	std::cout << std::left << std::boolalpha;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Scene:     " << std::setw( 40 ) << logInfo.pScene->GetSceneName() << "|" << std::endl;
	std::cout << "| dFPS:      " << std::setw( 40 ) << logInfo.dFPS << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Mode:      " << std::setw( 40 ) << lightingModeMap[logInfo.lightingMode] << "|" << std::endl;
	std::cout << "| Shadows:   " << std::setw( 40 ) << shadowModeMap[logInfo.shadowMode] << "|" << std::endl;
	std::cout << "| GI:        " << std::setw( 40 ) << logInfo.gi << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
}
