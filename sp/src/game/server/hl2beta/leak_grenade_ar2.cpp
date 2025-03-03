//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "leak_grenade_ar2.h"
#include "weapon_ar2.h"
#include "soundent.h"
#include "decals.h"
#include "shake.h"
#include "smoke_trail.h"
#include "ar2_explosion.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "world.h"

#if 1
// Moved to HL2_SharedGameRules because these are referenced by shared AmmoDef functions
extern ConVar    sk_plr_dmg_ar2_grenade;
extern ConVar    sk_npc_dmg_ar2_grenade;
extern ConVar    sk_max_ar2_grenade;
#else
// Placeholder
ConVar	  sk_plr_dmg_ar2_grenade("sk_plr_dmg_ar2_grenade", "100");
ConVar	  sk_npc_dmg_ar2_grenade("sk_npc_dmg_ar2_grenade", "120");
ConVar	  sk_max_ar2_grenade("sk_max_ar2_grenade", "256");
ConVar g_CV_SmokeTrail("smoke_trail", "1", 0); // temporary dust explosion switch
#endif

extern short	g_sModelIndexFireball;			// (in combatweapon.cpp) holds the index for the smoke cloud
ConVar	  sk_ar2_grenade_radius		( "sk_ar2_grenade_radius","0");
ConVar g_CV_DustExplosion("dust_explosion", "0", 0); // temporary dust explosion switch
extern ConVar    g_CV_SmokeTrail; // temporary dust explosion switch


BEGIN_DATADESC( CGrenadeAR2Leak )

	DEFINE_FIELD( m_pSmokeTrail, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_fSpawnTime, FIELD_TIME ),

	// Function pointers
	DEFINE_ENTITYFUNC(GrenadeAR2Touch),
	DEFINE_THINKFUNC(GrenadeAR2Think),

END_DATADESC()

LINK_ENTITY_TO_CLASS(old_grenade_ar2, CGrenadeAR2Leak);



void CGrenadeAR2Leak::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY );

	SetModel( "models/Weapons/ar2_grenade.mdl");
	UTIL_SetSize(this, Vector(-3, -3, -3), Vector(3, 3, 3));

	SetUse(&CGrenadeAR2Leak::DetonateUse);
	SetTouch(&CGrenadeAR2Leak::GrenadeAR2Touch);
	SetThink(&CGrenadeAR2Leak::GrenadeAR2Think);
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_ar2_grenade.GetFloat();
	m_DmgRadius		= sk_ar2_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_bIsLive		= false;
	m_iHealth		= 1;

	SetGravity( 0.5 );
	SetFriction( 0.8 );

	m_fSpawnTime = gpGlobals->curtime;

	// -------------
	// Smoke trail.
	// -------------
	if(g_CV_SmokeTrail.GetInt())
	{
		m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
		
		if( m_pSmokeTrail )
		{
			m_pSmokeTrail->m_SpawnRate = 48;
			m_pSmokeTrail->m_ParticleLifetime = 1;
			m_pSmokeTrail->m_StartColor.Init(0.1f, 0.1f, 0.1f);
			m_pSmokeTrail->m_EndColor.Init(0,0,0);
			m_pSmokeTrail->m_StartSize = 12;
			m_pSmokeTrail->m_EndSize = m_pSmokeTrail->m_StartSize * 4;
			m_pSmokeTrail->m_SpawnRadius = 4;
			m_pSmokeTrail->m_MinSpeed = 4;
			m_pSmokeTrail->m_MaxSpeed = 24;
			m_pSmokeTrail->m_Opacity = 0.2f;
			m_pSmokeTrail->SetLifetime(10.0f);
			m_pSmokeTrail->FollowEntity(GetBaseEntity()); // was entindex()
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  The grenade has a slight delay before it goes live.  That way the
//			 person firing it can bounce it off a nearby wall.  However if it
//			 hits another character it blows up immediately
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CGrenadeAR2Leak::GrenadeAR2Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (!m_bIsLive)
	{
		// Go live after a short delay
		if (m_fSpawnTime + MAX_AR2_NO_COLLIDE_TIME < gpGlobals->curtime)
		{
			m_bIsLive  = true;
		}
	}
	
	// If I just went solid and my velocity is zero, it means I'm resting on
	// the floor already when I went solid so blow up
	if (m_bIsLive)
	{
		if (GetAbsVelocity().Length() == 0.0)
		{
			Detonate();
		}
	}

	CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length(), 0.2 );
}

void CGrenadeAR2Leak::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

void CGrenadeAR2Leak::GrenadeAR2Touch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	// If I'm live go ahead and blow up
	if (m_bIsLive)
	{
		Detonate();
	}
	else
	{
		// If I'm not live, only blow up if I'm hitting an chacter that
		// is not the owner of the weapon
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pOther );
		if (pBCC && GetBaseEntity() != pBCC) // GetOwner
		{ 
			m_bIsLive = true;
			Detonate();
		}
	}
}

void CGrenadeAR2Leak::Detonate(void)
{
	if (!m_bIsLive)
	{
		return;
	}
	m_bIsLive		= false;
	m_takedamage	= DAMAGE_NO;	

	if(m_pSmokeTrail)
	{
		UTIL_RemoveImmediate(m_pSmokeTrail);
		m_pSmokeTrail = NULL;
	}

	// Create a particle explosion.
	if ( g_CV_DustExplosion.GetBool() )
	{
		if(AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion(GetAbsOrigin()))
		{
			pExplosion->SetLifetime(10);
		}
	}

	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		2.0, 
		15,
		TE_EXPLFLAG_NONE,
		m_DmgRadius,
		m_flDamage );

	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward, MASK_SOLID, 
		this, COLLISION_GROUP_NONE, &tr);

	if ((tr.m_pEnt != GetWorldEntity()) || (tr.hitbox != 0)) 
	{
		// non-world needs smaller decals
		UTIL_DecalTrace( &tr, "SmallScorch" );
	}
	else
	{
		UTIL_DecalTrace( &tr, "Scorch" );
	}

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
//	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	RadiusDamage(CTakeDamageInfo(this, GetThrower(), m_flDamage, DMG_BLAST), GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL);
    UTIL_Remove( this );
}

void CGrenadeAR2Leak::Precache( void )
{
	PrecacheModel("models/Weapons/ar2_grenade.mdl"); 
}


CGrenadeAR2Leak::CGrenadeAR2Leak(void)
{
	Precache();
	m_pSmokeTrail  = NULL;
}