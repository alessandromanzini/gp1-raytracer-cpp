#pragma once
#include <stdexcept>
#include <vector>

#include "Maths.h"

namespace dae
{
#pragma region GEOMETRY
	struct Sphere
	{
		Vector3 origin{};
		float radius{};

		unsigned char materialIndex{ 0 };
	};

	struct Plane
	{
		Vector3 origin{};
		Vector3 normal{};

		unsigned char materialIndex{ 0 };
	};

	enum class TriangleCullMode
	{
		FrontFaceCulling,
		BackFaceCulling,
		NoCulling
	};

	struct Triangle
	{
		Triangle() = default;
		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2, const Vector3& _normal) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }, normal{ _normal.Normalized() } {}

		Triangle(const Vector3& _v0, const Vector3& _v1, const Vector3& _v2) :
			v0{ _v0 }, v1{ _v1 }, v2{ _v2 }
		{
			const Vector3 edgeV0V1 = v1 - v0;
			const Vector3 edgeV0V2 = v2 - v0;
			normal = Vector3::Cross(edgeV0V1, edgeV0V2).Normalized();
		}

		Vector3 v0{};
		Vector3 v1{};
		Vector3 v2{};

		Vector3 normal{};

		TriangleCullMode cullMode{};
		unsigned char materialIndex{};
	};

	struct TriangleMesh
	{
		TriangleMesh() = default;
		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), cullMode(_cullMode)
		{
			//Calculate Normals
			CalculateNormals();

			//Update Transforms
			UpdateTransforms();
		}

		TriangleMesh(const std::vector<Vector3>& _positions, const std::vector<int>& _indices, const std::vector<Vector3>& _normals, TriangleCullMode _cullMode) :
			positions(_positions), indices(_indices), normals(_normals), cullMode(_cullMode)
		{
			UpdateTransforms();
		}

		std::vector<Vector3> positions{};
		std::vector<Vector3> normals{};
		std::vector<int> indices{};
		unsigned char materialIndex{};

		TriangleCullMode cullMode{ TriangleCullMode::BackFaceCulling };

		Matrix rotationTransform{};
		Matrix translationTransform{};
		Matrix scaleTransform{};

		std::vector<Vector3> transformedPositions{};
		std::vector<Vector3> transformedNormals{};

		void Translate(const Vector3& translation)
		{
			translationTransform = Matrix::CreateTranslation(translation);
		}

		void RotateY(float yaw)
		{
			rotationTransform = Matrix::CreateRotationY(yaw);
		}

		void Scale(const Vector3& scale)
		{
			scaleTransform = Matrix::CreateScale(scale);
		}

		void AppendTriangle(const Triangle& triangle, bool ignoreTransformUpdate = false)
		{
			int startIndex = static_cast<int>(positions.size());

			positions.push_back(triangle.v0);
			positions.push_back(triangle.v1);
			positions.push_back(triangle.v2);

			indices.push_back(startIndex);
			indices.push_back(++startIndex);
			indices.push_back(++startIndex);

			normals.push_back(triangle.normal);

			//Not ideal, but making sure all vertices are updated
			if (!ignoreTransformUpdate)
				UpdateTransforms();
		}

		void CalculateNormals()
		{
			// We clear the vector before recalculating the normals
			normals.clear( );

			for ( int i{}; i < indices.size(); ++i )
			{
				const auto index0 = indices[i];
				const auto index1 = indices[++i];
				const auto index2 = indices[++i];

				const auto v0 = positions[index0];
				const auto v1 = positions[index1];
				const auto v2 = positions[index2];

				const auto edgeV0V1 = v1 - v0;
				const auto edgeV0V2 = v2 - v0;

				const auto normal = Vector3::Cross( edgeV0V1, edgeV0V2 ).Normalized( );

				normals.push_back( normal );
			}
		}

		void UpdateTransforms()
		{
			//Calculate Final Transform 
			//const auto trsMatrix{ translationTransform * rotationTransform * scaleTransform };
			
			//We actually use an RTS matrix instead because we're looking to do orbital rotations
			const auto rtsMatrix{ rotationTransform * translationTransform * scaleTransform };

			//Clear Transformed Positions/Normals and reserve space
			transformedPositions.clear( );
			transformedPositions.reserve( positions.size( ) );
			transformedNormals.clear( );
			transformedNormals.reserve( normals.size( ) );

			//Transform Positions (positions > transformedPositions)
			for ( int i{}; i < positions.size( ); ++i )
			{
				transformedPositions.emplace_back( rtsMatrix.TransformPoint( positions[i] ) );
			}

			//Transform Normals (normals > transformedNormals)
			for ( int i{}; i < normals.size( ); ++i )
			{
				transformedNormals.emplace_back( rtsMatrix.TransformVector( normals[i] ).Normalized( ) );
			}
		}
	};
#pragma endregion
#pragma region LIGHT
	enum class LightType
	{
		Point,
		Directional
	};

	struct Light
	{
		Vector3 origin{};
		Vector3 direction{};
		ColorRGB color{};
		float intensity{};

		LightType type{};
	};
#pragma endregion
#pragma region MISC
	struct Ray
	{
		Vector3 origin{};
		Vector3 direction{};

		float min{ 0.0001f };
		float max{ FLT_MAX };
	};

	struct HitRecord
	{
		Vector3 origin{};
		Vector3 normal{};
		float t = FLT_MAX;

		bool didHit{ false };
		unsigned char materialIndex{ 0 };
	};

	class Material;
	struct LightingInfo
	{
		Vector3 rayDirection;
		Ray hitRay;

		HitRecord closestHit;

		const Light* pLight;
		Vector3 hitToLight;

		Material* pMaterial;

		float observedAreaMeasure;
	};
#pragma endregion
}