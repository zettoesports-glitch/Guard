#include "stdafx.h"
#include "UI/Modern/PC/SystemMenu/RmlWindowMenuLegacyBridge.h"
#include "UI/Modern/PC/SystemMenu/RmlWindowMenuPanel.h"
#include "UI/NewUI/Dialogs/NewUIWindowMenu.h"

namespace UI::Modern::PC::SystemMenu
{
bool RmlWindowMenuLegacyBridge::Synchronize(
    const SEASON3B::CNewUIWindowMenu& legacy,
    RmlWindowMenuPanel& modern)
{
    SEASON3B::WindowMenuSnapshot snapshot;
    legacy.BuildSnapshot(snapshot);
    RmlWindowMenuPanel::State state;
    state.visible=snapshot.visible;
    state.viewportWidth=snapshot.viewportWidth;
    state.viewportHeight=snapshot.viewportHeight;
    state.x=static_cast<float>(snapshot.x);
    state.y=static_cast<float>(snapshot.y);
    return modern.ApplyState(state);
}
}
