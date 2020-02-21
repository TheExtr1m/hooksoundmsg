/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "extensions/ISDKTools.h"

#include "CDetour/detours.h"

#include <vector>
#include <mutex>

#include <inetchannel.h>
#include <inetmessage.h>
#include <netmessages.pb.h>

IGameConfig *g_pGameConf = nullptr;
CDetour *DSendNetMsg = nullptr;

class CNetMessagePB_Sounds;
static bool (CNetMessagePB_Sounds::*WriteToBuffer_Pointer)(bf_write &buffer) = nullptr;

class CNetMessagePB_Sounds : public INetMessage, public CSVCMsg_Sounds
{
public:
	virtual void	SetNetChannel(INetChannel * netchan) {}
	virtual void	SetReliable(bool state) {}

	virtual bool	Process(void) { return false; }

	virtual	bool	ReadFromBuffer(bf_read &buffer) { return false; }
	virtual	bool	WriteToBuffer(bf_write &buffer) {
		return (this->*WriteToBuffer_Pointer)(buffer);
	}

	virtual bool	IsReliable(void) const {
		return true;
	}
	virtual int				GetType(void) const {
		return 17;
	}
	virtual int				GetGroup(void) const {
		return 4;
	}
	virtual const char		*GetName(void) const {
		return "";
	}
	virtual INetChannel		*GetNetChannel(void) const {
		return NULL;
	}
	virtual const char		*ToString(void) const {
		return "";
	}
	virtual size_t			GetSize() const {
		return this->ByteSize();
	}
	
	CNetMessagePB_Sounds& operator=(const CNetMessagePB_Sounds& other) {
		CSVCMsg_Sounds::operator=(other);
		return *this;
	}
};

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

HookSoundMessages g_Hooker;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Hooker);

IForward *g_Forward = nullptr;

DETOUR_DECL_MEMBER3(DETOUR_SendNetMsg, bool, INetMessage &, msg, bool, bForceReliable, bool, bVoice)
{
	if (msg.GetType() != 17 || msg.GetGroup() != 4) {
		return DETOUR_MEMBER_CALL(DETOUR_SendNetMsg)(msg, bForceReliable, bVoice);
	}
	auto netchannel = (INetChannel*)this;
	IClient *pClient = (IClient *)netchannel->GetMsgHandler();
	if (pClient == nullptr) {
		return DETOUR_MEMBER_CALL(DETOUR_SendNetMsg)(msg, bForceReliable, bVoice);
	}
	int client = pClient->GetPlayerSlot() + 1;
	auto& sounds = *(CNetMessagePB_Sounds*)(&msg);

	CNetMessagePB_Sounds tempsounds;
	tempsounds = sounds;
	tempsounds.clear_sounds();
	for (int i = 0; i < sounds.sounds_size(); i++) {
		auto sound = sounds.sounds(i);
		int origin[3];
		origin[0] = (sound.has_origin_x()) ? sound.origin_x() : 0;
		origin[1] = (sound.has_origin_y()) ? sound.origin_y() : 0;
		origin[2] = (sound.has_origin_z()) ? sound.origin_z() : 0;
		int volume = (sound.has_volume()) ? sound.volume() : -1;
		float delay_value = (sound.has_delay_value()) ? sound.delay_value() : -1.0;
		int seq_num = (sound.has_sequence_number()) ? sound.sequence_number() : -1;
		int entity = (sound.has_entity_index()) ? sound.entity_index() : -1;
		int channel = (sound.has_channel()) ? sound.channel() : -1;
		int pitch = (sound.has_pitch()) ? sound.pitch() : 0;
		int flags = (sound.has_flags()) ? sound.flags() : 0;
		int sound_num = (sound.has_sound_num()) ? sound.sound_num() : -1;
		int sound_num_handle = (sound.has_sound_num_handle()) ? sound.sound_num_handle() : -1;
		int speaker_entity = (sound.has_speaker_entity()) ? sound.speaker_entity(): -1;
		//int random_seed = sound.random_seed();
		int sound_level = (sound.sound_level()) ? sound.sound_level() : -1;
		bool is_sentence = sound.is_sentence();
		bool is_ambient = sound.is_ambient();

		g_Forward->PushCell(client);
		g_Forward->PushArray(origin, 3, SM_PARAM_COPYBACK);
		g_Forward->PushCellByRef(&volume);
		g_Forward->PushFloatByRef(&delay_value);
		g_Forward->PushCellByRef(&seq_num);
		g_Forward->PushCellByRef(&entity);
		g_Forward->PushCellByRef(&channel);
		g_Forward->PushCellByRef(&pitch);
		g_Forward->PushCellByRef(&flags);
		g_Forward->PushCellByRef(&sound_num);
		g_Forward->PushCellByRef(&sound_num_handle);
		g_Forward->PushCellByRef(&speaker_entity);
		g_Forward->PushCellByRef(&sound_level);
		g_Forward->PushCell((is_sentence)? 1 : 0);
		g_Forward->PushCell((is_ambient)? 1 : 0);
		cell_t result = Pl_Continue;
		int err = g_Forward->Execute(&result);
		if (err != 0) {
			smutils->LogError(myself, "Cannot execute forward: %d", err);
			continue;
		}
		
		if (result == Pl_Changed) {
			if (origin[0] != 0 || origin[1] != 0 || origin[2] != 0) {
				sound.set_origin_x(origin[0]);
				sound.set_origin_y(origin[1]);
				sound.set_origin_z(origin[2]);
			}
			if (volume != -1) {
				sound.set_volume(volume);
			}
			if (delay_value != -1.0) {
				sound.set_delay_value(delay_value);
			}
			if (seq_num != -1) {
				sound.set_sequence_number(seq_num);
			}
			if (entity != -1) {
				sound.set_entity_index(entity);
			}
			if (channel != -1) {
				sound.set_channel(channel);
			}
			if (pitch != 0) {
				sound.set_pitch(pitch);
			}
			if (flags != 0) {
				sound.set_flags(flags);
			}
			if (sound_num != -1) {
				sound.set_sound_num(sound_num);
			}
			if (sound_num_handle != -1) {
				sound.set_sound_num_handle(sound_num_handle);
			}
			if (speaker_entity != -1) {
				sound.set_speaker_entity(speaker_entity);
			}
			if (sound_level != -1) {
				sound.set_sound_level(sound_level);
			}
		}
		else if (result >= Pl_Handled) {
			continue;
		}
		*(tempsounds.add_sounds()) = sound;
	}
	if (tempsounds.sounds_size() <= 0) {
		return true;
	}
	return DETOUR_MEMBER_CALL(DETOUR_SendNetMsg)(tempsounds, bForceReliable, bVoice);
}

bool HookSoundMessages::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "sdktools.ext", true, true);
	
	char conf_err[256];
	if (!gameconfs->LoadGameConfigFile("hooksoundmsg.games", &g_pGameConf, conf_err, sizeof(conf_err))) {
		smutils->Format(error, maxlength, "Cannot open hooksoundmsg.games gamedata: %s", conf_err);
		SDK_OnUnload();
		return false;
	}
	char* addr;
	if (!g_pGameConf->GetMemSig("CNetMessage::WriteToBuffer", (void**)&addr)) {
		smutils->Format(error, maxlength, "Cannot get memsig of CNetMessage::WriteToBuffer");
		SDK_OnUnload();
		return false;
	}
	*(intptr_t*)&WriteToBuffer_Pointer = (intptr_t)(addr + 5 + *(int*)(addr+1));

	CDetourManager::Init(smutils->GetScriptingEngine(), g_pGameConf);
	DSendNetMsg = DETOUR_CREATE_MEMBER(DETOUR_SendNetMsg, "CNetChan::SendNetMsg");
	if (DSendNetMsg == nullptr) {
		smutils->Format(error, maxlength, "Could not create detour for CNetChan::SendNetMsg");
		SDK_OnUnload();
		return false;
	}

	// public Action OnSoundEmittingToClient(int client, int& origin_x, int& origin_y, int& origin_z, int& volume, float& delay, int& seq, int& entity, int& channel, int& pitch, int& flags, int& soundnum, int& soundnum_handle, int& speaker_entity, int& sound_level, bool is_sentence, bool is_ambient);
	ParamType paramtypes[] = {
		Param_Cell,
		Param_Array,
		Param_CellByRef,
		Param_FloatByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_CellByRef,
		Param_Cell,
		Param_Cell,
	};
	g_Forward = forwards->CreateForward("OnSoundEmittingToClient", ET_Hook, 15, paramtypes);
	DSendNetMsg->EnableDetour();

	return true;
}

void HookSoundMessages::SDK_OnUnload()
{
	if (g_Forward != nullptr) {
		forwards->ReleaseForward(g_Forward);
		g_Forward = nullptr;
	}

	if (g_pGameConf != nullptr) {
		gameconfs->CloseGameConfigFile(g_pGameConf);
		g_pGameConf = nullptr;
	}
	if (DSendNetMsg != nullptr) {
		DSendNetMsg->Destroy();
		DSendNetMsg = nullptr;
	}
}
