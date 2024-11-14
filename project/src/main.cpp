//External includes
#ifdef ENABLE_VLD
#include "vld.h"
#endif
#include "SDL.h"
#include "SDL_surface.h"
#undef main

//Standard includes
#include <iostream>
#include <iomanip>
#include <vector>

//Project includes
#include "Timer.h"
#include "Renderer.h"
#include "Scene.h"

//#define SIMPLE_OUTPUT;

using namespace dae;

static void LogSceneInfo( const Scene* pScene, const std::string& status, float dFPS = 0.f )
{
#ifdef SIMPLE_OUTPUT 
	std::cout << pScene->GetSceneName( ) << " - FPS: " << dFPS << std::endl;
	return;
#else
	system( "cls" );
	std::cout << std::left;
	std::cout << "+----------------------------------------------------+" << std::endl;
	std::cout << "| Scene:     " << std::setw( 40 ) << pScene->GetSceneName() << "|" << std::endl;
	std::cout << "| Mode:      " << std::setw( 40 ) << status << "|" << std::endl;
	std::cout << "| FPS:       " << std::setw( 40 ) << dFPS << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;
	/*std::cout << "| Planes:    " << std::setw() << "|" << std::endl;
	std::cout << "| Spheres:   " << "|" << std::endl;
	std::cout << "| Triangles: " << "|" << std::endl;
	std::cout << "+----------------------------------------------------+" << std::endl;*/
#endif
}

static void LogSceneInfo( const Scene* pScene, LightingMode lightingMode, float dFPS )
{
	std::string status{};
	switch ( lightingMode )
	{
	case dae::LightingMode::ObservedArea:
		status = "Observed Area";
		break;
	case dae::LightingMode::Radiance:
		status = "Radiance";
		break;
	case dae::LightingMode::BRDF:
		status = "BRDF";
		break;
	case dae::LightingMode::Combined:
		status = "Combined";
		break;
	}
	LogSceneInfo( pScene, status, dFPS );
}

static void LoadScene( Scene** pScene, const std::function<Scene* ( )>& fnFactory )
{
	delete *pScene;
	*pScene = fnFactory();

	LogSceneInfo( *pScene, "Initializing" );
	( *pScene )->Initialize( );
}

void ShutDown( SDL_Window* pWindow )
{
	SDL_DestroyWindow( pWindow );
	SDL_Quit( );
}

int main( int argc, char* args[] )
{
	//Unreferenced parameters
	(void) argc;
	(void) args;

	//Create window + surfaces
	SDL_Init( SDL_INIT_VIDEO );

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"RayTracer - Alessandro Manzini",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0 );

	if ( !pWindow )
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer( );
	const auto pRenderer = new Renderer( pWindow );

	size_t sceneIndex = 0;
	// Vector of factory functions to create each type on demand
	std::vector<std::function<Scene* ( )>> sceneFactories{
		[]( ) -> auto*
		{
			return new Scene_W4_ReferenceScene( );
		},
		[]( ) -> auto*
		{
			return new Scene_W4_BunnyScene( );
		}
	};

	auto pScene = sceneFactories.at( 0 )();

	pScene->Initialize( );

	//Start loop
	pTimer->Start( );

	// Start Benchmark
	// pTimer->StartBenchmark();

	float printTimer = 0.f;
	bool isLooping = true;
	bool takeScreenshot = false;
	while ( isLooping )
	{
		//--------- Get input events ---------
		SDL_Event e;
		while ( SDL_PollEvent( &e ) )
		{
			switch ( e.type )
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				switch ( e.key.keysym.scancode )
				{
				case SDL_SCANCODE_X:
					takeScreenshot = true;
					break;

				case SDL_SCANCODE_F2:
					pRenderer->ToggleShadows( );
					break;
				case SDL_SCANCODE_F3:
					pRenderer->ToggleLightingMode( );
					break;
				case SDL_SCANCODE_UP:
					sceneIndex = ( sceneIndex + 1 ) % sceneFactories.size( );
					LoadScene( &pScene, sceneFactories.at( sceneIndex ) );
					break;
				case SDL_SCANCODE_DOWN:
					if ( sceneIndex < 1 )
					{
						sceneIndex = sceneFactories.size( ) - 1;
					}
					else
					{
						--sceneIndex;
					}
					LoadScene( &pScene, sceneFactories.at( sceneIndex ) );
					break;
				}

				break;
			}
		}

		//--------- Update ---------
		pScene->Update( pTimer );

		//--------- Render ---------
		pRenderer->Render( pScene );

		//--------- Timer ---------
		pTimer->Update( );
		printTimer += pTimer->GetElapsed( );
		if ( printTimer >= 1.f )
		{
			printTimer = 0.f;
			LogSceneInfo( pScene, pRenderer->GetLightingMode(), pTimer->GetdFPS( ) );
		}

		//Save screenshot after full render
		if ( takeScreenshot )
		{
			if ( !pRenderer->SaveBufferToImage( ) )
				std::cout << "Screenshot saved!" << std::endl;
			else
				std::cout << "Something went wrong. Screenshot not saved!" << std::endl;
			takeScreenshot = false;
		}
	}
	pTimer->Stop( );

	//Shutdown "framework"
	delete pScene;
	delete pRenderer;
	delete pTimer;

	ShutDown( pWindow );
	return 0;
}