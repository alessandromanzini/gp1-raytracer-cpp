#pragma once

#include <cstdint>
#include <functional>

#include "Scene.h"
#include "logging.hpp"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;

	enum class LightingMode
	{
		ObservedArea, // Lambert Cosine Law
		Radiance,	  // Incident Radiance
		BRDF,		  // Scattering of the light
		Combined	  // Combination of all
	};

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
		void ProcessRay( Scene* pScene, Ray ray, ColorRGB& finalColor, int bounce = 0 ) const;
		bool SaveBufferToImage( ) const;

		void ToggleShadows( );
		void ToggleLightingMode( );
		void ToggleGlobalIllumination( );
		void ToggleSoftShadows( );

		LightingMode GetLightingMode( );

		friend void LogSceneInfo( const Scene* pScene, const Renderer* pRenderer, float dFPS );
		
	private:
		LightingMode m_LightingMode{ LightingMode::Combined };
		std::function<void( ShadeInfo& shadeInfo, const LightingInfo&, ColorRGB& )> m_LightingFn{};
		bool m_ShadowsEnabled{ true };
		bool m_GlobalIlluminationEnabled{ false };
		bool m_SoftShadowsEnabled{ false };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		uint32_t* m_pPixelIndices{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio;

		//void ExecuteRenderCycle( dae::Scene* pScene ) const;
		inline void ScreenToNDC( float& x, float& y, int px, int py, float fov ) const;

		void ObservedAreaLightingFn( ShadeInfo& shadeInfo, const LightingInfo& info, ColorRGB& finalColor ) const;
		void RadianceLightingFn( ShadeInfo& shadeInfo, const LightingInfo& info, ColorRGB& finalColor ) const;
		void BRDFLightingFn( ShadeInfo& shadeInfo, const LightingInfo& info, ColorRGB& finalColor ) const;
		void CombinedLightingFn( ShadeInfo& shadeInfo, const LightingInfo& info, ColorRGB& finalColor ) const;

		void RenderSoftShadows( Scene* pScene, LightingInfo& info ) const;

		void UpdateBuffer( dae::ColorRGB& finalColor, uint32_t* const pBufferHead ) const;

		void SetLightingMode( LightingMode mode );
	};
}
