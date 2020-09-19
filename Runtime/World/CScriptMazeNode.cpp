#include "Runtime/World/CScriptMazeNode.hpp"

#include "Runtime/Character/CModelData.hpp"
#include "Runtime/CStateManager.hpp"
#include "Runtime/GameGlobalObjects.hpp"
#include "Runtime/World/CActorParameters.hpp"

#include "TCastTo.hpp" // Generated file, do not modify include path

namespace urde {

std::array<s32, 300> CScriptMazeNode::sMazeSeeds{};

CScriptMazeNode::CScriptMazeNode(TUniqueId uid, std::string_view name, const CEntityInfo& info,
                                 const zeus::CTransform& xf, bool active, s32 w1, s32 w2, s32 w3,
                                 const zeus::CVector3f& actorPos, const zeus::CVector3f& triggerPos,
                                 const zeus::CVector3f& effectPos)
: CActor(uid, active, name, info, xf, CModelData::CModelDataNull(), CMaterialList(), CActorParameters::None(),
         kInvalidUniqueId)
, xe8_(w1)
, xec_(w1)
, xf0_(w2)
, x100_actorPos(actorPos)
, x110_triggerPos(triggerPos)
, x120_effectPos(effectPos) {}

void CScriptMazeNode::Accept(IVisitor& visitor) { visitor.Visit(this); }

void CScriptMazeNode::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId uid, CStateManager& mgr) {
  if (GetActive()) {
    if (msg == EScriptObjectMessage::Action) {
      // TODO
    } else if (msg == EScriptObjectMessage::SetToZero) {
      // TODO
    } else if (msg == EScriptObjectMessage::Deactivate) {
      // TODO
    } else if (msg == EScriptObjectMessage::InitializedInArea) {
      if (mgr.GetCurrentMaze() == nullptr) {
        auto maze = std::make_unique<CScriptMazeState>(4, 4, 5, 3);
        maze->Reset(sMazeSeeds[mgr.GetActiveRandom()->Next() % sMazeSeeds.size()]);
        maze->Initialize();
        // sub_802899c8
        mgr.SetCurrentMaze(std::move(maze));
      }
    } else if (msg == EScriptObjectMessage::Deleted) {
      mgr.ClearCurrentMaze();
      Reset(mgr);
    }
  }
  CActor::AcceptScriptMsg(msg, uid, mgr);
}

void CScriptMazeNode::LoadMazeSeeds() {
  const SObjectTag* tag = g_ResFactory->GetResourceIdByName("DUMB_MazeSeeds");
  const u32 resSize = g_ResFactory->ResourceSize(*tag);
  const std::unique_ptr<u8[]> buf = g_ResFactory->LoadResourceSync(*tag);
  CMemoryInStream in(buf.get(), resSize);
  for (auto& seed : sMazeSeeds) {
    seed = in.readInt32Big();
  }
}

void CScriptMazeNode::GenerateObjects(CStateManager& mgr) {
  for (const auto& conn : GetConnectionList()) {
    if (conn.x0_state != EScriptObjectState::MaxReached || conn.x4_msg != EScriptObjectMessage::Activate) {
      continue;
    }
    const auto* ent = mgr.GetObjectById(mgr.GetIdForScript(conn.x8_objId));
    TCastToConstPtr<CScriptEffect> scriptEffect{ent};
    TCastToConstPtr<CScriptActor> scriptActor{ent};
    TCastToConstPtr<CScriptTrigger> scriptTrigger{ent};
    if ((scriptEffect || scriptActor || scriptTrigger) && (!scriptEffect || !x13c_25_)) {
      bool wasGeneratingObject = mgr.GetIsGeneratingObject();
      mgr.SetIsGeneratingObject(true);
      const auto genObj = mgr.GenerateObject(conn.x8_objId);
      mgr.SetIsGeneratingObject(wasGeneratingObject);
      if (auto* actor = static_cast<CActor*>(mgr.ObjectById(genObj.second))) {
        mgr.SendScriptMsg(actor, GetUniqueId(), EScriptObjectMessage::Activate);
        if (scriptEffect) {
          actor->SetTranslation(GetTranslation() + x120_effectPos);
          x11c_effectId = genObj.second;
        }
        if (scriptActor) {
          actor->SetTranslation(GetTranslation() + x100_actorPos);
          xfc_actorId = genObj.second;
        }
        if (scriptTrigger) {
          actor->SetTranslation(GetTranslation() + x110_triggerPos);
          x10c_triggerId = genObj.second;
        }
      }
    }
  }
}

void CScriptMazeNode::Reset(CStateManager& mgr) {
  mgr.FreeScriptObject(x11c_effectId);
  mgr.FreeScriptObject(xfc_actorId);
  mgr.FreeScriptObject(x10c_triggerId);
  mgr.FreeScriptObject(xf4_);
  xf4_ = kInvalidUniqueId;
  xfc_actorId = kInvalidUniqueId;
  x10c_triggerId = kInvalidUniqueId;
  x11c_effectId = kInvalidUniqueId;
}

void CScriptMazeState::Reset(s32 seed) {
  x0_rand.SetSeed(seed);
  x94_24_initialized = false;
  x4_arr.fill({});
  // TODO wtf is the rest?
}

void CScriptMazeState::Initialize() {
  std::array<size_t, 66> arr{};
  arr[0] = x84_ + x88_ * 9;
  x4_arr[arr[0]].x1_26_ = true;
  size_t i = 1;
  while (true) {
    if (arr[0] == x8c_ + x90_ * 9) {
      break;
    }
    if (x4_arr[arr[0]].x0_24_) {
      if (!x4_arr[arr[0] - 9].x1_26_) {
        arr[i] = arr[0] - 9;
        i++;
      }
    }
    if (x4_arr[arr[0]].x0_25_) {
      if (!x4_arr[arr[0] + 1].x1_26_) {
        arr[i] = arr[0] + 1;
        i++;
      }
    }
    if (x4_arr[arr[0]].x0_26_) {
      if (!x4_arr[arr[0] + 9].x1_26_) {
        arr[i] = arr[0] + 9;
        i++;
      }
    }
    if (x4_arr[arr[0]].x0_27_) {
      if (!x4_arr[arr[0] - 1].x1_26_) {
        arr[i] = arr[0] - 1;
        i++;
      }
    }
    if (arr[0] == arr[i - 1]) {
      i--;
    }
    arr[0] = arr[i - 1];
    x4_arr[arr[0]].x1_26_ = true;
  }
  size_t* v = &arr[i];
  while (true) {
    if (i == 0) {
      break;
    }
    i--;
    v--;
    if (x4_arr[*v].x1_26_) {
      x4_arr[*v].x1_25_ = true;
    }
  }
  x94_24_initialized = true;
}
} // namespace urde
