#pragma once
#include <Engine/Material.h>
namespace rr
{
	struct NodeRelationship
	{
		int parent;
		int first_child;
		int next_sibling;
		int subtree_size;
	};

	struct NodeTransform
	{
		float translation[3];
		float rotation[4];
		float scale[3];
	};

	struct Scene
	{
		int node_count;
		NodeRelationship* node_relationships;
		NodeTransform* node_transforms;

		//MeshInstance* mesh_instances;
	};
}