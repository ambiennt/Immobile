#include "main.h"
#include <dllentry.h>

std::unordered_set<int64_t> shouldResendImmobilityStatus;

class ImmobileCommand : public Command {
public:

	ImmobileCommand() { selector.setIncludeDeadPlayers(true); }

	// initialize command arguments
	CommandSelector<Player> selector;
	bool toggle = true;

	void setImmobilityStatus(Player *player, CommandOutput &output, bool sendCommandFeedback) {
		player->setStatusFlag(ActorFlags::Immobile, toggle);
		std::string immobilityStatusString = toggle ? "You have been immobilized" : "You have been mobilized";

		if (sendCommandFeedback) {
			auto output = TextPacket::createTextPacket<TextPacketType::SystemMessage>(immobilityStatusString);
			player->sendNetworkPacket(output);
		}

		auto &db = Mod::PlayerDatabase::GetInstance();
		auto xuid = db.Find(player)->xuid; // map immobility status to player xuid

		bool isImmobileInMap = shouldResendImmobilityStatus.count(xuid);
		if (toggle && !isImmobileInMap) {
			shouldResendImmobilityStatus.insert(xuid);
		}
		else if (isImmobileInMap) {
			shouldResendImmobilityStatus.erase(xuid);
		}
	}

	void execute(CommandOrigin const &origin, CommandOutput &output) {

		//get selected entity/entities
		auto selectedEntities = selector.results(origin);
		if (selectedEntities.empty()) {
			return output.error("No targets matched selector");
		}

		auto gr = CallServerClassMethod<GameRules*>("?getGameRules@Level@@QEAAAEAVGameRules@@XZ", LocateService<Level>());
		GameRuleIds sendCommandFeedbackId = GameRuleIds::SendCommandFeedback;
		bool sendCommandFeedback = CallServerClassMethod<bool>("?getBool@GameRules@@QEBA_NUGameRuleId@@@Z", gr, &sendCommandFeedbackId);

		for (auto player : selectedEntities) {
			setImmobilityStatus(player, output, sendCommandFeedback);
		}
		
		if (toggle) {
			output.success(
				"Immobilized " + std::to_string(selectedEntities.count()) + std::string(selectedEntities.count() == 1 ? " player" : " players"));
		}
		else {
			output.success(
				"Mobilized " + std::to_string(selectedEntities.count()) + std::string(selectedEntities.count() == 1 ? " player" : " players"));
		}
	}

	static void setup(CommandRegistry *registry) {
		using namespace commands;
		registry->registerCommand(
			"immobile", "Locks player movement.", CommandPermissionLevel::GameMasters, CommandFlagNone, CommandFlagNone);
		registry->registerOverload<ImmobileCommand>("immobile",
			optional(&ImmobileCommand::selector, "target"),
			optional(&ImmobileCommand::toggle, "toggle")
		);
	}
};

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
		if (shouldResendImmobilityStatus.count(entry.xuid)) {
			entry.player->setStatusFlag(ActorFlags::Immobile, true);
		}
	});
}
void PostInit() {}