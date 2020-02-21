#if defined _hooksoundmsg_ext_included
  #endinput
#endif
#define _hooksoundmsg_ext_included

forward Action OnSoundEmittingToClient(int client, int origin[3], int& volume, float& delay, int& seq, int& entity, int& channel, int& pitch, int& flags, int& soundnum, int& soundnum_handle, int& speaker_entity, int& sound_level, bool is_sentence, bool is_ambient);

/**
 * Do not edit below this line!
 */

public Extension __ext_hooksoundmsg = 
{
	name = "hooksoundmsg",
	file = "hooksoundmsg.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
