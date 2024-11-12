#pragma once

#include <cstdint>
#include <functional>

#include "Scene.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;

	class Renderer final
	{
	public:
		Renderer( SDL_Window* pWindow );
		~Renderer( );

		Renderer( const Renderer& ) = delete;
		Renderer( Renderer&& ) noexcept = delete;
		Renderer& operator=( const Renderer& ) = delete;
		Renderer& operator=( Renderer&& ) noexcept = delete;

		void Render( Scene* pScene ) const;
		void RenderPixel( Scene* pScene, uint32_t pixelIdx, const Camera& camera ) const;
		bool SaveBufferToImage( ) const;

		void ToggleShadows( );
		void ToggleLightingMode( );
		
	private:
		enum class LightingMode
		{
			ObservedArea, // Lambert Cosine Law
			Radiance,	  // Incident Radiance
			BRDF,		  // Scattering of the light
			Combined	  // Combination of all
		};

		LightingMode m_LightingMode{ LightingMode::Combined };
		std::function<void( const LightingInfo&, ColorRGB& )> m_LightingFn{};
		bool m_ShadowsEnabled{ true };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		uint32_t* m_pPixelIndices{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio;

		//void ExecuteRenderCycle( dae::Scene* pScene ) const;
		inline void ScreenToNDC( float& x, float& y, int px, int py, float fov ) const;

		void ObservedAreaLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const;
		void RadianceLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const;
		void BRDFLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const;
		void CombinedLightingFn( const LightingInfo& info, ColorRGB& finalColor ) const;

		void UpdateBuffer( dae::ColorRGB& finalColor, uint32_t* const pBufferHead ) const;

		void SetLightingMode( LightingMode mode );
	};
}
