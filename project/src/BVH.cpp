#include "DataTypes.h"

using namespace dae;

BVHNodeBuilder::BVHNodeBuilder( const std::vector<Vector3>& positions, const std::vector<int>& indices ) :
	m_Positions{ positions },
	m_Indices{ indices }
{
}

void BVHNodeBuilder::BuildBVH( BVHNode bvhNode[] )
{
	m_NodesUsed = 1;

	// Initialize triangles
	m_Triangles.reserve( m_Indices.size( ) / 3 );
	for ( int i = 0; i < m_Indices.size( ); i += 3 )
	{
		m_Triangles.push_back( { m_Positions[m_Indices[i]], m_Positions[m_Indices[i + 1]], m_Positions[m_Indices[i + 2]] } );
	}

	// assign all triangles to root node
	BVHNode& root = bvhNode[m_RootNodeIdx];
	root.leftNode = 0;
	root.firstTriIdx = 0;
	root.triCount = m_Triangles.size();

	if ( root.triCount == 0 ) return;

	UpdateNodeBounds( bvhNode, m_RootNodeIdx );

	// subdivide recursively
	Subdivide( bvhNode, m_RootNodeIdx );
}

void BVHNodeBuilder::UpdateNodeBounds( BVHNode bvhNode[], uint64_t nodeIdx )
{
	BVHNode& node = bvhNode[nodeIdx];

	node.aabbMin = m_Triangles[node.firstTriIdx].v0;
	node.aabbMax = m_Triangles[node.firstTriIdx].v0;
	for ( uint64_t first = node.firstTriIdx, i = 0; i < node.triCount; i++ )
	{
		dae::Triangle& leafTri{ m_Triangles[GetLookupIdx( first + i )] };
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v0 );
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v1 );
		node.aabbMin = Vector3::Min( node.aabbMin, leafTri.v2 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v0 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v1 );
		node.aabbMax = Vector3::Max( node.aabbMax, leafTri.v2 );
	}
}

void BVHNodeBuilder::Subdivide( BVHNode bvhNode[], uint64_t nodeIdx )
{
	// terminate recursion
	BVHNode& node = bvhNode[nodeIdx];
	if ( node.triCount <= 2 ) return;

	// determine split axis and position
	Vector3 extent = node.aabbMax - node.aabbMin;
	int axis = 0;
	if ( extent.y > extent.x ) axis = 1;
	if ( extent.z > extent[axis] ) axis = 2;
	float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

	int i = node.firstTriIdx;
	int j = i + node.triCount - 1;
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
	int leftCount = i - node.firstTriIdx;
	if ( leftCount == 0 || leftCount == node.triCount ) return;
	// create child nodes
	int leftChildIdx = m_NodesUsed++;
	int rightChildIdx = m_NodesUsed++;
	bvhNode[leftChildIdx].firstTriIdx = node.firstTriIdx;
	bvhNode[leftChildIdx].triCount = leftCount;
	bvhNode[rightChildIdx].firstTriIdx = i;
	bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
	node.leftNode = leftChildIdx;
	node.triCount = 0;
	UpdateNodeBounds( bvhNode, leftChildIdx );
	UpdateNodeBounds( bvhNode, rightChildIdx );
	// recurse
	Subdivide( bvhNode, leftChildIdx );
	Subdivide( bvhNode, rightChildIdx );
}

int dae::BVHNodeBuilder::GetLookupIdx( int idx ) const
{
	return m_Indices[idx*3];
}
