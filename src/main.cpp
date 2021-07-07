#include <dllentry.h>
#include "main.h"

std::unordered_map<std::string, bool> shouldResendImmobilityStatus;

class ImmobileCommand : public Command {
public:

  ImmobileCommand() { selector.setIncludeDeadPlayers(true); }

  //initialize command arguments
  CommandSelector<Player> selector;
  bool toggle = true;

  void setImmobilityStatus(Player *player, bool toggle) {
    CallServerClassMethod<bool>("?setStatusFlag@Actor@@QEAA_NW4ActorFlags@@_N@Z", player, ActorFlags::IMMOBILE, toggle);
    std::string immobilityStatusString = toggle ? "You have been immobilized" : "You have been mobilized";
    auto pkt = TextPacket::createTextPacket<TextPacketType::SystemMessage>(immobilityStatusString);
    player->sendNetworkPacket(pkt);
  }

  void execute(CommandOrigin const &origin, CommandOutput &output) {

    //get selected entity/entities
    auto selectedEntities = selector.results(origin);
    if (selectedEntities.empty()) {
      output.error("No targets matched selector");
      return;
    }

    for (auto player : selectedEntities) {
      setImmobilityStatus(player, toggle);
    }
    
    if (toggle) {
      output.success("Immobilized " + std::to_string(selectedEntities.count()) + (selectedEntities.count() == 1 ? " player" : " players"));
    }
    else {
      output.success("Mobilized " + std::to_string(selectedEntities.count()) + (selectedEntities.count() == 1 ? " player" : " players"));
    }
  }

  static void setup(CommandRegistry *registry) {
    using namespace commands;
    registry->registerCommand(
      "immobile", "Locks player movement.", CommandPermissionLevel::GameMasters, CommandFlagCheat, CommandFlagNone);
    registry->registerOverload<ImmobileCommand>("immobile",
      mandatory(&ImmobileCommand::selector, "target"),
      optional(&ImmobileCommand::toggle, "toggle")
    );
  }
};

void initCommand(CommandRegistry *registry) {
    ImmobileCommand::setup(registry);
}

void dllenter() {}
void dllexit() {}

//by default, immobile flag clears when a player leaves the world
//hack: resend flag if immobile player leaves and rejoins
void PreInit() {
  Mod::CommandSupport::GetInstance().AddListener(SIG("loaded"), initCommand);

  Mod::PlayerDatabase::GetInstance().AddListener(SIG("joined"), [](Mod::PlayerEntry const &entry) {
    if (shouldResendImmobilityStatus[entry.player->getEntityName()]) {
      ImmobileCommand ic;
      ic.setImmobilityStatus(entry.player, true);
      shouldResendImmobilityStatus[entry.player->getEntityName()] = false;
    }
  });
  Mod::PlayerDatabase::GetInstance().AddListener(SIG("left"), [](Mod::PlayerEntry const &entry) {
    if (entry.player->isImmobile()) {
      shouldResendImmobilityStatus[entry.player->getEntityName()] = true;
    }
  });
}
void PostInit() {}