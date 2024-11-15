#pragma once
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"
#include "SquirrelNoise5.hpp"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 sphereToRay{ sphere.origin, ray.origin };
			const float a{ ray.direction.SqrMagnitude() }, 
				b{ Vector3::Dot( 2 * ray.direction, sphereToRay )},
				c{ sphereToRay.SqrMagnitude() - sphere.radius * sphere.radius };
			const float discriminant{ b * b - 4 * a * c };

			if ( discriminant > 0 )
			{
				// Get the closest hit point. If t < ray.min, then the hit point is behind the ray origin
				const float t1{ (-b - sqrtf( discriminant )) / (2 * a) };
				const float t2{ (-b + sqrtf( discriminant )) / (2 * a) };
				const float t{ (t1 < ray.min) ? t2 : t1 };

				if ( t >= ray.min && t < ray.max )
				{
					if ( !ignoreHitRecord )
					{
						hitRecord.didHit = true;
						hitRecord.materialIndex = sphere.materialIndex;
						hitRecord.t = t;
						hitRecord.origin = ray.origin + ray.direction * t;
						hitRecord.normal = Vector3{ sphere.origin, hitRecord.origin };
						hitRecord.normal.Normalize( );
					}
					return true;
				}
			}
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const Vector3 rayToPlane{ ray.origin, plane.origin };
			const float t{ Vector3::Dot(rayToPlane, plane.normal) / Vector3::Dot( ray.direction, plane.normal ) };
			
			// Checking interval [Tmin,Tmax)
			if ( t >= ray.min && t < ray.max )
			{
				if ( !ignoreHitRecord )
				{
					hitRecord.didHit = true;
					hitRecord.materialIndex = plane.materialIndex;
					hitRecord.origin = ray.origin;
					hitRecord.t = t;
					hitRecord.normal = plane.normal;
					hitRecord.origin = ray.origin + ray.direction * t;
				}
				return true;
			}
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//HELPER FUNCTION
		inline bool IsPointInsideEdge( const Vector3& v0, const Vector3& v1, const Vector3& hitPoint, const Vector3& n )
		{
			auto e{ v1 - v0 };
			auto p{ hitPoint - v0 };

			if ( Vector3::Dot( n, Vector3::Cross( e, p ) ) < 0.f )
			{
				// If the hit point is outside the triangle, it will never hit
				return false;
			}
			return true;
		}

		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			// Setup for shadow rays
			auto cullMode{ triangle.cullMode };
			if ( ignoreHitRecord )
			{
				if ( triangle.cullMode == TriangleCullMode::BackFaceCulling )
				{
					cullMode = TriangleCullMode::FrontFaceCulling;
				}
				else if ( triangle.cullMode == TriangleCullMode::FrontFaceCulling )
				{
					cullMode = TriangleCullMode::BackFaceCulling;
				}
			}

			const auto orthogonality{ Vector3::Dot( ray.direction, triangle.normal ) };
			if ( AreEqual(orthogonality, 0.f) 
				|| ( cullMode == TriangleCullMode::BackFaceCulling && orthogonality > 0.f )
				|| ( cullMode == TriangleCullMode::FrontFaceCulling && orthogonality < 0.f ) )
			{
				// If the ray is parallel to the triangle or the triangle should be culled, it doesn't hit
				return false;
			}

			const auto l{ triangle.v0 - ray.origin };
			const auto t{ Vector3::Dot( l, triangle.normal ) / orthogonality };
			if ( t < ray.min || t >= ray.max )
			{
				// if the hit point is behind the ray origin or outside the ray interval, it will never hit
				return false;
			}

			const auto hitPoint{ ray.origin + ray.direction * t };

			// Check if hitPoint is inside edges
			// If the hit point is outside the triangle, it will never hit
			if (   !IsPointInsideEdge( triangle.v0, triangle.v1, hitPoint, triangle.normal )
				|| !IsPointInsideEdge( triangle.v1, triangle.v2, hitPoint, triangle.normal )
				|| !IsPointInsideEdge( triangle.v2, triangle.v0, hitPoint, triangle.normal ) )
			{
				return false;
			}

			// The hit point is confirmed to be inside the triangle
			if ( !ignoreHitRecord )
			{
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;
				hitRecord.normal = triangle.normal;
				hitRecord.origin = hitPoint;
				hitRecord.t = t;
			}
			return true;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool SlabTest_TriangleMesh( const Vector3& minAABB, const Vector3& maxAABB, const Ray& ray )
		{
			const float tx1{ ( minAABB.x - ray.origin.x ) / ray.direction.x };
			const float tx2{ ( maxAABB.x - ray.origin.x ) / ray.direction.x };

			float tmin = std::min( tx1, tx2 );
			float tmax = std::max( tx1, tx2 );

			const float ty1{ ( minAABB.y - ray.origin.y ) / ray.direction.y };
			const float ty2{ ( maxAABB.y - ray.origin.y ) / ray.direction.y };

			tmin = std::max( tmin, std::min( ty1, ty2 ) );
			tmax = std::min( tmax, std::max( ty1, ty2 ) );

			const float tz1{ ( minAABB.z - ray.origin.z ) / ray.direction.z };
			const float tz2{ ( maxAABB.z - ray.origin.z ) / ray.direction.z };

			tmin = std::max( tmin, std::min( tz1, tz2 ) );
			tmax = std::min( tmax, std::max( tz1, tz2 ) );

			return tmax > 0 && tmax >= tmin;
		}

		inline bool BHV_TriangleMesh( BVHNode bvhNode[], const Ray& ray, const int nodeIdx = 0 )
		{
			BVHNode& node = bvhNode[nodeIdx];
			if ( !SlabTest_TriangleMesh( node.aabbMin, node.aabbMax, ray ) )
			{
				return false;
			}
			// If it's a leaf node and SlabTest was positive, ray is inside a triangle
			if ( node.isLeaf( ) )
			{
				return true;
			}
			else
			{
				// else dig deeper. quicksort type of algorithm
				bool confirm = BHV_TriangleMesh( bvhNode, ray, node.leftFirst ) ||
					BHV_TriangleMesh( bvhNode, ray, node.leftFirst + 1 );
				return confirm;
			}
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			// If BHV fails, exits
			if ( !BHV_TriangleMesh( mesh.pBVHRoot, ray ) )
			{
				return false;
			}

			HitRecord temp{};
			for ( int i{}; i < mesh.indices.size(); i+=3 )
			{
				Triangle triangle{ 
					mesh.transformedPositions[mesh.indices[i]], 
					mesh.transformedPositions[mesh.indices[i + 1]], 
					mesh.transformedPositions[mesh.indices[i + 2]], 
					mesh.transformedNormals[i / 3] 
				};
				triangle.cullMode = mesh.cullMode;
				triangle.materialIndex = mesh.materialIndex;

				if ( HitTest_Triangle( triangle, ray, temp, ignoreHitRecord ) )
				{
					if ( ignoreHitRecord )
					{
						return true;
					}

					if ( temp.t < hitRecord.t )
					{
						hitRecord = temp;
					}
				}
			}
			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3& origin)
		{
			switch ( light.type )
			{
			case LightType::Directional:
				return light.direction * FLT_MAX;
			case LightType::Point:
				return { origin, light.origin };
			default:
				throw std::runtime_error( "LightType not implemented yet." );
			}
		}

		inline ColorRGB GetRadiance(const Light& light, float sqrDistanceToLight)
		{
			switch ( light.type )
			{
			case LightType::Directional:
				return light.color * light.intensity;
			case LightType::Point:
				return light.color * light.intensity / sqrDistanceToLight;
			default:
				throw std::runtime_error( "LightType not implemented yet." );
			}
		}

		static unsigned int i{};
		inline Vector3 GetRandomPointInRadius( const Vector3& origin, const float& radius )
		{

			// uniform numbers in a sphere

			float u = Get1dNoiseZeroToOne( i );
			float theta = 2.0f * M_PI * Get1dNoiseZeroToOne( ++i );
			float phi = acos( 1.0f - 2.0f * u );

			// convert to cartesian coordinates
			float sinPhi = sin( phi );
			Vector3 randomPoint(
				sinPhi * cos( theta ),
				sinPhi * sin( theta ),
				cos( phi )
			);

			return origin + randomPoint * radius;

		}

		/*inline float CalculateLambertCosineLaw( const Light& light, const HitRecord& hitRecord )
		{
			Vector3 lightDirection{};
			switch ( light.type )
			{
			case LightType::Directional:
				lightDirection = light.direction;
				break;
			case LightType::Point:
				lightDirection = { light.origin, hitRecord.origin };
				lightDirection.Normalize( );
				break;
			}

			return Vector3::Dot( lightDirection, hitRecord.normal );
		}*/
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<uint32_t>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((uint32_t)i0 - 1);
					indices.push_back((uint32_t)i1 - 1);
					indices.push_back((uint32_t)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}