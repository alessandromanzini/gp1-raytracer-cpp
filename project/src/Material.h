#pragma once
#include "Maths.h"
#include "DataTypes.h"
#include "BRDFs.h"

#define USE_REFLECTIONS

namespace dae
{
#pragma region Material BASE
	class Material
	{
	public:
		Material( ) = default;
		virtual ~Material( ) = default;

		Material( const Material& ) = delete;
		Material( Material&& ) noexcept = delete;
		Material& operator=( const Material& ) = delete;
		Material& operator=( Material&& ) noexcept = delete;

		/**
		 * \brief Function used to calculate the correct color for the specific material and its parameters
		 * \param hitRecord current hitrecord
		 * \param l light direction
		 * \param v view direction
		 * \return color
		 */
		virtual ColorRGB Shade( ShadeInfo& shadeInfo, const HitRecord& hitRecord = {}, const Vector3& l = {}, const Vector3& v = {} ) = 0;
	};
#pragma endregion

#pragma region Material SOLID COLOR
	//SOLID COLOR
	//===========
	class Material_SolidColor final : public Material
	{
	public:
		Material_SolidColor( const ColorRGB& color ) : m_Color( color )
		{
		}

		ColorRGB Shade( ShadeInfo& shadeInfo, const HitRecord& hitRecord, const Vector3& l, const Vector3& v ) override
		{
			return m_Color;
		}

	private:
		ColorRGB m_Color{ colors::White };
	};
#pragma endregion

#pragma region Material LAMBERT
	//LAMBERT
	//=======
	class Material_Lambert final : public Material
	{
	public:
		Material_Lambert( const ColorRGB& diffuseColor, float diffuseReflectance ) :
			m_DiffuseColor( diffuseColor ), m_DiffuseReflectance( diffuseReflectance )
		{
		}

		ColorRGB Shade( ShadeInfo& shadeInfo, const HitRecord& hitRecord = {}, const Vector3& l = {}, const Vector3& v = {} ) override
		{
			return BRDF::Lambert( m_DiffuseReflectance, m_DiffuseColor );
		}

	private:
		ColorRGB m_DiffuseColor{ colors::White };
		float m_DiffuseReflectance{ 1.f }; //kd
	};
#pragma endregion

#pragma region Material LAMBERT PHONG
	//LAMBERT-PHONG
	//=============
	class Material_LambertPhong final : public Material
	{
	public:
		Material_LambertPhong( const ColorRGB& diffuseColor, float kd, float ks, float phongExponent ) :
			m_DiffuseColor( diffuseColor ), m_DiffuseReflectance( kd ), m_SpecularReflectance( ks ),
			m_PhongExponent( phongExponent )
		{
		}

		ColorRGB Shade( ShadeInfo& shadeInfo, const HitRecord& hitRecord = {}, const Vector3& l = {}, const Vector3& v = {} ) override
		{
			// BRDF Linearity
			return BRDF::Lambert( m_DiffuseReflectance, m_DiffuseColor )
				+ BRDF::Phong( m_SpecularReflectance, m_PhongExponent, l, v, hitRecord.normal );
		}

	private:
		ColorRGB m_DiffuseColor{ colors::White };
		float m_DiffuseReflectance{ 0.5f }; //kd
		float m_SpecularReflectance{ 0.5f }; //ks
		float m_PhongExponent{ 1.f }; //Phong Exponent
	};
#pragma endregion

#pragma region Material COOK TORRENCE
	//COOK TORRENCE
	class Material_CookTorrence final : public Material
	{
	public:
		Material_CookTorrence( const ColorRGB& albedo, float metalness, float roughness ) :
			m_Albedo( albedo ), m_Metalness( metalness ), m_Roughness( roughness )
		{
		}

		ColorRGB Shade( ShadeInfo& shadeInfo, const HitRecord& hitRecord = {}, const Vector3& l = {}, const Vector3& v = {} ) override
		{
			const Vector3 h{ ( l + v ).Normalized( ) };

			const auto fresnel{ BRDF::FresnelFunction_Schlick( h, v, ( m_Metalness == 0.f ? ColorRGB{ .04f, .04f, .04f } : m_Albedo ) ) };
			const auto normalDistribution{ BRDF::NormalDistribution_GGX( hitRecord.normal, h, m_Roughness ) };
			const auto geometricAttenuation{ BRDF::GeometryFunction_Smith( hitRecord.normal, v, l, m_Roughness ) };

			const auto reflipCoefficient{ 1 / ( 4 * Vector3::Dot( v, hitRecord.normal ) * Vector3::Dot( l, hitRecord.normal ) ) };

			const auto specular{ normalDistribution * geometricAttenuation * reflipCoefficient * fresnel };
			const auto diffuse{ m_Metalness == 1.f ? ColorRGB{} : BRDF::Lambert( ColorRGB{ 1.f, 1.f, 1.f } - specular, m_Albedo ) };

#ifdef USE_REFLECTIONS
			if ( m_Metalness == 1.f )
			{
				shadeInfo.needsBounce = true;
				shadeInfo.reflectionRay = { hitRecord.origin + hitRecord.normal * .0001f, Vector3::Reflect( -v, hitRecord.normal ) };
				shadeInfo.reflectance = powf(1.f - m_Roughness, 2);
			}
#endif

			return specular + diffuse;
		}

	private:
		ColorRGB m_Albedo{ 0.955f, 0.637f, 0.538f }; //Copper
		float m_Metalness{ 1.0f };
		float m_Roughness{ 0.1f }; // [1.0 > 0.0] >> [ROUGH > SMOOTH]
	};
#pragma endregion
}
