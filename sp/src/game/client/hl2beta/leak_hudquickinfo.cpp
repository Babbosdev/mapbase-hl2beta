//========= Copyright Valve Corporation, MilkywayM16, All rights reserved. ============//
//
// This Code comes from this repo: https://github.com/MilkywayM16/hl2-old-hud
// So Thanks MilkywayM16!
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	HEALTH_WARNING_THRESHOLD	25

static ConVar	beta_hudquickinfo("beta_hud_quickinfo", "1", FCVAR_ARCHIVE);

extern ConVar   EnableRetailHud;
extern ConVar   crosshair;

#define QUICKINFO_EVENT_DURATION	1.0f
#define	QUICKINFO_BRIGHTNESS_FULL	255
#define	QUICKINFO_BRIGHTNESS_DIM	30
#define	QUICKINFO_FADE_IN_TIME		0.5f
#define QUICKINFO_FADE_OUT_TIME		2.0f
#define	QINFO_FADE_TIME				150

/*
==================================================
CHUDQuickInfoLeak
==================================================
*/

using namespace vgui;

class CHUDQuickInfoLeak : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHUDQuickInfoLeak, vgui::Panel);
public:
	CHUDQuickInfoLeak(const char *pElementName);
	void Init(void);
	void VidInit(void);
	bool ShouldDraw(void);
	virtual void OnThink();
	virtual void Paint();

	virtual void ApplySchemeSettings(IScheme *scheme);
private:

	void	DrawWarning(int x, int y, CHudTexture *icon, float &time);
	void	UpdateEventTime(void);
	bool	EventTimeElapsed(void);

	int		m_lastAmmo;
	int		m_lastHealth;

	float	m_ammoFade;
	float	m_healthFade;
	float	m_fDamageFade;
	float	m_fFade;

	bool	m_warnAmmo;
	bool	m_warnHealth;

	bool	m_bFadedOut;

	bool	m_bDimmed;			// Whether or not we are dimmed down
	float	m_flLastEventTime;	// Last active event (controls dimmed state)

	CHudTexture	*m_icon_c;

	CHudTexture	*m_icon_rbn;	// right bracket
	CHudTexture	*m_icon_lbn;	// left bracket

	CHudTexture	*m_icon_rb;		// right bracket, full
	CHudTexture	*m_icon_lb;		// left bracket, full
	CHudTexture	*m_icon_rbe;	// right bracket, empty
	CHudTexture	*m_icon_lbe;	// left bracket, empty
};

DECLARE_HUDELEMENT(CHUDQuickInfoLeak);

CHUDQuickInfoLeak::CHUDQuickInfoLeak(const char *pElementName) :
CHudElement(pElementName), BaseClass(NULL, "HUDQuickInfoLeak")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits(HIDEHUD_CROSSHAIR);
}

void CHUDQuickInfoLeak::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
}


void CHUDQuickInfoLeak::Init(void)
{
	m_ammoFade = 0.0f;
	m_healthFade = 0.0f;
	m_fFade = 0.0f;

	m_lastAmmo = 0;
	m_lastHealth = 100;

	m_warnAmmo = false;
	m_warnHealth = false;

	m_bFadedOut = false;
	m_bDimmed = false;
	m_flLastEventTime = 0.0f;
}


void CHUDQuickInfoLeak::VidInit(void)
{
	Init();

	m_icon_c = gHUD.GetIcon  ("leakquickinfo_crosshair");
	m_icon_rb = gHUD.GetIcon ("leakquickinfo_right_full");
	m_icon_lb = gHUD.GetIcon ("leakquickinfo_left_full");
	m_icon_rbe = gHUD.GetIcon("leakquickinfo_right_empty");
	m_icon_lbe = gHUD.GetIcon("leakquickinfo_left_empty");
	m_icon_rbn = gHUD.GetIcon("leakquickinfo_right");
	m_icon_lbn = gHUD.GetIcon("leakquickinfo_left");
}


void CHUDQuickInfoLeak::DrawWarning(int x, int y, CHudTexture *icon, float &time)
{
	float scale = (int)(fabs(sin(gpGlobals->curtime*8.0f)) * 128.0);

	// Only fade out at the low point of our blink
	if (time <= (gpGlobals->frametime * 200.0f))
	{
		if (scale < 40)
		{
			time = 0.0f;
			return;
		}
		else
		{
			// Counteract the offset below to survive another frame
			time += (gpGlobals->frametime * 200.0f);
		}
	}

	// Update our time
	time -= (gpGlobals->frametime * 200.0f);
	Color caution = gHUD.m_clrCaution;
	caution[3] = scale * 255;

	icon->DrawSelf(x, y, caution);
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHUDQuickInfoLeak::ShouldDraw(void)
{
	if (!m_icon_c || !m_icon_rb || !m_icon_rbe || !m_icon_lb || !m_icon_lbe)
		return false;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if (player == NULL)
		return false;

	if (!crosshair.GetBool() && !IsX360())
		return false;

	if (!beta_hudquickinfo.GetInt())
		return false;

	if (EnableRetailHud.GetInt())
		return false;

	return (CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage());
}

//-----------------------------------------------------------------------------
// Purpose: Checks if the hud element needs to fade out
//-----------------------------------------------------------------------------
void CHUDQuickInfoLeak::OnThink()
{
	BaseClass::OnThink();

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if (player == NULL)
		return;

	// see if we should fade in/out
	bool bFadeOut = player->IsZoomed();

	// check if the state has changed
	if (m_bFadedOut != bFadeOut)
	{
		m_bFadedOut = bFadeOut;

		m_bDimmed = false;

		if (bFadeOut)
		{
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", 0.0f, 0.0f, 0.25f, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_FULL, 0.0f, QUICKINFO_FADE_IN_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else if (!m_bFadedOut)
	{
		// If we're dormant, fade out
		if (EventTimeElapsed())
		{
			if (!m_bDimmed)
			{
				m_bDimmed = true;
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_DIM, 0.0f, QUICKINFO_FADE_OUT_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
			}
		}
		else if (m_bDimmed)
		{
			// Fade back up, we're active
			m_bDimmed = false;
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_FULL, 0.0f, QUICKINFO_FADE_IN_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
	}
}

void CHUDQuickInfoLeak::Paint()
{
	CHudTexture	*icon_c = gHUD.GetIcon("leakquickinfo_crosshair");
	CHudTexture	*icon_rb = gHUD.GetIcon("leakquickinfo_right");
	CHudTexture	*icon_lb = gHUD.GetIcon("leakquickinfo_left");

	if (!icon_c || !icon_rb || !icon_lb)
		return;

	int		xCenter = (ScreenWidth() / 2) - icon_c->Width() / 2;
	int		yCenter = (ScreenHeight() / 2) - icon_c->Height() / 2;
	//int		scalar;
	float	scalar = 1.0f; //138.0f/255.0f;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

	if (player == NULL)
		return;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if (pWeapon == NULL)
		return;

	//Get our values
	int	health = player->GetHealth();
	int	ammo = pWeapon->Clip1();

	if (m_fDamageFade > 0.0f)
	{
		m_fDamageFade -= (gpGlobals->frametime * 200.0f);
	}

	//Check our health for a warning
	if (health != m_lastHealth)
	{
		UpdateEventTime();
		if (health < m_lastHealth)
		{
			m_fDamageFade = QINFO_FADE_TIME;
		}

		m_fFade = QINFO_FADE_TIME;
		m_lastHealth = health;

		if (health <= HEALTH_WARNING_THRESHOLD)
		{
			if (m_warnHealth == false)
			{
				m_healthFade = 255;
				m_warnHealth = true;

				CLocalPlayerFilter filter;
				if (!EnableRetailHud.GetBool() && beta_hudquickinfo.GetBool()) C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowHealth");
			}
		}
		else
		{
			m_warnHealth = false;
		}
	}

	// Check our ammo for a warning
	if (ammo != m_lastAmmo)
	{
		UpdateEventTime();
		m_fFade = QINFO_FADE_TIME;
		m_lastAmmo = ammo;

		// Find how far through the current clip we are
		float ammoPerc = (float)ammo / (float)pWeapon->GetMaxClip1();

		if ((pWeapon->GetMaxClip1() > 1) && (ammoPerc <= (1.0f - CLIP_PERC_THRESHOLD)))
		{
			if (m_warnAmmo == false)
			{
				m_ammoFade = 255;
				m_warnAmmo = true;

				CLocalPlayerFilter filter;
				if (!EnableRetailHud.GetBool() && beta_hudquickinfo.GetBool()) C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowAmmo");
			}
		}
		else
		{
			m_warnAmmo = false;
		}
	}

	Color clrNormal = gHUD.m_clrNormal;
	clrNormal[3] = 255 * scalar;
	icon_c->DrawSelf(xCenter, yCenter, clrNormal);

	int	sinScale = (int)(fabs(sin(gpGlobals->curtime*8.0f)) * 128.0f);

	//Update our health
	if (m_healthFade > 0.0f)
	{
		DrawWarning(xCenter - 10, yCenter - 5, icon_lb, m_healthFade);
	}
	else
	{
		float	healthPerc = (float)health / 100.0f;

		Color healthColor = m_warnHealth ? gHUD.m_clrCaution : gHUD.m_clrNormal;

		if (m_warnHealth)
		{
			healthColor[3] = 255 * sinScale;
		}
		else
		{
			healthColor[3] = 255 * scalar;
		}

		int width = icon_lb->Width();
		int height = icon_lb->Height();

		gHUD.DrawIconProgressBar(xCenter - 10, yCenter - 5, width, height, icon_lb, (1.0f - healthPerc), healthColor, CHud::HUDPB_VERTICAL);
	}

	//Update our ammo
	if (m_ammoFade > 0.0f)
	{
		DrawWarning(xCenter + icon_rb->Width() - 6, yCenter - 5, icon_rb, m_ammoFade);
	}
	else
	{
		float ammoPerc;// = 1.0f - ((float)ammo / (float)pWeapon->GetMaxClip1());

		if (pWeapon->GetMaxClip1() <= 0)
		{
			ammoPerc = 0.0f;
		}
		else
		{
			ammoPerc = 1.0f - ((float)ammo / (float)pWeapon->GetMaxClip1());
			ammoPerc = clamp(ammoPerc, 0.0f, 1.0f);
		}


		Color ammoColor = m_warnAmmo ? gHUD.m_clrCaution : gHUD.m_clrNormal;

		if (m_warnAmmo)
		{
			ammoColor[3] = 255 * sinScale;
		}
		else
		{
			ammoColor[3] = 255 * scalar;
		}

		int width = icon_rb->Width();
		int height = icon_rb->Height();

		gHUD.DrawIconProgressBar(xCenter + icon_rb->Width() - 6, yCenter - 5, width, height, icon_rb, ammoPerc, ammoColor, CHud::HUDPB_VERTICAL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHUDQuickInfoLeak::UpdateEventTime(void)
{
	m_flLastEventTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHUDQuickInfoLeak::EventTimeElapsed(void)
{
	if ((gpGlobals->curtime - m_flLastEventTime) > QUICKINFO_EVENT_DURATION)
		return true;

	return false;
}
