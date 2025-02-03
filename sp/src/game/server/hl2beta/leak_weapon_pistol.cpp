//========= Copyright  1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Pistol - hand gun
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h" // add Beta basehlcombatweapon?
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"

#define	PISTOL_FASTEST_REFIRE_TIME		0.1f
#define	PISTOL_FASTEST_DRY_REFIRE_TIME	0.2f

extern ConVar weapons_2003leakbehaviour;
extern ConVar weapons_2003leak_allowreloadsounds;

//-----------------------------------------------------------------------------
// CWeaponPistolLeak
//-----------------------------------------------------------------------------

class CWeaponPistolLeak : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponPistolLeak, CBaseHLCombatWeapon );

	CWeaponPistolLeak(void);

	DECLARE_SERVERCLASS();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
	
	virtual float GetFireRate( void ) { return 0.5f; }

    bool	Deploy( void );

	virtual bool Reload(void);
	
	DECLARE_ACTTABLE();

private:

	float	m_flSoonestPrimaryAttack;

};

IMPLEMENT_SERVERCLASS_ST(CWeaponPistolLeak, DT_WeaponPistolLeak)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_old, CWeaponPistolLeak );
PRECACHE_WEAPON_REGISTER( weapon_pistol_old );

BEGIN_DATADESC( CWeaponPistolLeak )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),

END_DATADESC()

acttable_t	CWeaponPistolLeak::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD, ACT_RELOAD_PISTOL, true },
};

IMPLEMENT_ACTTABLE( CWeaponPistolLeak );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPistolLeak::CWeaponPistolLeak( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;

	m_fMinRange1		= 65;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 65;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPistolLeak::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponPistolLeak::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistolLeak::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + PISTOL_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponPistolLeak::PrimaryAttack( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponPistolLeak::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		DryFire();
		return;
	}

	BaseClass::ItemPostFrame();
}

/*
==================================================
AddViewKick
==================================================
*/

void CWeaponPistolLeak::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( -2.0f, -1.0f );
	viewPunch.y = random->RandomFloat( 0.5f, 1.0f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponPistolLeak::Deploy(void)
{
	if (weapons_2003leakbehaviour.GetInt() == 0)
	{ 
		// temp code
		engine->ServerCommand("give weapon_pistol\n");
		engine->ServerCommand("use weapon_pistol\n");
		UTIL_Remove(this);
	}

	return BaseClass::Deploy();

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponPistolLeak::Reload(void)
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
	if (weapons_2003leak_allowreloadsounds.GetBool())	WeaponSound(RELOAD);
	}
	return fRet;
}