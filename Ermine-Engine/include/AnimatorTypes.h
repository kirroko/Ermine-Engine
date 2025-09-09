/* Start Header ************************************************************************/
/*!
\file       AnimatorTypes.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the declaration of the animator core.
            This file is used to contain all the core types of the animator.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace Ermine
{
	// Vector3
	struct Vector3
	{
		float x, y, z;
		static Vector3 zero() { return { 0,0,0 }; }
		static Vector3 one() { return { 1,1,1 }; }
	};

	// Quat
	struct Quat
	{
		float x, y, z, w;
		static Quat identity() { return { 0,0,0,1 }; }
		Quat inverse() const { return { -x, -y, -z, w }; }

		// Normalize to unit quaternion
		Quat normalized() const
		{
			float len = std::sqrt(x * x + y * y + z * z + w * w);
			if (len <= 1e-8f) return Quat::identity();
			float inv = 1.0f / len;
			return { x * inv, y * inv, z * inv, w * inv };
		}

		// Spherical linear interpolation
		static Quat slerp(const Quat& a, const Quat& b, float t)
		{
			// Compute dot product
			float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

			// If dot < 0, negate one quaternion to take shortest path
			Quat b2 = b;
			if (dot < 0.0f) {
				dot = -dot;
				b2 = { -b.x, -b.y, -b.z, -b.w };
			}

			// If quaternions are close, fall back to lerp
			const float EPSILON = 1e-6f;
			if (dot > 1.0f - EPSILON) {
				// Linear interpolation + normalize
				Quat result = {
					a.x + t * (b2.x - a.x),
					a.y + t * (b2.y - a.y),
					a.z + t * (b2.z - a.z),
					a.w + t * (b2.w - a.w)
				};
				return result.normalized();
			}

			// Standard SLERP
			float theta_0 = std::acos(dot);        // angle between input quats
			float theta = theta_0 * t;           // scaled angle
			float sin_theta = std::sin(theta);
			float sin_theta_0 = std::sin(theta_0);

			float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
			float s1 = sin_theta / sin_theta_0;

			return {
				(s0 * a.x) + (s1 * b2.x),
				(s0 * a.y) + (s1 * b2.y),
				(s0 * a.z) + (s1 * b2.z),
				(s0 * a.w) + (s1 * b2.w)
			};
		}

		Quat operator*(const Quat& o) const { return *this; }
	};

	// Transform
	struct Trans
	{
		Vector3 t;
		Quat r;
		Vector3 s;
		static Trans Identity() { return { Vector3::zero(), Quat::identity(), Vector3::one() }; }
	};

	// Lerp
	inline Trans Lerp(const Trans& a, const Trans& b, float t)
	{
		Trans r;
		r.t = { a.t.x * (1.0f - t) + b.t.x * t, a.t.y * (1.0f - t) + b.t.y * t, a.t.z * (1.0f - t) + b.t.z * t };
		r.r = Quat::slerp(a.r, b.r, t);
		r.s = { a.s.x * (1.0f - t) + b.s.x * t, a.s.y * (1.0f - t) + b.s.y * t, a.s.z * (1.0f - t) + b.s.z * t };
		return r;
	}

	// Animation Clip
	struct AnimationClip
	{
		// Forward declaration for clip sampling.
		// Your importer should provide a clip type.

		std::string name;
		float duration = 0.0f; // seconds
		int bone_count = 0;

		// Must be provided by the clip implementation of the pipeline:
		Trans sampleBone(int boneIndex, float timeSeconds) const
		{
			return Trans{};
		}
	};

	// Parameters
	enum class ParamType { Bool, Float, Trigger };
	struct ParamVal
	{
		ParamType type = ParamType::Float;
		union { bool b; float f; };
		bool trigger_consumed = false; // for trigger semantics
		ParamVal() { f = 0.0f; }
	};

	// Conditions
	enum class CompareOp { Greater, Less, Equals, True, False, Changed };
	struct Condition
	{
		std::string param;
		CompareOp op = CompareOp::Equals;
		float threshold = 0.0f;
	};

	// State
	struct State
	{
		std::string name;
		std::shared_ptr<AnimationClip> clip;
		float speed = 1.0f;
		bool loop = true;
	};

	// Transition
	struct Transition
	{
		int from_state = -1; // -1 == ANY
		int to_state = -1;
		float duration = 0.2f;
		bool has_exit_time = false;
		float exit_normalized_time = 1.0f;
		bool can_interrupt = false;
		std::vector<Condition> conditions;
	};

	// Runtime player for a state
	struct Player
	{
		int state_index = -1;
		float time_seconds = 0.0f;
	};

	// Layer
	enum class LayerBlendMode { Override, Additive };
	struct Layer
	{
		std::string name;
		std::vector<State> states;
		std::vector<Transition> transitions;
		Player current;
		LayerBlendMode blend_mode = LayerBlendMode::Override;
		float weight = 1.0f; // 0..1
		// boneMask: byte per bone (0/1). Length == skeleton bone count. If empty => affect all bones.
		std::vector<uint8_t> bone_mask;
	};
}
