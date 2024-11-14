//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <execution>

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Utils.h"

#define PARALLEL_EXECUTION

using namespace dae;

Renderer::Renderer( SDL_Window* pWindow ) :
	m_pWindow( pWindow ),
	m_pBuffer( SDL_GetWindowSurface( pWindow ) )
{
	//Initialize
	SDL_GetWindowSize( pWindow, &m_Width, &m_Height );
	m_AspectRatio = float( m_Width ) / float( m_Height );
	m_pBufferPixels = static_cast<uint32_t*>( m_pBuffer->pixels );

	// Create a dynamic array with the amount of pixels
	uint32_t amountOfPixels{ uint32_t( m_Width * m_Height ) };
	m_pPixelIndices = new uint32_t[amountOfPixels];
	// Fill with sequential values starting at 0
	std::iota( m_pPixelIndices, m_pPixelIndices + amountOfPixels, 0 );

	SetLightingMode( LightingMode::Combined );
}

dae::Renderer::~Renderer( )
{
	delete[] m_pPixelIndices;
}

void Renderer::Render( Scene* pScene ) const
{
	Camera& camera = pScene->GetCamera( );
	camera.CalculateCameraToWorld( );

#ifdef PARALLEL_EXECUTION
	// Parallel logic
	uint32_t amountOfPixels{ uint32_t( m_Width * m_Height ) };

	std::for_each( std::execution::par, m_pPixelIndices, m_pPixelIndices + amountOfPixels,
	[this, pScene, &camera]( uint32_t pixelIdx )
		{
			RenderPixel( pScene, pixelIdx, camera );
		} );
#else
	// Single thread logic
	uint32_t amountOfPixels{ uint32_t( m_Width * m_Height ) };
	for ( uint32_t pixelIdx{}; pixelIdx < amountOfPixels; ++pixelIdx )
	{
		RenderPixel( pScene, pixelIdx, camera );
	}
#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface( m_pWindow );
}

void dae::Renderer::RenderPixel( Scene* pScene, uint32_t pixelIdx, const Camera& camera ) const
{
	const uint32_t px{ pixelIdx % m_Width };
	const uint32_t py{ pixelIdx / m_Width };

	float x, y;
	ScreenToNDC( x, y, px, py, camera.fovCoefficient );

	// Color to be filled in the buffer
	ColorRGB finalColor{};

	LightingInfo info{};

	// Create Ray and place it on the camera origin
	info.rayDirection = camera.cameraToWorld.TransformVector( { x, y, 1.f } );
	info.hitRay = { camera.origin, info.rayDirection };

	pScene->GetClosestHit( info.hitRay, info.closestHit );
	if ( info.closestHit.didHit )
	{
		// Hit visualization
		finalColor = {};

		// Calculate the shade of the pixel
		for ( const dae::Light& light : pScene->GetLights( ) )
		{
			info.hitToLight = LightUtils::GetDirectionToLight( light, info.closestHit.origin );
			info.hitToLightDistance = info.hitToLight.Normalize( );
			info.pLight = &light;

			Ray shadowRay{ info.closestHit.origin, info.hitToLight, .0001f, info.hitToLightDistance };
			
			// if shadow ray doesn't hit anything, we have a clear view of the light
			// if shadows are not enabled, continue
			if ( !m_ShadowsEnabled || !pScene->DoesHit( std::move( shadowRay ) ) )
			{
				info.observedAreaMeasure = Vector3::Dot( info.closestHit.normal, info.hitToLight );
				if ( info.observedAreaMeasure >= 0.f )
				{
					// Set material and call the lighting function to obtain pixel color
					info.pMaterial = pScene->GetMaterials( )[info.closestHit.materialIndex];
					m_LightingFn( info, finalColor );
				}
			}
		}
	}

	//Normalize color and update Buffer
	finalColor.MaxToOne( );

	// Update Color in Buffer
	UpdateBuffer( finalColor, &m_pBufferPixels[pixelIdx] );
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
		SetLightingMode( LightingMode( static_cast<int>( 0 ) ) );
	}
	else
	{
		SetLightingMode( LightingMode( static_cast<int>( m_LightingMode ) + 1 ) );
	}
}

LightingMode dae::Renderer::GetLightingMode( )
{
	return m_LightingMode;
}

inline void dae::Renderer::ScreenToNDC( float& x, float& y, int px, int py, float fov ) const
{
	x = ( 2.f * ( px + 0.5f ) / m_Width - 1.f ) * m_AspectRatio * fov;
	y = ( 1.f - 2.f * ( py + 0.5f ) / m_Height ) * fov;
}

void dae::Renderer::ObservedAreaLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const
{
	finalColor += ColorRGB{ 1.f, 1.f, 1.f } * info.observedAreaMeasure;
}

void dae::Renderer::RadianceLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const
{
	finalColor += LightUtils::GetRadiance( *info.pLight, powf( info.hitToLightDistance, 2 ) );
}

void dae::Renderer::BRDFLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const
{
	finalColor += info.pMaterial->Shade( info.closestHit, info.hitToLight, -info.hitRay.direction );
}

void dae::Renderer::BVHLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const
{
	CombinedLightingFn( info, finalColor );
}

void dae::Renderer::CombinedLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const
{
	const ColorRGB Ergb{ LightUtils::GetRadiance( *info.pLight, powf( info.hitToLightDistance, 2 ) ) };
	const ColorRGB BRDFrgb{ info.pMaterial->Shade( info.closestHit, info.hitToLight, -info.hitRay.direction ) };

	finalColor += Ergb * BRDFrgb * info.observedAreaMeasure;
}

void dae::Renderer::UpdateBuffer( dae::ColorRGB& finalColor, uint32_t* const pBufferHead ) const
{
	// Cap the color to 0-1
	finalColor.MaxToOne( );

	( *pBufferHead ) = SDL_MapRGB( m_pBuffer->format,
		static_cast<uint8_t>( finalColor.r * 255 ),
		static_cast<uint8_t>( finalColor.g * 255 ),
		static_cast<uint8_t>( finalColor.b * 255 ) );
}

void dae::Renderer::SetLightingMode( LightingMode mode )
{
	m_LightingMode = mode;
	switch ( m_LightingMode )
	{
	case LightingMode::ObservedArea:
		m_LightingFn = std::bind( &Renderer::ObservedAreaLightingFn, this, std::placeholders::_1, std::placeholders::_2 );
		break;
	case LightingMode::Radiance:
		m_LightingFn = std::bind( &Renderer::RadianceLightingFn, this, std::placeholders::_1, std::placeholders::_2 );
		break;
	case LightingMode::BRDF:
		m_LightingFn = std::bind( &Renderer::BRDFLightingFn, this, std::placeholders::_1, std::placeholders::_2 );
		break;
	case LightingMode::BVH:
		m_LightingFn = std::bind( &Renderer::BVHLightingFn, this, std::placeholders::_1, std::placeholders::_2 );
		break;
	case LightingMode::Combined:
		m_LightingFn = std::bind( &Renderer::CombinedLightingFn, this, std::placeholders::_1, std::placeholders::_2 );
		break;
	}
}
