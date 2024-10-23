//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer( SDL_Window* pWindow ) :
	m_pWindow( pWindow ),
	m_pBuffer( SDL_GetWindowSurface( pWindow ) )
{
	//Initialize
	SDL_GetWindowSize( pWindow, &m_Width, &m_Height );
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render( Scene* pScene ) const
{
	/*switch ( m_LightingMode )
	{
	case LightingMode::ObservedArea:
		ExecuteRenderCycle( pScene );
		break;
	case LightingMode::Radiance:
		ExecuteRenderCycle( pScene );
		break;
	case LightingMode::BRDF:
		ExecuteRenderCycle( pScene );
		break;
	case LightingMode::Combined:
		break;
	}
	ExecuteRenderCycle( pScene );*/

	Camera& camera = pScene->GetCamera( );
	auto& materials = pScene->GetMaterials( );
	auto& lights = pScene->GetLights( );

	const float aspectRatio{ float( m_Width ) / float( m_Height ) };
	float x, y;

	// Calculate Camera to World Matrix
	camera.CalculateCameraToWorld( );

	for ( int px{}; px < m_Width; ++px )
	{
		for ( int py{}; py < m_Height; ++py )
		{
			ScreenToNDC( x, y, px, py, aspectRatio, camera.fovCoefficient );

			// Create Ray and place it on the camera origin
			const Vector3 rayDirection{ camera.cameraToWorld.TransformVector( { x, y, 1.f } ) };
			const Ray hitRay{ camera.origin, rayDirection };

			// Color to be filled in the buffer
			ColorRGB finalColor{};
			HitRecord closestHit{};
			pScene->GetClosestHit( hitRay, closestHit );
			if ( closestHit.didHit )
			{
				// Hit visualization
				finalColor = {};

				// Calculate the shade of the pixel
				for ( const dae::Light& light : lights )
				{
					Vector3 hitToLight{ LightUtils::GetDirectionToLight( light, closestHit.origin ) };
					Ray shadowRay{ closestHit.origin, hitToLight.Normalized( ), .0001f, hitToLight.Magnitude( ) };

					// if shadow ray doesn't hit anything, we have a clear view of the light
					// if shadows are not enabled, just pass through
					if ( !m_ShadowsEnabled || !pScene->DoesHit( std::move( shadowRay ) ) )
					{
						const float observedAreaMeasure{ Vector3::Dot( closestHit.normal, hitToLight.Normalized( ) ) };
						if ( observedAreaMeasure >= 0.f )
						{
							if ( m_LightingMode == LightingMode::Combined )
							{
								const ColorRGB Ergb{ LightUtils::GetRadiance( light, hitToLight ) };
								const ColorRGB BRDFrgb{ materials[closestHit.materialIndex]->Shade( closestHit, hitToLight.Normalized(), -hitRay.direction )};

								finalColor += Ergb * BRDFrgb * observedAreaMeasure;
							} 
							else if( m_LightingMode == LightingMode::ObservedArea )
							{
								finalColor += ColorRGB{ 1.f, 1.f, 1.f } * observedAreaMeasure;
							}
							else if ( m_LightingMode == LightingMode::Radiance )
							{
								finalColor += LightUtils::GetRadiance( light, hitToLight );
							}
							else if ( m_LightingMode == LightingMode::BRDF )
							{
								finalColor += materials[closestHit.materialIndex]->Shade( closestHit, hitToLight.Normalized(), -hitRay.direction );
							}
						}
					}
				}
			}

			//Normalize color and update Buffer
			finalColor.MaxToOne( );

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB( m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255) );
		}
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface( m_pWindow );
}

bool Renderer::SaveBufferToImage( ) const
{
	return SDL_SaveBMP( m_pBuffer, "RayTracing_Buffer.bmp" );
}

void dae::Renderer::ToggleShadows( )
{
	m_ShadowsEnabled = !m_ShadowsEnabled;
}

void dae::Renderer::ToggleLightingMode( )
{
	if ( m_LightingMode == LightingMode::Combined )
	{
		m_LightingMode = LightingMode::ObservedArea;
	}
	else
	{
		m_LightingMode = static_cast<LightingMode>(static_cast<int>(m_LightingMode) + 1);
	}
}

/*
void dae::Renderer::ExecuteRenderCycle( dae::Scene* pScene ) const
{
	Camera& camera = pScene->GetCamera( );
	auto& materials = pScene->GetMaterials( );
	auto& lights = pScene->GetLights( );

	const float aspectRatio{ float( m_Width ) / float( m_Height ) };
	float x, y;

	// Calculate Camera to World Matrix
	camera.CalculateCameraToWorld( );

	for ( int px{}; px < m_Width; ++px )
	{
		for ( int py{}; py < m_Height; ++py )
		{
			ScreenToNDC( x, y, px, py, aspectRatio, camera.fovCoefficient );

			// Create Ray and place it on the camera origin
			const Vector3 rayDirection{ camera.cameraToWorld.TransformVector( { x, y, 1.f } ) };
			const Ray hitRay{ camera.origin, rayDirection };

			// Color to be filled in the buffer
			ColorRGB finalColor{};
			HitRecord closestHit{};
			pScene->GetClosestHit( hitRay, closestHit );
			if ( closestHit.didHit )
			{
				// Hit visualization
				finalColor = {};

				// Calculate the shade of the pixel
				for ( const dae::Light& light : lights )
				{
					Vector3 directionToLight{ LightUtils::GetDirectionToLight( light, closestHit.origin ) };
					Ray shadowRay{ closestHit.origin, directionToLight.Normalized( ), .0001f, directionToLight.Magnitude( ) };

					// if shadow ray doesn't hit anything, we have a clear view of the light
					// if shadows are not enabled, just pass through
					if ( !m_ShadowsEnabled || !pScene->DoesHit( std::move( shadowRay ) ) )
					{
						const float observedAreaMeasure{ Vector3::Dot( closestHit.normal, directionToLight.Normalized( ) ) };
						if ( observedAreaMeasure >= 0.f )
						{
							const ColorRGB Ergb{ LightUtils::GetRadiance( light, directionToLight ) };
							const ColorRGB BRDFrgb{ materials[closestHit.materialIndex]->Shade( closestHit, directionToLight, hitRay.direction ) };

							finalColor += Ergb * BRDFrgb * observedAreaMeasure;

						}
					}
				}
			}

			//Update Color in Buffer
			finalColor.MaxToOne( );

			m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB( m_pBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255) );
		}
	}
}
*/

inline void dae::Renderer::ScreenToNDC( float& x, float& y, int px, int py, float aspectRatio, float fov ) const
{
	x = (2.f * (px + 0.5f) / m_Width - 1.f) * aspectRatio * fov;
	y = (1.f - 2.f * (py + 0.5f) / m_Height) * fov;
}
