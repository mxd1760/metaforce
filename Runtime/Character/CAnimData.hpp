#ifndef __URDE_CANIMDATA_HPP__
#define __URDE_CANIMDATA_HPP__

#include "RetroTypes.hpp"
#include "CToken.hpp"
#include "CCharacterInfo.hpp"
#include "CParticleDatabase.hpp"
#include "CPoseAsTransforms.hpp"
#include "CHierarchyPoseBuilder.hpp"
#include "CAdditiveAnimPlayback.hpp"
#include <set>

enum class EUserEventType
{
};

namespace urde
{
class CCharLayoutInfo;
class CSkinnedModel;
class CMorphableSkinnedModel;
struct CAnimSysContext;
class CAnimationManager;
class CTransitionManager;
class CCharacterFactory;
class IMetaAnim;
struct CModelFlags;
class CVertexMorphEffect;
class CFrustumPlanes;
class CPrimitive;
class CAnimPlaybackParms;
class CRandom16;
class CStateManager;
class CCharAnimTime;
class CModel;
class CSkinRules;
class CAnimTreeNode;
class CSegIdList;
class CSegStatementSet;
class CBoolPOINode;
class CInt32POINode;
class CParticlePOINode;
class CSoundPOINode;
struct SAdvancementDeltas;

class CAnimData
{
    friend class CModelData;
    TLockedToken<CCharacterFactory> x0_charFactory;
    CCharacterInfo xc_charInfo;
    TLockedToken<CCharLayoutInfo> xcc_layoutData;
    TCachedToken<CSkinnedModel> xd8_modelData;
    TLockedToken<CMorphableSkinnedModel> xe4_iceModelData;
    std::shared_ptr<CSkinnedModel> xf4_xrayModel;
    std::shared_ptr<CSkinnedModel> xf8_infraModel;
    std::shared_ptr<CAnimSysContext> xfc_animCtx;
    std::shared_ptr<CAnimationManager> x100_animMgr;
    u32 x104_ = 0;
    zeus::CAABox x108_aabb;
    CParticleDatabase x120_particleDB;
    ResId x1d8_selfId;
    zeus::CVector3f x1dc_;
    zeus::CQuaternion x1e8_;
    std::shared_ptr<CAnimTreeNode> x1f8_animRoot;
    std::shared_ptr<CTransitionManager> x1fc_transMgr;

    float x200_ = 1.f;
    u32 x204_charIdx;
    u16 x208_defaultAnim;
    u32 x20c_passedBoolCount = 0;
    u32 x210_passedIntCount = 0;
    u32 x214_passedParticleCount = 0;
    u32 x218_passedSoundCount = 0;

    union
    {
        u32 x220_flags = 0;
        struct
        {
            bool x220_24_animating : 1;
            bool x220_25_loop : 1;
            bool x220_26_ : 1;
            bool x220_27_ : 1;
            bool x220_28_ : 1;
            bool x220_29_ : 1;
            bool x220_30_ : 1;
            bool x220_31_poseCached : 1;
        };
    };

    CPoseAsTransforms x224_pose;
    CHierarchyPoseBuilder x2fc_poseBuilder;

    u32 x1020_ = -1;
    u32 x10204_ = -1;
    float x1028_ = 1.f;
    bool x102c_ = true;
    u32 x1030_ = 0;
    u32 x1034_ = 0;
    bool x1038_ = false;
    u32 x103c_ = 0;
    u32 x1040_ = 0;
    u32 x1044_ = 0;
    rstl::reserved_vector<std::pair<u32, CAdditiveAnimPlayback>, 8> x1048_additiveAnims;

    static rstl::reserved_vector<CBoolPOINode, 8> g_BoolPOINodes;
    static rstl::reserved_vector<CInt32POINode, 16> g_Int32POINodes;
    static rstl::reserved_vector<CParticlePOINode, 20> g_ParticlePOINodes;
    static rstl::reserved_vector<CSoundPOINode, 20> g_SoundPOINodes;

public:
    CAnimData(ResId,
              const CCharacterInfo& character,
              int defaultAnim, int charIdx, bool loop,
              const TLockedToken<CCharLayoutInfo>& layout,
              const TToken<CSkinnedModel>& model,
              const std::experimental::optional<TToken<CMorphableSkinnedModel>>& iceModel,
              const std::weak_ptr<CAnimSysContext>& ctx,
              const std::shared_ptr<CAnimationManager>& animMgr,
              const std::shared_ptr<CTransitionManager>& transMgr,
              const TLockedToken<CCharacterFactory>& charFactory);

    ResId GetEventResourceIdForAnimResourceId(ResId) const;
    void AddAdditiveSegData(const CSegIdList& list, CSegStatementSet& stSet);
    void AdvanceAdditiveAnims(float);
    void UpdateAdditiveAnims(float);
    bool IsAdditiveAnimation(u32) const;
    std::shared_ptr<CAnimTreeNode> GetAdditiveAnimationTree(u32) const;
    bool IsAdditiveAnimationActive(u32) const;
    void DelAdditiveAnimation(u32);
    void AddAdditiveAnimation(u32, float, bool, bool);
    std::shared_ptr<CAnimationManager> GetAnimationManager();
    const CCharLayoutInfo& GetCharLayoutInfo() const { return *xcc_layoutData.GetObj(); }
    void SetPhase(float);
    void Touch(const CSkinnedModel& model, int shaderIdx) const;
    SAdvancementDeltas GetAdvancementDeltas(const CCharAnimTime& a, const CCharAnimTime& b) const;
    CCharAnimTime GetTimeOfUserEvent(EUserEventType, const CCharAnimTime& time) const;
    void MultiplyPlaybackRate(float);
    void SetPlaybackRate(float);
    void SetRandomPlaybackRate(CRandom16&);
    void CalcPlaybackAlignmentParms(const CAnimPlaybackParms& parms,
                                    const std::weak_ptr<CAnimTreeNode>& node);
    zeus::CTransform GetLocatorTransform(CSegId id, const CCharAnimTime* time) const;
    zeus::CTransform GetLocatorTransform(const std::string& name, const CCharAnimTime* time) const;
    bool IsAnimTimeRemaining(float, const std::string& name) const;
    float GetAnimTimeRemaining(const std::string& name) const;
    float GetAnimationDuration(int) const;
    bool GetIsLoop() const {return x220_25_loop;}
    void EnableLooping(bool val) {x220_25_loop = val; x220_24_animating = true;}
    bool IsAnimating() const {return x220_24_animating;}
    std::shared_ptr<CAnimSysContext> GetAnimSysContext() const;
    std::shared_ptr<CAnimationManager> GetAnimationManager() const;
    void RecalcPoseBuilder(const CCharAnimTime*);
    void RenderAuxiliary(const CFrustumPlanes& frustum) const;
    void Render(const CSkinnedModel& model, const CModelFlags& drawFlags,
                const std::experimental::optional<CVertexMorphEffect>& morphEffect,
                const float* morphMagnitudes) const;
    void SetupRender(const CSkinnedModel& model,
                     const std::experimental::optional<CVertexMorphEffect>& morphEffect,
                     const float* morphMagnitudes) const;
    void PreRender();
    void BuildPose();
    void PrimitiveSetToTokenVector(const std::set<CPrimitive>& primSet, std::vector<CToken>& tokensOut);
    void GetAnimationPrimitives(const CAnimPlaybackParms& parms, std::set<CPrimitive>& primsOut) const;
    void SetAnimation(const CAnimPlaybackParms& parms, bool);
    void DoAdvance(float, bool&, CRandom16&, bool);
    SAdvancementDeltas Advance(float, const zeus::CVector3f&, CStateManager& stateMgr, bool);
    SAdvancementDeltas AdvanceIgnoreParticles(float, CRandom16&, bool);
    void AdvanceAnim(CCharAnimTime& time, zeus::CVector3f&, zeus::CQuaternion&);
    void SetXRayModel(const TLockedToken<CModel>& model, const TLockedToken<CSkinRules>& skinRules);
    void SetInfraModel(const TLockedToken<CModel>& model, const TLockedToken<CSkinRules>& skinRules);

    void PoseSkinnedModel(const CSkinnedModel& model, const CPoseAsTransforms& pose,
                          const std::experimental::optional<CVertexMorphEffect>& morphEffect,
                          const float* morphMagnitudes) const;
    void AdvanceParticles(const zeus::CTransform& xf, float dt,
                          const zeus::CVector3f&, CStateManager& stateMgr);
    void GetAverageVelocity(int) const;
    void ResetPOILists();
    CSegId GetLocatorSegId(const std::string& name) const;
    zeus::CAABox GetBoundingBox(const zeus::CTransform& xf) const;
    zeus::CAABox GetBoundingBox() const;

    static void FreeCache();
    static void InitializeCache();
    const CParticleDatabase& GetParticleDB() const { return x120_particleDB; }
};

}

#endif // __URDE_CANIMDATA_HPP__
