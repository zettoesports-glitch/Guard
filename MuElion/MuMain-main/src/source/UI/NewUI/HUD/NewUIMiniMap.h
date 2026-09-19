//////////////////////////////////////////////////////////////////////
// NewUIGuildInfoWindow.h: interface for the CNewUIGuildInfoWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/HUD/NewUIMainFrameWindow.h"
#include "UI/NewUI/HUD/NewUIChatLogWindow.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SEASON3B
{
    struct MiniMapSnapshotMarker
    {
        int kind = 0;
        int x = 0;
        int y = 0;
        int rotation = 0;
        std::wstring name;
    };

    struct MiniMapSnapshot
    {
        bool visible = false;
        bool textureReady = false;
        std::uint32_t textureId = 0;
        std::uint32_t textureWidth = 0;
        std::uint32_t textureHeight = 0;
        int viewportWidth = 0;
        int viewportHeight = 0;
        int heroX = 0;
        int heroY = 0;
        float mapWidth = 800.0f;
        float mapHeight = 800.0f;
        std::vector<MiniMapSnapshotMarker> markers;
    };

    class CNewUIMiniMap : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_MINIMAP_INTERFACE = BITMAP_MINI_MAP_BEGIN,
        };

        enum MASTER_DATA
        {
            SKILL_ICON_DATA_WDITH = 4,
            SKILL_ICON_DATA_HEIGHT = 8,
            SKILL_ICON_WIDTH = 20,
            SKILL_ICON_HEIGHT = 28,
            SKILL_ICON_STARTX1 = 75,
            SKILL_ICON_STARTY1 = 75,
        };

        enum EVENT_STATE
        {
            EVENT_NONE = 0,
            EVENT_SCROLL_BTN_DOWN,
        };

    private:
       std::wstring		m_TooltipText;
        HFONT					m_hToolTipFont;
        DWORD					m_TooltipTextColor;

        CNewUIManager* m_pNewUIMng;
        POINT					m_Pos;
        POINT					m_Width;
        POINT					m_MiniWidth;
        POINT					m_Lenth[6];
        int						m_MiniPos;
        CNewUIButton			m_BtnExit;
        MINI_MAP				m_Mini_Map_Data[MAX_MINI_MAP_DATA];
        float					m_Btn_Loc[MAX_MINI_MAP_DATA][4];

    public:
        bool					m_bSuccess;
        CNewUIMiniMap();
        virtual ~CNewUIMiniMap();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        void SetPos(int x, int y);
        void SetBtnPos(int Num, float x, float y, float nx, float ny);

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();

        float GetLayerDepth();	//. 8.1f

        void OpenningProcess();
        void ClosingProcess();
        void OpenMasterLevel(const wchar_t* filename);

        // Read-only projection for the reconstructed RmlUi minimap. It never
        // exposes MINI_MAP pointers or transfers bitmap ownership.
        void BuildSnapshot(MiniMapSnapshot& snapshot) const;

        CNewUIButton m_BtnToolTip;
        void LoadImages(const wchar_t* Filename);
        void UnloadImages();

    private:
        void Render_Text();
        void Render_Icon();
        void Render_Scroll();
        bool Check_Mouse(int mx, int my);
        bool Check_Btn(int mx, int my);
    };
}
