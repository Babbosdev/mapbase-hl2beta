//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hl2_beta/leak_hudsuitpower.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include "iclientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>

extern ConVar   EnableRetailHud;

//-----------------------------------------------------------------------------
// Purpose: Shows the flashlight icon
//-----------------------------------------------------------------------------
class CHudFlashlightLeak : public CHudElement, public vgui::Panel
{ 
	DECLARE_CLASS_SIMPLE( CHudFlashlightLeak, vgui::Panel );

public:
	CHudFlashlightLeak( const char *pElementName );
	virtual void Init( void );
	void SetFlashlightState( bool flashlightOn );
	bool ShouldDraw(void);

protected:
	virtual void Paint();
	

private:
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "20", "proportional_float" );

	bool m_bFlashlightOn;
};	

using namespace vgui;

DECLARE_HUDELEMENT( CHudFlashlightLeak );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudFlashlightLeak::CHudFlashlightLeak( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudFlashlightLeak" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_bFlashlightOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudFlashlightLeak::Init()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudFlashlightLeak::ShouldDraw(void)
{

	if (EnableRetailHud.GetInt())
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return false;
	}
	else
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
		return true;
	}

	return true;

}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudFlashlightLeak::SetFlashlightState( bool flashlightOn )
{
	if ( m_bFlashlightOn == flashlightOn )
		return;


	if ( flashlightOn )
	{
		// flashlight on
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitFlashlightOn_Leak");
	}
	else
	{
		// flashlight off
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitFlashlightOff_Leak");
	}

	m_bFlashlightOn = flashlightOn;
}

//-----------------------------------------------------------------------------
// Purpose: draws the flashlight icon
//-----------------------------------------------------------------------------
void CHudFlashlightLeak::Paint()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	SetFlashlightState( pPlayer->IsEffectActive( EF_DIMLIGHT ) ); 

	// draw our name
	const wchar_t *text = L"FLASHLIGHT: ON";
	if (!m_bFlashlightOn)
	{
		text = L"FLASHLIGHT: OFF";
	}

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_TextColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	for (const wchar_t *wch = text; *wch != 0; wch++)
	{
		surface()->DrawUnicodeChar(*wch);
	}
}


