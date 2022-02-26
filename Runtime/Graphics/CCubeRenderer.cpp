#include "Runtime/Graphics/CCubeRenderer.hpp"

#include "Runtime/GameGlobalObjects.hpp"
#include "Runtime/Graphics/CDrawable.hpp"
#include "Runtime/Graphics/CDrawablePlaneObject.hpp"
#include "Runtime/Graphics/CLight.hpp"
#include "Runtime/Graphics/CModel.hpp"

namespace metaforce {
static logvisor::Module Log("CCubeRenderer");

static rstl::reserved_vector<CDrawable, 512> sDataHolder;
static rstl::reserved_vector<rstl::reserved_vector<CDrawable*, 128>, 50> sBucketsHolder;
static rstl::reserved_vector<CDrawablePlaneObject, 8> sPlaneObjectDataHolder;
static rstl::reserved_vector<u16, 8> sPlaneObjectBucketHolder;

class Buckets {
  friend class CCubeRenderer;

  static inline rstl::reserved_vector<u16, 50> sBucketIndex;
  static inline rstl::reserved_vector<CDrawable, 512>* sData = nullptr;
  static inline rstl::reserved_vector<rstl::reserved_vector<CDrawable*, 128>, 50>* sBuckets = nullptr;
  static inline rstl::reserved_vector<CDrawablePlaneObject, 8>* sPlaneObjectData = nullptr;
  static inline rstl::reserved_vector<u16, 8>* sPlaneObjectBucket = nullptr;
  static constexpr std::array skWorstMinMaxDistance{99999.0f, -99999.0f};
  static inline std::array sMinMaxDistance{0.0f, 0.0f};

public:
  static void Clear();
  static void Sort();
  static void InsertPlaneObject(float closeDist, float farDist, const zeus::CAABox& aabb, bool invertTest,
                                const zeus::CPlane& plane, bool zOnly, EDrawableType dtype, void* data);
  static void Insert(const zeus::CVector3f& pos, const zeus::CAABox& aabb, EDrawableType dtype, void* data,
                     const zeus::CPlane& plane, u16 extraSort);
  static void Shutdown();
  static void Init();
};

void Buckets::Clear() {
  sData->clear();
  sBucketIndex.clear();
  sPlaneObjectData->clear();
  sPlaneObjectBucket->clear();
  for (rstl::reserved_vector<CDrawable*, 128>& bucket : *sBuckets) {
    bucket.clear();
  }
  sMinMaxDistance = skWorstMinMaxDistance;
}

void Buckets::Sort() {
  float delta = std::max(1.f, sMinMaxDistance[1] - sMinMaxDistance[0]);
  float pitch = 49.f / delta;
  for (auto it = sPlaneObjectData->begin(); it != sPlaneObjectData->end(); ++it)
    if (sPlaneObjectBucket->size() != sPlaneObjectBucket->capacity())
      sPlaneObjectBucket->push_back(s16(it - sPlaneObjectData->begin()));

  u32 precision = 50;
  if (sPlaneObjectBucket->size()) {
    std::sort(sPlaneObjectBucket->begin(), sPlaneObjectBucket->end(),
              [](u16 a, u16 b) { return (*sPlaneObjectData)[a].GetDistance() < (*sPlaneObjectData)[b].GetDistance(); });
    precision = 50 / u32(sPlaneObjectBucket->size() + 1);
    pitch = 1.f / (delta / float(precision - 2));

    int accum = 0;
    for (u16 idx : *sPlaneObjectBucket) {
      ++accum;
      CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[idx];
      planeObj.x24_targetBucket = u16(precision * accum);
    }
  }

  for (CDrawable& drawable : *sData) {
    int slot;
    float relDist = drawable.GetDistance() - sMinMaxDistance[0];
    if (sPlaneObjectBucket->empty()) {
      slot = zeus::clamp(1, int(relDist * pitch), 49);
    } else {
      slot = zeus::clamp(0, int(relDist * pitch), int(precision) - 2);
      for (u16 idx : *sPlaneObjectBucket) {
        CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[idx];
        bool partial, full;
        if (planeObj.x3c_25_zOnly) {
          partial = drawable.GetBounds().max.z() > planeObj.GetPlane().d();
          full = drawable.GetBounds().min.z() > planeObj.GetPlane().d();
        } else {
          partial = planeObj.GetPlane().pointToPlaneDist(
                        drawable.GetBounds().closestPointAlongVector(planeObj.GetPlane().normal())) > 0.f;
          full = planeObj.GetPlane().pointToPlaneDist(
                     drawable.GetBounds().furthestPointAlongVector(planeObj.GetPlane().normal())) > 0.f;
        }
        bool cont;
        if (drawable.GetType() == EDrawableType::Particle)
          cont = planeObj.x3c_24_invertTest ? !partial : full;
        else
          cont = planeObj.x3c_24_invertTest ? (!partial || !full) : (partial || full);
        if (!cont)
          break;
        slot += precision;
      }
    }

    if (slot == -1)
      slot = 49;
    rstl::reserved_vector<CDrawable*, 128>& bucket = (*sBuckets)[slot];
    if (bucket.size() < bucket.capacity())
      bucket.push_back(&drawable);
    // else
    //    Log.report(logvisor::Fatal, FMT_STRING("Full bucket!!!"));
  }

  u16 bucketIdx = u16(sBuckets->size());
  for (auto it = sBuckets->rbegin(); it != sBuckets->rend(); ++it) {
    --bucketIdx;
    sBucketIndex.push_back(bucketIdx);
    rstl::reserved_vector<CDrawable*, 128>& bucket = *it;
    if (bucket.size()) {
      std::sort(bucket.begin(), bucket.end(), [](CDrawable* a, CDrawable* b) {
        if (a->GetDistance() == b->GetDistance())
          return a->GetExtraSort() > b->GetExtraSort();
        return a->GetDistance() > b->GetDistance();
      });
    }
  }

  for (auto it = sPlaneObjectBucket->rbegin(); it != sPlaneObjectBucket->rend(); ++it) {
    CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[*it];
    rstl::reserved_vector<CDrawable*, 128>& bucket = (*sBuckets)[planeObj.x24_targetBucket];
    bucket.push_back(&planeObj);
  }
}

void Buckets::InsertPlaneObject(float closeDist, float farDist, const zeus::CAABox& aabb, bool invertTest,
                                const zeus::CPlane& plane, bool zOnly, EDrawableType dtype, void* data) {
  if (sPlaneObjectData->size() == sPlaneObjectData->capacity()) {
    return;
  }
  sPlaneObjectData->emplace_back(dtype, closeDist, farDist, aabb, invertTest, plane, zOnly, data);
}

void Buckets::Insert(const zeus::CVector3f& pos, const zeus::CAABox& aabb, EDrawableType dtype, void* data,
                     const zeus::CPlane& plane, u16 extraSort) {
  if (sData->size() == sData->capacity()) {
    Log.report(logvisor::Fatal, FMT_STRING("Rendering buckets filled to capacity"));
    return;
  }

  const float dist = plane.pointToPlaneDist(pos);
  sData->emplace_back(dtype, extraSort, dist, aabb, data);
  sMinMaxDistance[0] = std::min(sMinMaxDistance[0], dist);
  sMinMaxDistance[1] = std::max(sMinMaxDistance[1], dist);
}

void Buckets::Shutdown() {
  sData = nullptr;
  sBuckets = nullptr;
  sPlaneObjectData = nullptr;
  sPlaneObjectBucket = nullptr;
}

void Buckets::Init() {
  sData = &sDataHolder;
  sBuckets = &sBucketsHolder;
  sBuckets->resize(50);
  sPlaneObjectData = &sPlaneObjectDataHolder;
  sPlaneObjectBucket = &sPlaneObjectBucketHolder;
  sMinMaxDistance = skWorstMinMaxDistance;
}

CCubeRenderer::CCubeRenderer(IObjectStore& store, IFactory& resFac) : x8_factory(resFac), xc_store(store) {
  // void* data = xe4_blackTex.GetBitMapData();
  // memset(data, 0, 32);
  // xe4_blackTex.UnLock();
  GenerateReflectionTex();
  GenerateFogVolumeRampTex();
  GenerateSphereRampTex();
  LoadThermoPalette();
  g_Renderer = this;
  Buckets::Init();
  // GX draw sync
}

CCubeRenderer::~CCubeRenderer() {
  g_Renderer = nullptr;
}

void CCubeRenderer::GenerateReflectionTex() {}
void CCubeRenderer::GenerateFogVolumeRampTex() {}
void CCubeRenderer::GenerateSphereRampTex() {}
void CCubeRenderer::LoadThermoPalette() {}
void CCubeRenderer::ReallyDrawPhazonSuitIndirectEffect(const zeus::CColor& vertColor, const CTexture& maskTex,
                                                       const CTexture& indTex, const zeus::CColor& modColor,
                                                       float scale, float offX, float offY) {}
void CCubeRenderer::ReallyDrawPhazonSuitEffect(const zeus::CColor& modColor, const CTexture& maskTex) {}
void CCubeRenderer::DoPhazonSuitIndirectAlphaBlur(float blurRadius, float f2, const TLockedToken<CTexture>& indTex) {}
void CCubeRenderer::AddStaticGeometry(const std::vector<CMetroidModelInstance>* geometry,
                                      const CAreaRenderOctTree* octTree, int areaIdx) {}
void CCubeRenderer::EnablePVS(const CPVSVisSet& set, u32 areaIdx) {}
void CCubeRenderer::DisablePVS() {}
void CCubeRenderer::RemoveStaticGeometry(const std::vector<CMetroidModelInstance>* geometry) {}
void CCubeRenderer::DrawUnsortedGeometry(int areaIdx, int mask, int targetMask, bool shadowRender) {}
void CCubeRenderer::DrawSortedGeometry(int areaIdx, int mask, int targetMask) {}
void CCubeRenderer::DrawStaticGeometry(int areaIdx, int mask, int targetMask) {}
void CCubeRenderer::DrawAreaGeometry(int areaIdx, int mask, int targetMask) {}
void CCubeRenderer::PostRenderFogs() {}
void CCubeRenderer::SetModelMatrix(const zeus::CTransform& xf) {}
void CCubeRenderer::AddParticleGen(CParticleGen& gen) {}
void CCubeRenderer::AddParticleGen(CParticleGen& gen, const zeus::CVector3f& pos, const zeus::CAABox& bounds) {}
void CCubeRenderer::AddPlaneObject(void* obj, const zeus::CAABox& aabb, const zeus::CPlane& plane, int type) {}
void CCubeRenderer::AddDrawable(void* obj, const zeus::CVector3f& pos, const zeus::CAABox& aabb, int mode,
                                IRenderer::EDrawableSorting sorting) {}
void CCubeRenderer::SetDrawableCallback(IRenderer::TDrawableCallback cb, void* ctx) {}
void CCubeRenderer::SetWorldViewpoint(const zeus::CTransform& xf) {}
void CCubeRenderer::SetPerspective(float fovy, float aspect, float znear, float zfar) {}
void CCubeRenderer::SetPerspective(float fovy, float width, float height, float znear, float zfar) {}
std::pair<zeus::CVector2f, zeus::CVector2f> CCubeRenderer::SetViewportOrtho(bool centered, float znear, float zfar) {
  return std::pair<zeus::CVector2f, zeus::CVector2f>();
}
void CCubeRenderer::SetClippingPlanes(const zeus::CFrustum& frustum) {}
void CCubeRenderer::SetViewport(int left, int bottom, int width, int height) {}
void CCubeRenderer::BeginScene() {}
void CCubeRenderer::EndScene() {}
void CCubeRenderer::SetDebugOption(IRenderer::EDebugOption, int) {}
void CCubeRenderer::BeginPrimitive(IRenderer::EPrimitiveType, int) {}
void CCubeRenderer::BeginLines(int) {}
void CCubeRenderer::BeginLineStrip(int) {}
void CCubeRenderer::BeginTriangles(int) {}
void CCubeRenderer::BeginTriangleStrip(int) {}
void CCubeRenderer::BeginTriangleFan(int) {}
void CCubeRenderer::PrimVertex(const zeus::CVector3f&) {}
void CCubeRenderer::PrimNormal(const zeus::CVector3f&) {}
void CCubeRenderer::PrimColor(float, float, float, float) {}
void CCubeRenderer::PrimColor(const zeus::CColor&) {}
void CCubeRenderer::EndPrimitive() {}
void CCubeRenderer::SetAmbientColor(const zeus::CColor& color) {}
void CCubeRenderer::DrawString(const char* string, int, int) {}
u32 CCubeRenderer::GetFPS() { return 0; }
void CCubeRenderer::CacheReflection(IRenderer::TReflectionCallback cb, void* ctx, bool clearAfter) {}
void CCubeRenderer::DrawSpaceWarp(const zeus::CVector3f& pt, float strength) {}
void CCubeRenderer::DrawThermalModel(const CModel& model, const zeus::CColor& multCol, const zeus::CColor& addCol,
                                     TVectorRef positions, TVectorRef normals, CModelFlags flags) {}
void CCubeRenderer::DrawModelDisintegrate(const CModel& model, const CTexture& tex, const zeus::CColor& color,
                                          TVectorRef positions, TVectorRef normals) {}
void CCubeRenderer::DrawModelFlat(const CModel& model, const CModelFlags& flags, bool unsortedOnly) {}
void CCubeRenderer::SetWireframeFlags(int flags) {}
void CCubeRenderer::SetWorldFog(ERglFogMode mode, float startz, float endz, const zeus::CColor& color) {}
void CCubeRenderer::RenderFogVolume(const zeus::CColor& color, const zeus::CAABox& aabb,
                                    const TLockedToken<CModel>* model, const CSkinnedModel* sModel) {}
void CCubeRenderer::SetThermal(bool thermal, float level, const zeus::CColor& color) {}
void CCubeRenderer::SetThermalColdScale(float scale) {}
void CCubeRenderer::DoThermalBlendCold() {}
void CCubeRenderer::DoThermalBlendHot() {}
u32 CCubeRenderer::GetStaticWorldDataSize() { return 0; }
void CCubeRenderer::SetGXRegister1Color(const zeus::CColor& color) {}
void CCubeRenderer::SetWorldLightFadeLevel(float level) {}
void CCubeRenderer::SetWorldLightMultiplyColor(const zeus::CColor& color) {}
void CCubeRenderer::PrepareDynamicLights(const std::vector<CLight>& lights) {}
void CCubeRenderer::AllocatePhazonSuitMaskTexture() {}
void CCubeRenderer::DrawPhazonSuitIndirectEffect(const zeus::CColor& nonIndirectMod,
                                                 const TLockedToken<CTexture>& indTex, const zeus::CColor& indirectMod,
                                                 float blurRadius, float scale, float offX, float offY) {}
void CCubeRenderer::DrawXRayOutline(const zeus::CAABox& aabb) {}
void CCubeRenderer::FindOverlappingWorldModels(std::vector<u32>& modelBits, const zeus::CAABox& aabb) const {}
int CCubeRenderer::DrawOverlappingWorldModelIDs(int alphaVal, const std::vector<u32>& modelBits,
                                                const zeus::CAABox& aabb) {
  return 0;
}
void CCubeRenderer::DrawOverlappingWorldModelShadows(int alphaVal, const std::vector<u32>& modelBits,
                                                     const zeus::CAABox& aabb, float alpha) {}
} // namespace metaforce