#include "DataTypes.h"

using namespace dae;

struct aabb
{
	Vector3 bmin = { 1e30f, 1e30f, 1e30f }, bmax = { -1e30f, -1e30f, -1e30f };
	void grow( const Vector3& p )
	{
		bmin = Vector3::Min( bmin, p );
		bmax = Vector3::Max( bmax, p );
	}

	void grow( const aabb& other )
	{
		bmin = Vector3::Min( bmin, other.bmin );
		bmax = Vector3::Max( bmax, other.bmax );
	}

	float area( )
	{
		Vector3 e = bmax - bmin; // box extent
		return e.x * e.y + e.y * e.z + e.z * e.x;
	}
};

struct Bin
{
	aabb bounds; 
	uint32_t triCount = 0;
};

constexpr int BINS = 8;

MeshBVHNodeBuilder::MeshBVHNodeBuilder( const std::vector<Vector3>& positions, const std::vector<uint32_t>& indices ) :
	m_Positions{ positions },
	m_Indices{ indices }
{
}

void MeshBVHNodeBuilder::BuildBVH( BVHNode bvhNode[] )
{
	m_NodesUsed = 1;

	// Initialize triangles
	m_Triangles.reserve( m_Indices.size( ) / 3 );
	for ( uint32_t i = 0; i < m_Indices.size( ); i += 3 )
	{
		m_Triangles.push_back( { m_Positions[m_Indices[i]], m_Positions[m_Indices[i + 1]], m_Positions[m_Indices[i + 2]] } );
	}

	// assign all triangles to root node
	BVHNode& root = bvhNode[m_RootNodeIdx];
	/*root.leftNode = 0;
	root.firstTriIdx = 0;*/
	root.leftFirst = 0;
	root.triCount = m_Triangles.size( );

	if ( root.triCount == 0 ) return;

	UpdateNodeBounds( bvhNode, m_RootNodeIdx );

	// subdivide recursively
	Subdivide( bvhNode, m_RootNodeIdx );
}

void MeshBVHNodeBuilder::UpdateNodeBounds( BVHNode bvhNode[], uint32_t nodeIdx )
{
	BVHNode& node = bvhNode[nodeIdx];

	node.aabbMin = m_Triangles[node.leftFirst].v0;
	node.aabbMax = m_Triangles[node.leftFirst].v0;
	for ( uint32_t first = node.leftFirst, i = 0; i < node.triCount; i++ )
	{
		CentroidTriangle& leafTri{ m_Triangles[GetLookupIdx( first + i )] };
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v0 );
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v1 );
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v2 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v0 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v1 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v2 );
	}
}

//void MeshBVHNodeBuilder::Subdivide( BVHNode bvhNode[], uint32_t nodeIdx )
//{
//	// terminate recursion
//	BVHNode& node = bvhNode[nodeIdx];
//
//	// determine split axis using SAH
//	int bestAxis = -1;
//	float bestPos = 0, bestCost = 1e30f;
//	for ( int axis = 0; axis < 3; axis++ ) for ( uint32_t i = 0; i < node.triCount; i++ )
//	{
//		CentroidTriangle& triangle = m_Triangles[GetLookupIdx( node.leftFirst + i )];
//		float candidatePos = triangle.centroid[axis];
//		float cost = EvaluateSAH( node, axis, candidatePos );
//		if ( cost < bestCost )
//			bestPos = candidatePos, bestAxis = axis, bestCost = cost;
//	}
//	int axis = bestAxis;
//	float splitPos = bestPos;
//
//	Vector3 e = node.aabbMax - node.aabbMin; // extent of parent
//	float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
//	float parentCost = node.triCount * parentArea;
//	if ( bestCost >= parentCost ) return;
//
//	uint32_t i = node.leftFirst;
//	uint32_t j = i + node.triCount - 1;
//	while ( i <= j )
//	{
//		if ( m_Triangles[GetLookupIdx( i )].centroid[axis] < splitPos )
//		{
//			i++;
//		}
//		else
//		{
//			int from = i * 3, to = j * 3;
//			std::swap( m_Indices[from], m_Indices[to] );
//			std::swap( m_Indices[from + 1], m_Indices[to + 1] );
//			std::swap( m_Indices[from + 2], m_Indices[to + 2] );
//			--j;
//		}
//	}
//
//	// abort split if one of the sides is empty
//
//	uint32_t leftCount = i - node.leftFirst;
//	if ( leftCount == 0 || leftCount == node.triCount ) return;
//	// create child nodes
//	uint32_t leftChildIdx = m_NodesUsed++;
//	uint32_t rightChildIdx = m_NodesUsed++;
//	bvhNode[leftChildIdx].leftFirst = node.leftFirst;
//	bvhNode[leftChildIdx].triCount = leftCount;
//	bvhNode[rightChildIdx].leftFirst = i;
//	bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
//	node.leftFirst = leftChildIdx;
//	node.triCount = 0;
//	UpdateNodeBounds( bvhNode, leftChildIdx );
//	UpdateNodeBounds( bvhNode, rightChildIdx );
//	// recurse
//	Subdivide( bvhNode, leftChildIdx );
//	Subdivide( bvhNode, rightChildIdx );
//}

void MeshBVHNodeBuilder::Subdivide( BVHNode bvhNode[], uint32_t nodeIdx )
{
	// terminate recursion
	BVHNode& node = bvhNode[nodeIdx];

	// determine split axis using SAH
	uint8_t axis;
	float splitPos;
	float splitCost = FindBestSplitPlane( node, axis, splitPos );

	Vector3 e = node.aabbMax - node.aabbMin; // extent of parent
	float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
	float parentCost = node.triCount * parentArea;
	
	float nosplitCost = CalculateNodeCost( node );
	if ( splitCost >= nosplitCost ) return;

	uint32_t i = node.leftFirst;
	uint32_t j = i + node.triCount - 1;
	while ( i <= j )
	{
		if ( m_Triangles[GetLookupIdx( i )].centroid[axis] < splitPos )
		{
			i++;
		}
		else
		{
			int from = i * 3, to = j * 3;
			std::swap( m_Indices[from], m_Indices[to] );
			std::swap( m_Indices[from + 1], m_Indices[to + 1] );
			std::swap( m_Indices[from + 2], m_Indices[to + 2] );
			--j;
		}
	}

	// abort split if one of the sides is empty

	uint32_t leftCount = i - node.leftFirst;
	if ( leftCount == 0 || leftCount == node.triCount ) return;
	// create child nodes
	uint32_t leftChildIdx = m_NodesUsed++;
	uint32_t rightChildIdx = m_NodesUsed++;
	bvhNode[leftChildIdx].leftFirst = node.leftFirst;
	bvhNode[leftChildIdx].triCount = leftCount;
	bvhNode[rightChildIdx].leftFirst = i;
	bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
	node.leftFirst = leftChildIdx;
	node.triCount = 0;
	UpdateNodeBounds( bvhNode, leftChildIdx );
	UpdateNodeBounds( bvhNode, rightChildIdx );
	// recurse
	Subdivide( bvhNode, leftChildIdx );
	Subdivide( bvhNode, rightChildIdx );
}

float dae::MeshBVHNodeBuilder::EvaluateSAH( BVHNode& node, uint8_t axis, float pos )
{
	// determine triangle counts and bounds for this split candidate
	aabb leftBox, rightBox;
	uint32_t leftCount = 0, rightCount = 0;
	for ( uint32_t i = 0; i < node.triCount; i++ )
	{
		CentroidTriangle& triangle = m_Triangles[GetLookupIdx( node.leftFirst + i )];
		if ( triangle.centroid[axis] < pos )
		{
			leftCount++;
			leftBox.grow( triangle.v0 );
			leftBox.grow( triangle.v1 );
			leftBox.grow( triangle.v2 );
		}
		else
		{
			rightCount++;
			rightBox.grow( triangle.v0 );
			rightBox.grow( triangle.v1 );
			rightBox.grow( triangle.v2 );
		}
	}
	float cost = leftCount * leftBox.area( ) + rightCount * rightBox.area( );
	return cost > 0 ? cost : 1e30f;
}

//float dae::MeshBVHNodeBuilder::FindBestSplitPlane( BVHNode& node, uint8_t& axis, float& splitPos )
//{
//	float bestCost = 1e30f;
//	for ( int a = 0; a < 3; a++ ) for ( uint32_t i = 0; i < node.triCount; i++ )
//	{
//		CentroidTriangle& triangle = m_Triangles[GetLookupIdx( node.leftFirst + i )];
//		float candidatePos = triangle.centroid[a];
//		float cost = EvaluateSAH( node, a, candidatePos );
//		if ( cost < bestCost ) splitPos = candidatePos, axis = a, bestCost = cost;
//	}
//	return bestCost;
//}

float dae::MeshBVHNodeBuilder::FindBestSplitPlane( BVHNode& node, uint8_t& axis, float& splitPos )
{
	float bestCost = 1e30f;
	for ( int a = 0; a < 3; a++ )
	{
		float boundsMin = 1e30f, boundsMax = -1e30f;
		for ( int i = 0; i < node.triCount; i++ )
		{
			CentroidTriangle& triangle = m_Triangles[GetLookupIdx( node.leftFirst + i )];
			boundsMin = std::min( boundsMin, triangle.centroid[a] );
			boundsMax = std::max( boundsMax, triangle.centroid[a] );
		}
		if ( boundsMin == boundsMax ) continue;
		// populate the bins
		Bin bin[BINS];
		float scale = BINS / ( boundsMax - boundsMin );
		for ( uint32_t i = 0; i < node.triCount; i++ )
		{
			CentroidTriangle& triangle = m_Triangles[GetLookupIdx( node.leftFirst + i )];
			int binIdx = std::min( BINS - 1, (int) ( ( triangle.centroid[a] - boundsMin ) * scale ) );
			bin[binIdx].triCount++;
			bin[binIdx].bounds.grow( triangle.v0 );
			bin[binIdx].bounds.grow( triangle.v1 );
			bin[binIdx].bounds.grow( triangle.v2 );
		}
		// gather data for the 7 planes between the 8 bins
		float leftArea[BINS - 1], rightArea[BINS - 1];
		int leftCount[BINS - 1], rightCount[BINS - 1];
		aabb leftBox, rightBox;
		int leftSum = 0, rightSum = 0;
		for ( int i = 0; i < BINS - 1; i++ )
		{
			leftSum += bin[i].triCount;
			leftCount[i] = leftSum;
			leftBox.grow( bin[i].bounds );
			leftArea[i] = leftBox.area( );
			rightSum += bin[BINS - 1 - i].triCount;
			rightCount[BINS - 2 - i] = rightSum;
			rightBox.grow( bin[BINS - 1 - i].bounds );
			rightArea[BINS - 2 - i] = rightBox.area( );
		}
		// calculate SAH cost for the 7 planes
		scale = ( boundsMax - boundsMin ) / BINS;
		for ( int i = 0; i < BINS - 1; i++ )
		{
			float planeCost =
				leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
			if ( planeCost < bestCost )
				axis = a, splitPos = boundsMin + scale * ( i + 1 ),
				bestCost = planeCost;
		}
	}
	return bestCost;
}

float dae::MeshBVHNodeBuilder::CalculateNodeCost( BVHNode& node )
{
	Vector3 e = node.aabbMax - node.aabbMin; // extent of the node
	float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
	return node.triCount * surfaceArea;
}

uint32_t dae::MeshBVHNodeBuilder::GetLookupIdx( uint32_t idx ) const
{
	return m_Indices[idx * 3];
}
