#include "CScriptDockAreaChange.hpp"
#include "CStateManager.hpp"
#include "World/CScriptDock.hpp"

namespace urde
{
CScriptDockAreaChange::CScriptDockAreaChange(TUniqueId uid, const std::string& name, const CEntityInfo& info, s32 w1,
                                             bool active)
: CEntity(uid, info, active, name), x34_dockReference(w1)
{
}

void CScriptDockAreaChange::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId objId, CStateManager& stateMgr)
{
    if (msg == EScriptObjectMessage::Action && GetActive())
    {
        for (SConnection conn : x20_conns)
        {
            if (conn.x0_state != EScriptObjectState::Play)
                continue;

            auto search = stateMgr.GetIdListForScript(conn.x8_objId);
            for (auto it = search.first ; it != search.second ; ++it)
            {
                TUniqueId id = it->second;
                CScriptDock* dock = dynamic_cast<CScriptDock*>(stateMgr.ObjectById(id));
                if (dock)
                    dock->SetDockReference(x34_dockReference);
            }
        }

        SendScriptMsgs(EScriptObjectState::Play, stateMgr, EScriptObjectMessage::None);
    }

    CEntity::AcceptScriptMsg(msg, objId, stateMgr);
}
}
