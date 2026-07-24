#pragma once
#include <DirectXMath.h>
#include <string_view>
namespace rr
{
	struct NodeRelationship
	{
		int parent;
		int child_first;
		int sibling_next;
		int subtree_size;
	};

	struct NodeTransform
	{
		DirectX::XMFLOAT3 translation;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;
	};

	struct Primitive
	{
		int geometry;
		int material;
	};

	struct MeshInstance
	{
		int primitive_first;
		int primitive_count;
		int joint_first;
		int joint_count;
		int node;
	};

	struct Joint
	{
		int node;
	};

	struct AnimTrack
	{
		int target_node;
		int target_path; // 0 = translation, 1 = rotation, 2 = scale

		int keyframe_count;
		float* keyframe_times;
		DirectX::XMFLOAT4* keyframe_outputs;
	};
	struct AnimClip
	{
		std::string_view name;
		int track_first;
		int track_count;
		float duration;
	};

	struct Scene
	{
		// Node SOA
		NodeRelationship* node_relationships;
		NodeTransform* node_transforms;
		int node_count;

		// Primitive SOA
		Primitive* primitives;
		int        primitive_count;
		
		// Joint SOA 
		Joint* joints;
		DirectX::XMMATRIX* joint_ibms;
		int                joint_count;

		MeshInstance* mesh_instances;

		// Animation
		AnimTrack* anim_tracks;
		int        anim_track_count;

		AnimClip* anim_clips;
		int       anim_clip_count;
	};
}