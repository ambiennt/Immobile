#include <dllentry.h>
#include "main.h"

std::unordered_set<std::string> shouldResendImmobilityStatus;

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

    bool isImmobileInMap = shouldResendImmobilityStatus.count(player->getEntityName());
    if (toggle) {
      if (!isImmobileInMap) {
        shouldResendImmobilityStatus.insert(player->getEntityName());
      }
    }
    else if (isImmobileInMap) {
      shouldResendImmobilityStatus.erase(player->getEntityName());
    }
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

//new method allows commands to work in function files
TClasslessInstanceHook(void, "?load@FunctionManager@@QEAAXAEAVResourcePackManager@@AEAVCommandRegistry@@@Z",
  class ResourcePackManager &rpManager, class CommandRegistry &registry) {
    ImmobileCommand::setup(&registry);
    original(this, rpManager, registry);
}

void dllenter() {}
void dllexit() {}

//by default, immobile flag clears when a player leaves the world
//hack: resend flag if immobile player leaves and rejoins
void PreInit() {
  Mod::PlayerDatabase::GetInstance().AddListener(SIG("joined"), [](Mod::PlayerEntry const &entry) {
    if (shouldResendImmobilityStatus.count(entry.player->getEntityName())) {
      ImmobileCommand ic;
      ic.setImmobilityStatus(entry.player, true);
    }
  });
}
void PostInit() {}