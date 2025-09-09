/* Start Header ************************************************************************/
/*!
\file       Animator.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       04/09/2025
\brief      This file contains the definition of the animator.
            This file is used for the animator core logic.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Animator.h"

namespace Ermine
{
    Animator::Animator(int bone_count) { setSkeletonBoneCount(bone_count); }

    void Animator::setSkeletonBoneCount(int n)
    {
        skeleton_bone_count = n;
        base_pose.assign(n, Trans::Identity());
        out_pose.assign(n, Trans::Identity());
        blend_jobs.assign(0, BlendJob());
    }

    void Animator::setBasePose(const std::vector<Trans>& bind_pose)
    {
        base_pose = bind_pose;
        if ((int)out_pose.size() != (int)bind_pose.size())
            out_pose = bind_pose;
    }

    static bool paramExists(const std::unordered_map<std::string, ParamVal>& m, const std::string& k) { return m.find(k) != m.end(); }

    bool Animator::evalCondition(const Condition& c) const
    {
        auto it = params.find(c.param);
        if (it == params.end()) return false;
        const ParamVal& v = it->second;
        switch (c.op)
        {
        case CompareOp::Greater: return v.f > c.threshold;
        case CompareOp::Less: return v.f < c.threshold;
        case CompareOp::Equals: return fabs(v.f - c.threshold) < 1e-5f;
        case CompareOp::True: return v.b;
        case CompareOp::False: return !v.b;
        case CompareOp::Changed: return false; // not implemented
        }
        return false;
    }

    void Animator::advanceStateTime(Layer& layer, int state_index, float dt)
    {
        if (state_index < 0 || state_index >= (int)layer.states.size()) return;
        State& st = layer.states[state_index];
        Player& p = layer.current;
        if (p.state_index != state_index)
        {
            // new state -> reset player's time if necessary
            // but we keep only single player per layer in this simplified model
            p.state_index = state_index;
            p.time_seconds = 0.0f;
        }
        p.time_seconds += dt * st.speed;
        if (st.loop && st.clip && st.clip->duration > 1e-6f)
        {
            // wrap
            p.time_seconds = fmod(p.time_seconds, st.clip->duration);
            if (p.time_seconds < 0)
                p.time_seconds += st.clip->duration;
        }
        else if (st.clip)
        {
            if (p.time_seconds > st.clip->duration)
                p.time_seconds = st.clip->duration;
        }
    }

    void Animator::samplePoseInto(const Layer& layer, int state_index, std::vector<Trans>& out, float weight)
    {
        if (state_index < 0 || state_index >= (int)layer.states.size()) return;
        const State& st = layer.states[state_index];
        if (!st.clip) return;

        // compute sample time: we use the layer.current timeSeconds, but Player is per-layer
        float t = layer.current.time_seconds * st.speed;
        if (st.loop && st.clip->duration > 1e-6f)
        {
            t = fmod(t, st.clip->duration);
            if (t < 0)
                t += st.clip->duration;
        }
        else
            t = std::min(t, st.clip->duration);

        // ensure out size matches skeleton
        if ((int)out.size() != skeleton_bone_count)
            out.assign(skeleton_bone_count, Trans::Identity());

        for (int b = 0; b < skeleton_bone_count; ++b)
        {
            // clip->sampleBone must be implemented by your importer/clip type
            Trans sampled = st.clip->sampleBone(b, t);
            if (weight >= 0.9999f)
                out[b] = sampled;
            else
                out[b] = Lerp(out[b], sampled, weight);
        }
    }

    void Animator::applyLayerToOutPose(const Layer& layer, const std::vector<Trans>& layer_pose)
    {
        float lw = layer.weight;
        for (int b = 0; b < skeleton_bone_count; ++b)
        {
            bool affected = true;
            if (!layer.bone_mask.empty())
                if (b < (int)layer.bone_mask.size())
                    affected = layer.bone_mask[b] != 0;

            if (!affected) continue;

            if (layer.blend_mode == LayerBlendMode::Override)
                out_pose[b] = Lerp(out_pose[b], layer_pose[b], lw);
            else
            {   // Additive
                // translation/scale additive
                Trans diff;
                diff.t = { layer_pose[b].t.x - base_pose[b].t.x, layer_pose[b].t.y - base_pose[b].t.y, layer_pose[b].t.z - base_pose[b].t.z };
                diff.s = { layer_pose[b].s.x - base_pose[b].s.x, layer_pose[b].s.y - base_pose[b].s.y, layer_pose[b].s.z - base_pose[b].s.z };
                // rotation delta: q_delta = q_layer * inverse(q_base)
                Quat inv_base = base_pose[b].r.inverse();
                Quat dq = layer_pose[b].r * inv_base;

                out_pose[b].t.x += diff.t.x * lw; out_pose[b].t.y += diff.t.y * lw; out_pose[b].t.z += diff.t.z * lw;
                out_pose[b].s.x += diff.s.x * lw; out_pose[b].s.y += diff.s.y * lw; out_pose[b].s.z += diff.s.z * lw;

                // apply rotation: interpolate from identity to dq then multiply to existing rotation
                Quat step = Quat::slerp(Quat::identity(), dq, lw);
                out_pose[b].r = step * out_pose[b].r;
            }
        }
    }

    void Animator::update(float dt)
    {
        // ensure blend_jobs size matches layers
        blend_jobs.resize(layers.size());

        // initialize out_pose with base_pose
        if ((int)out_pose.size() != skeleton_bone_count)
            out_pose.assign(skeleton_bone_count, Trans::Identity());
        for (int b = 0; b < skeleton_bone_count; ++b)
            out_pose[b] = base_pose[b];

        for (size_t li = 0; li < layers.size(); ++li)
        {
            Layer& layer = layers[li];
            // ensure current player is valid
            if (layer.current.state_index < 0 && !layer.states.empty())
            {
                layer.current.state_index = 0;
                layer.current.time_seconds = 0.0f;
            }

            // evaluate transitions (simple: take first valid)
            BlendJob& bj = blend_jobs[li];
            if (!bj.active)
            {
                for (const Transition& tr : layer.transitions)
                {
                    if (tr.from_state != -1 && tr.from_state != layer.current.state_index) continue;
                    bool ok = true;
                    for (const Condition& c : tr.conditions)
                    {
                        if (!evalCondition(c))
                        {
                            ok = false;
                            break;
                        }
                    }
                    if (!ok) continue;
                    if (tr.has_exit_time)
                    {
                        const State& curS = layer.states[layer.current.state_index];
                        if (!curS.clip) continue;
                        float nt = (curS.clip->duration > 1e-6f) ? (layer.current.time_seconds / curS.clip->duration) : 0.0f;
                        if (nt < tr.exit_normalized_time) continue;
                    }
                    // start blend job
                    bj.active = true;
                    bj.from_state = layer.current.state_index;
                    bj.to_state = tr.to_state;
                    bj.elapsed = 0.0f;
                    bj.duration = std::max(1e-5f, tr.duration);
                    break; // one transition at a time
                }
            }

            // advance times
            if (bj.active)
            {
                // advance both states time
                advanceStateTime(layer, bj.from_state, dt);
                advanceStateTime(layer, bj.to_state, dt);
                bj.elapsed += dt;
                if (bj.elapsed >= bj.duration)
                {
                    // finish
                    layer.current.state_index = bj.to_state;
                    // leave time as whatever was advanced into the to-state
                    bj.active = false;
                }
            }
            else
                advanceStateTime(layer, layer.current.state_index, dt);

            // sample layer pose
            std::vector<Trans> layer_pose(skeleton_bone_count, Trans::Identity());
            if (bj.active)
            {
                float w = std::clamp(bj.elapsed / bj.duration, 0.0f, 1.0f);
                samplePoseInto(layer, bj.from_state, layer_pose, 1.0f - w);
                std::vector<Trans> to_pose(skeleton_bone_count, Trans::Identity());
                samplePoseInto(layer, bj.to_state, to_pose, w);
                for (int b = 0; b < skeleton_bone_count; ++b)
                    layer_pose[b] = Lerp(layer_pose[b], to_pose[b], w);
            }
            else
                samplePoseInto(layer, layer.current.state_index, layer_pose, 1.0f);

            // apply this layer into outPose
            applyLayerToOutPose(layer, layer_pose);
        }
    }
}
