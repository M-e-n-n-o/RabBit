#pragma once
#include "RabBitCommon.h"
#include "Transform.h"
#include "entity/GameObject.h"
#include "entity/ObjectComponent.h"

namespace RB::Entity
{
    enum AnimProperty : uint32_t
    {
        kAnimProperty_Position,
        kAnimProperty_Rotation,
        kAnimProperty_Scale,

        kAnimProperty_Custom // Use from this point to define custom animation props
    };

    template<typename T>
    struct AnimationKey
    {
        float time;
        T     value;

        static T InterpolateKeys(const T& a, const T& b, float mix) 
        { 
            return Math::Lerp(a, b, mix); 
        }
    };

    template<>
    inline Math::Float3 AnimationKey<Math::Float3>::InterpolateKeys(const Math::Float3& a, const Math::Float3& b, float mix)
    {
        return Math::Float3::Lerp(a, b, mix);
    }

    template<>
    inline Math::Quaternion AnimationKey<Math::Quaternion>::InterpolateKeys(const Math::Quaternion& a, const Math::Quaternion& b, float mix)
    {
        return Math::Quaternion::Slerp(a, b, mix);
    }

    // All keyframes of a single node
    template<typename T>
    struct AnimationTrack
    {
        uint32_t                nodeIndex;  // The bone index this is linked to
        uint32_t                propertyID; // AnimProperty
        List<AnimationKey<T>>   keys;

        T Sample(float time) const
        {
            if (time <= keys.front().time)
                return keys.front().value;
            if (time >= keys.back().time)
                return keys.back().value;

            size_t high = keys.size();

            for (size_t i = 0; i < keys.size(); ++i)
            {
                if (time < keys[i].time)
                {
                    high = i;
                    break;
                }
            }

            size_t low = high - 1;

            const float span = keys[high].time - keys[low].time;
            const float mix = span > 0.0f ? (time - keys[low].time) / span : 0.0f;

            return AnimationKey<T>::InterpolateKeys(keys[low].value, keys[high].value, mix);
        }
    };



    class BindContext
    {
    public:
        BindContext(const List<GameObject*>& nodes, const UnorderedMap<uint64_t, void*>& custom_targets)
            : m_Nodes(nodes)
            , m_CustomTargets(custom_targets)
        {
        }

        template<typename T>
        T* Resolve(uint32_t node_index, uint32_t property_id)
        {
            // Built-in animation properties
            if constexpr (std::is_same_v<T, Math::Float3>)
            {
                Transform* trans = m_Nodes[node_index]->GetComponent<Transform>();

                if (property_id == kAnimProperty_Position)
                    return trans ? &trans->position : nullptr;
                else if (property_id == kAnimProperty_Scale)
                    return trans ? &trans->scale : nullptr;
            }
            if constexpr (std::is_same_v<T, Math::Quaternion>)
            {
                Transform* trans = m_Nodes[node_index]->GetComponent<Transform>();

                if (property_id == kAnimProperty_Rotation)
                    return trans ? &trans->rotation : nullptr;
            }

            // Custom properties (bound using Animator::Bind)
            auto it = m_CustomTargets.find(MakeKey(node_index, property_id));
            if (it == m_CustomTargets.end())
            {
                RB_LOG_WARN(LOGTAG_ENTITY, "Property ID %d is not bound to any output, the animation will not be applied properly", property_id);
                return nullptr;
            }

            return static_cast<T*>(it->second);
        }

        static uint64_t MakeKey(uint32_t node_index, uint32_t property_id)
        {
            return (uint64_t(node_index) << 32) | property_id;
        }

    private:
        const List<GameObject*>&             m_Nodes;
        const UnorderedMap<uint64_t, void*>& m_CustomTargets;
    };



    struct ITrackList
    {
        virtual ~ITrackList() = default;
        virtual void Bind(BindContext& ctx) = 0;     // Resolves target pointers
        virtual void Evaluate(float time) const = 0; // Applies the animation to the targets
    };

    template<typename T>
    class TrackList : public ITrackList
    {
    public:
        AnimationTrack<T>& GetOrAddTrack(uint32_t node_index, uint32_t property_id)
        {
            for (AnimationTrack<T>& track : m_Tracks)
            {
                if (track.nodeIndex == node_index && track.propertyID == property_id)
                    return track;
            }

            m_Tracks.push_back({ node_index, property_id, {} });
            return m_Tracks.back();
        }

        void Bind(BindContext& ctx) override
        {
            m_BoundTargets.resize(m_Tracks.size());
            for (size_t i = 0; i < m_Tracks.size(); i++)
                m_BoundTargets[i] = ctx.Resolve<T>(m_Tracks[i].nodeIndex, m_Tracks[i].propertyID);
        }

        void Evaluate(float time) const override
        {
            for (size_t i = 0; i < m_Tracks.size(); i++)
            {
                if (T* target = m_BoundTargets[i])
                    *target = m_Tracks[i].Sample(time);
            }
        }

    private:
        List<AnimationTrack<T>> m_Tracks;
        List<T*>                m_BoundTargets;
    };



    class Animation
    {
    public:
        Animation(const char* name, float duration_sec) 
            : m_Name(name)
            , m_DurationSeconds(duration_sec) 
        {
        }

        template<typename T>
        void AddKey(uint32_t node_index, uint32_t property_id, float time, const T& value)
        {
            GetOrAddTrackList<T>().GetOrAddTrack(node_index, property_id).keys.push_back({ time, value });
        }

        const char* GetName() const { return m_Name.c_str(); }
        float       GetDuration() const { return m_DurationSeconds; }
        const List<Unique<ITrackList>>& GetTrackLists() const { return m_TrackLists; }

    private:
        template<typename T>
        TrackList<T>& GetOrAddTrackList()
        {
            const std::type_index type = typeid(T);

            auto it = m_TypeToIndex.find(type);
            if (it != m_TypeToIndex.end())
                return *static_cast<TrackList<T>*>(m_TrackLists[it->second].get());

            m_TrackLists.push_back(CreateUnique<TrackList<T>>());
            m_TypeToIndex[type] = m_TrackLists.size() - 1;
            return *static_cast<TrackList<T>*>(m_TrackLists.back().get());
        }

        std::string                             m_Name;
        float                                   m_DurationSeconds;
        List<Unique<ITrackList>>                m_TrackLists;
        UnorderedMap<std::type_index, size_t>   m_TypeToIndex;
    };



    class Animator : public ObjectComponent
    {
    public:
        Animator(const List<GameObject*>& nodes) 
            : m_Nodes(nodes)
            , m_CurrentAnimation(nullptr)
            , m_Time(0.0f)
            , m_IsLooping(true)
            , m_RootTransform(nullptr)
            , m_ApplyRootMotion(true)
            , m_InterpolationFrameRate(0)
        {
        }

        void AddAnimation(Animation* animation) 
        { 
            m_Animations.push_back(animation); 
        }

        // Register any custom property to be animated by the Animator
        template<typename T>
        void Bind(uint32_t node_index, uint32_t property_id, T* target)
        {
            m_CustomTargets[BindContext::MakeKey(node_index, property_id)] = target;
        }

        void SetLooping(bool looping)
        {
            m_IsLooping = looping;
        }

        void SetApplyRootMotion(bool apply)
        {
            m_ApplyRootMotion = apply;
        }

        void SetInterpolationFrameRate(uint32_t fps)
        {
            m_InterpolationFrameRate = fps;
        }

        void PlayAnimation(const char* name)
        {
            auto itr = std::find_if(m_Animations.begin(), m_Animations.end(), [name](const Animation* anim) -> bool
                {
                    return (std::strcmp(anim->GetName(), name) == 0);
                });

            if (itr == m_Animations.end())
            {
                RB_LOG_WARN(LOGTAG_ENTITY, "Animation \"%s\" does not exist", name);
                return;
            }

            if (m_Nodes.empty())
            {
                RB_LOG_WARN(LOGTAG_ENTITY, "Animator does not have any output nodes to animate");
                return;
            }

            m_CurrentAnimation = *itr;
            m_RootTransform = m_Nodes[0]->GetComponent<Transform>();
            m_Time = 0.0f;

            BindContext ctx(m_Nodes, m_CustomTargets);
            for (auto& track_list : m_CurrentAnimation->GetTrackLists())
                track_list->Bind(ctx);
        }

        void OnUpdate(float delta) override
        {
            if (!m_CurrentAnimation)
                return;

            m_Time += delta;

            if (m_Time > m_CurrentAnimation->GetDuration())
            {
                if (m_IsLooping)
                    m_Time = fmodf(m_Time, m_CurrentAnimation->GetDuration());
                else
                    m_Time = m_CurrentAnimation->GetDuration();
            }

            float anim_time = m_Time;
            if (m_InterpolationFrameRate > 0)
            {
                anim_time = Math::AlignDown(anim_time, 1.0f / (float)m_InterpolationFrameRate);
            }

            Math::Float3 original_pos = m_RootTransform ? m_RootTransform->position : Math::Float3();

            for (auto& track_list : m_CurrentAnimation->GetTrackLists())
                track_list->Evaluate(anim_time);

            if (!m_ApplyRootMotion && m_RootTransform)
                m_RootTransform->position = original_pos;
        }

    private:
        Animation*                      m_CurrentAnimation;
        float                           m_Time;
        bool                            m_IsLooping;
        Transform*                      m_RootTransform;

        List<GameObject*>               m_Nodes;
        List<Animation*>                m_Animations;
        UnorderedMap<uint64_t, void*>   m_CustomTargets;

        bool                            m_ApplyRootMotion;
        uint32_t                        m_InterpolationFrameRate;   // 0 is smooth (follow actual fps)
    };
}