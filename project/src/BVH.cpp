#include "DataTypes.h"

using namespace dae;

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
	root.triCount = m_Triangles.size();

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

void MeshBVHNodeBuilder::Subdivide( BVHNode bvhNode[], uint32_t nodeIdx )
{
	// terminate recursion
	BVHNode& node = bvhNode[nodeIdx];
	if ( node.triCount <= 2 ) return;

	// determine split axis and position
	Vector3 extent = node.aabbMax - node.aabbMin;
	uint8_t axis = 0;
	if ( extent.y > extent.x ) axis = 1;
	if ( extent.z > extent[axis] ) axis = 2;
	float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

	uint32_t i = node.leftFirst;
	uint32_t j = i + node.triCount - 1;
	while ( i <= j )
	{
		if ( m_Triangles[GetLookupIdx(i)].centroid[axis] < splitPos )
		{
			i++;
		}
		else
		{
			int from = i*3, to = j*3;
			std::swap( m_Indices[from], m_Indices[to] );
			std::swap( m_Indices[from+1], m_Indices[to+1] );
			std::swap( m_Indices[from+2], m_Indices[to+2] );
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

uint32_t dae::MeshBVHNodeBuilder::GetLookupIdx( uint32_t idx ) const
{
	return m_Indices[idx*3];
}
