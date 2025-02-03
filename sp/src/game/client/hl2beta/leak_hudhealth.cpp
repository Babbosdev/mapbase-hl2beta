/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Health.cpp
//
// implementation of CHudHealthLeak class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"
#include "parsemsg.h"

#include "iclientmode.h"

#define PAIN_NAME "sprites/%d_pain.vmt"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "ConVar.h"

#define INIT_HEALTH -1

extern ConVar   EnableRetailHud;

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealthLeak : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudHealthLeak, CHudNumericDisplay);

public:
	CHudHealthLeak(const char *pElementName);
	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual void			OnThink();
	void MsgFunc_Health(const char *pszName, int iSize, void *pbuf);
	void MsgFunc_DamageLeak(bf_read &msg);

private:
	// old variables
	int		m_iHealth;

	float	m_fFade;
	int		m_bitsDamage;
	int		m_iGhostHealth;
};

DECLARE_HUDELEMENT(CHudHealthLeak);
DECLARE_HUD_MESSAGE(CHudHealthLeak, DamageLeak); 

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealthLeak::CHudHealthLeak(const char *pElementName) : CHudElement(pElementName), CHudNumericDisplay(NULL, "HudHealthLeak")
{
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthLeak::Init()
{
	HOOK_HUD_MESSAGE(CHudHealthLeak, DamageLeak);
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthLeak::Reset()
{
	m_iHealth = INIT_HEALTH;
	m_iGhostHealth = 100;
	m_fFade = 0;
	m_bitsDamage = 0;

	SetLabelText(L"HEALTH");
	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthLeak::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthLeak::OnThink()
{

	if (EnableRetailHud.GetInt())
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}
	else
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
	}

	int x = 0;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if (local)
	{
		// Never below zero
		x = max(local->GetHealth(), 0);
	}

	// Only update the fade if we've changed health
	if (x == m_iHealth)
	{
		return;
	}

	m_iGhostHealth = m_iHealth;
	m_fFade = 200;
	m_iHealth = x;


	if (m_iHealth >= 20)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedAbove20_Leak");
	}
	else if (m_iHealth > 0)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedBelow20_Leak");
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthLow_Leak");
	}

	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthLeak::MsgFunc_DamageLeak(bf_read &msg)
{

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits
	bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = READ_COORD();
	vecFrom.y = READ_COORD();
	vecFrom.z = READ_COORD();

	// Actually took damage?
	if (damageTaken > 0 || armor > 0)
	{
		if (damageTaken > 0)
		{
			// start the animation
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthDamageTaken_Leak");
		}
	}
}

