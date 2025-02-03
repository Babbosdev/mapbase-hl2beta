//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CS_GAMEVARS_SHARED_H
#define CS_GAMEVARS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

extern ConVar mp_forcecamera;
extern ConVar mp_allowspectators;
extern ConVar friendlyfire;
extern ConVar mp_fadetoblack;

// mapbase-hl2beta

#ifdef HL2_CLIENT_DLL
extern ConVar EnableRetailHud;
#endif

#ifdef GAME_DLL
extern ConVar weapons_2003leakbehaviour;
extern ConVar weapons_2003leak_allowreloadsounds;
#endif

#endif // CS_GAMEVARS_SHARED_H
