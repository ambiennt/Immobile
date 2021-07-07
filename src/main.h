#pragma once

#include <base/log.h>
#include <mods/CommandSupport.h>
#include <base/playerdb.h>
#include <Actor/Actor.h>
#include <Actor/Player.h>
#include <Packet/TextPacket.h>

void initCommand(CommandRegistry *registry);

DEF_LOGGER("ImmobileCommand");