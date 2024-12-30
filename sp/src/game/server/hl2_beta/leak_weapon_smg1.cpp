//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hl2_retail/basehlcombatweapon.h"
#include "NPCevent.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "game.h"
#include "in_buttons.h"
#include "hl2_beta/leak_grenade_ar2.h"
#include "AI_Memory.h"

extern ConVar    sk_plr_dmg_ar2_grenade;
extern ConVar weapons_2003leakbehaviour;
extern ConVar weapons_2003leak_allowreloadsounds;

class CWeaponSMG1Leak : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponSMG1Leak, CHLSelectFireMachineGun );

	CWeaponSMG1Leak();

	DECLARE_SERVERCLASS();
	
	void	Precache( void );
	void	AddViewKick( void );
	void	ItemPostFrame( void );
	void	SecondaryAttack( void );

	int		GetMinBurst() { return 4; }
	int		GetMaxBurst() { return 7; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual bool Reload(void);

	float	GetFireRate( void ) { return 0.1f; }
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	bool	Deploy(void);

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		switch( pEvent->event )
		{
			case EVENT_WEAPON_SMG1:
			{
				Vector vecShootOrigin, vecShootDir;
				QAngle angDiscard;
				if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
				{
					vecShootOrigin = pOperator->Weapon_ShootPosition();
				}

				CAI_BaseNPC *npc = pOperator->MyNPCPointer();
				ASSERT( npc != NULL );

				vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

				// all of these fire a bullet, but they make different sounds
				if ( pEvent->event == EVENT_WEAPON_SMG1 )
				{
					WeaponSound(SINGLE_NPC);
				}

				pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0 );
				pOperator->DoMuzzleFlash();
				m_iClip1 = m_iClip1 - 1;
			}
			break;
			
			/*//FIXME: Re-enable
			case EVENT_WEAPON_AR2_GRENADE:
			{
				CAI_BaseNPC *npc = pOperator->MyNPCPointer();

				Vector vecShootOrigin, vecShootDir;
				vecShootOrigin = pOperator->Weapon_ShootPosition();
				vecShootDir = npc->GetShootEnemyDir( vecShootOrigin );

				Vector vecThrow = m_vecTossVelocity;

				CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create( "grenade_ar2", vecShootOrigin, vec3_angle, npc );
				pGrenade->SetAbsVelocity( vecThrow );
				pGrenade->SetLocalAngularVelocity( QAngle( 0, 400, 0 ) );
				pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY ); 
				pGrenade->m_hOwner			= npc;
				pGrenade->m_pMyWeaponAR2	= this;
				pGrenade->SetDamage(sk_npc_dmg_ar2_grenade.GetFloat());

				// FIXME: arrgg ,this is hard coded into the weapon???
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.

				m_iClip2--;
			}
			break;
			*/
			
			default:
				BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
				break;
		}
	}

	DECLARE_ACTTABLE();

protected:

	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;

};

IMPLEMENT_SERVERCLASS_ST(CWeaponSMG1Leak, DT_WeaponSMG1Leak)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg1_old, CWeaponSMG1Leak );
PRECACHE_WEAPON_REGISTER(weapon_smg1_old);

BEGIN_DATADESC( CWeaponSMG1Leak )

	DEFINE_FIELD(  m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD(  m_flNextGrenadeCheck, FIELD_TIME ),

END_DATADESC()

acttable_t	CWeaponSMG1Leak::m_acttable[] = 
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					false },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			false },
	{ ACT_WALK,						ACT_WALK_RIFLE,					false },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				false },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			false },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		false },
	{ ACT_RUN,						ACT_RUN_RIFLE,					false },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				false },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			false },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		false },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
};

IMPLEMENT_ACTTABLE(CWeaponSMG1Leak);

//=========================================================
CWeaponSMG1Leak::CWeaponSMG1Leak( )
{
	m_fMinRange1		= 64;
	m_fMaxRange1		= 1400;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1Leak::Precache( void )
{
	UTIL_PrecacheOther("old_grenade_ar2");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponSMG1Leak::Equip( CBaseCombatCharacter *pOwner )
{
	if( pOwner->Classify() == CLASS_PLAYER_ALLY )
	{
		m_fMaxRange1 = 3000;
	}
	else
	{
		m_fMaxRange1 = 1400;
	}

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponSMG1Leak::Deploy(void)
{
	if (weapons_2003leakbehaviour.GetInt() == 0)
	{
		// temp code
		engine->ServerCommand("give weapon_smg1\n");
		engine->ServerCommand("use weapon_smg1\n");
		UTIL_Remove(this);
	}

	return BaseClass::Deploy();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1Leak::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	4.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1Leak::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		if (m_flNextSecondaryAttack <= gpGlobals->curtime)
		{
			SecondaryAttack();
			pOwner->m_nButtons &= ~IN_ATTACK2;
			return;
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1Leak::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//Must have ammo
	if ( pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) <= 0 )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		BaseClass::WeaponSound( EMPTY );
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	BaseClass::WeaponSound(WPN_DOUBLE);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecThrow = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES ) * 1000.0f;
	
	//Create the grenade
	CGrenadeAR2Leak *pGrenade = (CGrenadeAR2Leak*)Create( "old_grenade_ar2", vecSrc, vec3_angle, pPlayer );
	pGrenade->SetAbsVelocity( vecThrow );

	pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY ); 
	//pGrenade->SetOwner(GetBaseEntity()); //GetOwner
	pGrenade->SetDamage( sk_plr_dmg_ar2_grenade.GetFloat() );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Decrease ammo
	pPlayer->RemoveAmmo( 1, m_iSecondaryAmmoType );

	// Can shoot again immediately
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	// Can blow up after a short delay (so have time to release mouse button)
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	// Register a muzzleflash for the AI.
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );	
}

#define	COMBINE_MIN_GRENADE_CLEAR_DIST 256

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponSMG1Leak::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	CAI_BaseNPC *npcOwner = GetOwner()->MyNPCPointer();

	return COND_NONE;

/*
	// --------------------------------------------------------
	// Assume things haven't changed too much since last time
	// --------------------------------------------------------
	if (gpGlobals->curtime < m_flNextGrenadeCheck )
		return m_lastGrenadeCondition;
*/

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( npcOwner->IsMoving())
		return COND_NONE;

	CBaseEntity *pEnemy = npcOwner->GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector vecEnemyLKP = npcOwner->GetEnemyLKP();
	if (!(pEnemy->GetFlags() & FL_ONGROUND) && pEnemy->GetWaterLevel() == 0 && vecEnemyLKP.z > WorldAlignMaxs().z) 
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}
	
	// --------------------------------------
	//  Get target vector
	// --------------------------------------
	Vector vecTarget;
	if (random->RandomInt(0,1))
	{
		// magically know where they are
		vecTarget = pEnemy->WorldSpaceCenter();
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = vecEnemyLKP;
	}
	// vecTarget = m_vecEnemyLKP + (pEnemy->BodyTarget( GetLocalOrigin() ) - pEnemy->GetLocalOrigin());
	// estimate position
	// vecTarget = vecTarget + pEnemy->m_vecVelocity * 2;


	if ( ( vecTarget - npcOwner->GetLocalOrigin() ).Length2D() <= COMBINE_MIN_GRENADE_CLEAR_DIST )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return (COND_NONE);
	}

	// ---------------------------------------------------------------------
	// Are any friendlies near the intended grenade impact area?
	// ---------------------------------------------------------------------
	CBaseEntity *pTarget = NULL;

	while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST ) ) != NULL )
	{
		//Check to see if the default relationship is hatred, and if so intensify that
		if ( npcOwner->IRelationType( pTarget ) == D_LI )
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
			return (COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: speed is based on difficulty...

	Vector vecToss = VecCheckThrow( this, npcOwner->GetLocalOrigin() + Vector(0,0,60), vecTarget, 600.0, 0.5 );
	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		// JAY: HL1 keeps checking - test?
		//m_flNextGrenadeCheck = gpGlobals->curtime;
		m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.
		return COND_CAN_RANGE_ATTACK2;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_WEAPON_SIGHT_OCCLUDED;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponSMG1Leak::Reload(void)
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		if (weapons_2003leak_allowreloadsounds.GetBool())	WeaponSound(RELOAD);
	}
	return fRet;
}