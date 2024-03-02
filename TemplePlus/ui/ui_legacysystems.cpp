
#include "stdafx.h"

#include <infrastructure/exception.h>
#include <temple/dll.h>
#include "ui_legacysystems.h"
#include "tig/tig_texture.h"
#include "tig/tig_msg.h"
#include "party.h"
#include "ui_systems.h"
#include "dungeon_master.h"
#include "ui_dm.h"
#include "fade.h"
#include "gamesystems/gamesystems.h"
#include "gamesystems/mapsystem.h"
#include "legacyscriptsystem.h"
#include "tutorial.h"
#include "gamesystems/random_encounter.h"
#include "gamesystems/objects/objsystem.h"
#include "damage.h"
#include "critter.h"
#include "combat.h"
#include "ui_char.h"
#include "hotkeys.h"
#include "tig/tig_loadingscreen.h"
#include "tig/tig_console.h"
#include "gamesystems/d20/d20_help.h"
#include <messages\messagequeue.h>
#include "ui_dialog.h"

//*****************************************************************************
//* MainMenu-UI
//*****************************************************************************

UiMainMenu::UiMainMenu(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10112810);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system MainMenu-UI");
    }
}
UiMainMenu::~UiMainMenu() {
    auto shutdown = temple::GetPointer<void()>(0x101128e0);
    shutdown();
}
const std::string &UiMainMenu::GetName() const {
    static std::string name("MainMenu-UI");
    return name;
}

//*****************************************************************************
//* LoadGame
//*****************************************************************************

UiLoadGame::UiLoadGame(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10177ed0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system LoadGame");
    }
}
UiLoadGame::~UiLoadGame() {
    auto shutdown = temple::GetPointer<void()>(0x101772b0);
    shutdown();
}
void UiLoadGame::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10178130);
    resize(&resizeArg);
}
const std::string &UiLoadGame::GetName() const {
    static std::string name("LoadGame");
    return name;
}

void UiLoadGame::Show(bool fromMainMenu)
{
	static auto ui_loadgame_show = temple::GetPointer<void(int fromMainMenu)>(0x101772e0);
	ui_loadgame_show(fromMainMenu ? TRUE : FALSE);
}

void UiLoadGame::Hide()
{
	static auto ui_loadgame_hide = temple::GetPointer<int()>(0x101773a0);
	ui_loadgame_hide();
}

void UiLoadGame::Hide2()
{
	static auto ui_loadgame_hide2 = temple::GetPointer<int()>(0x101774d0);
	ui_loadgame_hide2();
}

static class UiLoadGameHooks: public TempleFix {
public:
	void apply() override {
		static LgcyWidgetHandleMsgFn orgLoadgameWndMsg;

		// Hook to allow scrollwheel to affect loadgame UI
		orgLoadgameWndMsg = replaceFunction<BOOL(LgcyWidgetId, TigMsg*)>(0x101775C0, [](LgcyWidgetId widId, TigMsg* msg) {
			auto result = orgLoadgameWndMsg(widId, msg);
			if (msg->type == TigMsgType::MOUSE) {
				auto msgMouse = (TigMsgMouse*)msg;
				if (msgMouse->buttonStateFlags & MouseStateFlags::MSF_SCROLLWHEEL_CHANGE) {
					auto scrollbarId = temple::GetRef<LgcyWidgetId>(0x10C07C90);
					auto scrollBar = uiManager->GetScrollBar(scrollbarId);
					if (scrollBar) {
						scrollBar->HandleMessage(*msg);
					}
				}
			}
			return result;
			});
	}
} uiLoadgameHooks;

//*****************************************************************************
//* SaveGame
//*****************************************************************************

UiSaveGame::UiSaveGame(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101767f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system SaveGame");
    }
}
UiSaveGame::~UiSaveGame() {
    auto shutdown = temple::GetPointer<void()>(0x10175950);
    shutdown();
}
void UiSaveGame::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10176ae0);
    resize(&resizeArg);
}
const std::string &UiSaveGame::GetName() const {
    static std::string name("SaveGame");
    return name;
}

void UiSaveGame::Show(bool fromMainMenu)
{
	static auto ui_savegame_show = temple::GetPointer<void(int)>(0x10175980);
	ui_savegame_show(fromMainMenu ? TRUE : FALSE);
}

void UiSaveGame::Hide()
{
	static auto ui_savegame_hide = temple::GetPointer<int()>(0x10175a40);
	ui_savegame_hide();
}

void UiSaveGame::Hide2()
{
	static auto ui_savegame_hide2 = temple::GetPointer<int()>(0x10175ae0);
	ui_savegame_hide2();
}

static class UiSaveGameHooks : public TempleFix {
public:
	void apply() override {
		static LgcyWidgetHandleMsgFn orgSavegameWndMsg;

		// Hook to allow scrollwheel to affect loadgame UI
		orgSavegameWndMsg = replaceFunction<BOOL(LgcyWidgetId, TigMsg*)>(0x10175E80, [](LgcyWidgetId widId, TigMsg* msg) {
			auto result = orgSavegameWndMsg(widId, msg);
			if (msg->type == TigMsgType::MOUSE) {
				auto msgMouse = (TigMsgMouse*)msg;
				if (msgMouse->buttonStateFlags & MouseStateFlags::MSF_SCROLLWHEEL_CHANGE) {
					auto scrollbarId = temple::GetRef<LgcyWidgetId>(0x10C07348);
					auto scrollBar = uiManager->GetScrollBar(scrollbarId);
					if (scrollBar) {
						scrollBar->HandleMessage(*msg);
					}
				}
			}
			return result;
			});
	}
} uiSavegameHooks;

//*****************************************************************************
//* IntgameSelect
//*****************************************************************************

UiInGameSelect::UiInGameSelect(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10138a40);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system IntgameSelect");
    }
}
UiInGameSelect::~UiInGameSelect() {
    auto shutdown = temple::GetPointer<void()>(0x10137560);
    shutdown();
}
void UiInGameSelect::Reset() {
    auto reset = temple::GetPointer<void()>(0x10137640);
    reset();
}
const std::string &UiInGameSelect::GetName() const {
    static std::string name("IntgameSelect");
    return name;
}

//*****************************************************************************
//* RadialMenu
//*****************************************************************************

UiRadialMenu::UiRadialMenu(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1013d230);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system RadialMenu");
    }
}
UiRadialMenu::~UiRadialMenu() {
    auto shutdown = temple::GetPointer<void()>(0x1013c1f0);
    shutdown();
}
void UiRadialMenu::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10139d60);
    resize(&resizeArg);
}
const std::string &UiRadialMenu::GetName() const {
    static std::string name("RadialMenu");
    return name;
}

//*****************************************************************************
//* Anim-UI
//*****************************************************************************

UiAnim::UiAnim(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101739f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Anim-UI");
    }
}
void UiAnim::Reset() {
    auto reset = temple::GetPointer<void()>(0x10173780);
    reset();
}
bool UiAnim::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x101737b0);
        return load(&save) == 1;
}
const std::string &UiAnim::GetName() const {
    static std::string name("Anim-UI");
    return name;
}

//*****************************************************************************
//* TB-UI
//*****************************************************************************

UiTB::UiTB(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1014e1f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system TB-UI");
    }
}
UiTB::~UiTB() {
    auto shutdown = temple::GetPointer<void()>(0x1014de70);
    shutdown();
}
const std::string &UiTB::GetName() const {
    static std::string name("TB-UI");
    return name;
}

//*****************************************************************************
//* WMap-Rnd
//*****************************************************************************

void UiWMapRnd::LoadModule() {
    auto loadModule = temple::GetPointer<int()>(0x10121300);
    if (!loadModule()) {
        throw TempleException("Unable to load module data for game system WMap-Rnd");
    }
}
void UiWMapRnd::UnloadModule() {
    auto unloadModule = temple::GetPointer<void()>(0x10121180);
    unloadModule();
}
bool UiWMapRnd::SaveGame(TioFile *file) {
        auto save = temple::GetPointer<int(TioFile*)>(0x10120bf0);
        return save(file) == 1;
}
bool UiWMapRnd::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x10120d40);
        return load(&save) == 1;
}
const std::string &UiWMapRnd::GetName() const {
    static std::string name("WMap-Rnd");
    return name;
}

void UiWMapRnd::StartRandomEncounterTimer()
{
	static auto ui_wmaprnd_start_timer = temple::GetPointer<void()>(0x101210d0);
	ui_wmaprnd_start_timer();
}

//*****************************************************************************
//* Combat-UI
//*****************************************************************************

UiCombat::UiCombat(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10173690);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Combat-UI");
    }
}
UiCombat::~UiCombat() {
    auto shutdown = temple::GetPointer<void()>(0x10172df0);
    shutdown();
}
void UiCombat::Reset() {
    auto reset = temple::GetPointer<void()>(0x10172e70);
    reset();
}
const std::string &UiCombat::GetName() const {
    static std::string name("Combat-UI");
    return name;
}

void UiCombat::Update()
{
	static auto ui_combat_update = temple::GetPointer<void()>(0x10142740);
	ui_combat_update();
}

//*****************************************************************************
//* Slide-UI
//*****************************************************************************

void UiSlide::LoadModule() {
    auto loadModule = temple::GetPointer<int()>(0x10138bb0);
    if (!loadModule()) {
        throw TempleException("Unable to load module data for game system Slide-UI");
    }
}
void UiSlide::UnloadModule() {
    auto unloadModule = temple::GetPointer<void()>(0x10138bd0);
    unloadModule();
}
const std::string &UiSlide::GetName() const {
    static std::string name("Slide-UI");
    return name;
}



//*****************************************************************************
//* ToolTip-UI
//*****************************************************************************

UiToolTip::UiToolTip(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10124380);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system ToolTip-UI");
    }
}
UiToolTip::~UiToolTip() {
    auto shutdown = temple::GetPointer<void()>(0x10122d00);
    shutdown();
}
const std::string &UiToolTip::GetName() const {
    static std::string name("ToolTip-UI");
    return name;
}

//*****************************************************************************
//* Logbook-UI
//*****************************************************************************

UiLogbook::UiLogbook(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101281a0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Logbook-UI");
    }
}
UiLogbook::~UiLogbook() {
    auto shutdown = temple::GetPointer<void()>(0x10128270);
    shutdown();
}
void UiLogbook::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10125ce0);
    resize(&resizeArg);
}
void UiLogbook::Reset() {
    auto reset = temple::GetPointer<void()>(0x10125dc0);
    reset();
}
bool UiLogbook::SaveGame(TioFile *file) {
        auto save = temple::GetPointer<int(TioFile*)>(0x10125de0);
        return save(file) == 1;
}
bool UiLogbook::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x10125e40);
        return load(&save) == 1;
}
const std::string &UiLogbook::GetName() const {
    static std::string name("Logbook-UI");
    return name;
}

//*****************************************************************************
//* Scrollpane-UI
//*****************************************************************************

UiScrollpane::UiScrollpane(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10172330);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Scrollpane-UI");
    }
}
UiScrollpane::~UiScrollpane() {
    auto shutdown = temple::GetPointer<void()>(0x10171ea0);
    shutdown();
}
const std::string &UiScrollpane::GetName() const {
    static std::string name("Scrollpane-UI");
    return name;
}


//*****************************************************************************
//* TextDialog-UI
//*****************************************************************************

UiTextDialog::UiTextDialog(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1014eec0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system TextDialog-UI");
    }
}
UiTextDialog::~UiTextDialog() {
    auto shutdown = temple::GetPointer<void()>(0x1014e640);
    shutdown();
}
const std::string &UiTextDialog::GetName() const {
    static std::string name("TextDialog-UI");
    return name;
}

//*****************************************************************************
//* FocusManager-UI
//*****************************************************************************

UiFocusManager::UiFocusManager(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101709a0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system FocusManager-UI");
    }
}
const std::string &UiFocusManager::GetName() const {
    static std::string name("FocusManager-UI");
    return name;
}

//*****************************************************************************
//* RandomEncounter-UI
//*****************************************************************************

UiRandomEncounter::UiRandomEncounter(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101708f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system RandomEncounter-UI");
    }
}
UiRandomEncounter::~UiRandomEncounter() {
    auto shutdown = temple::GetPointer<void()>(0x1016fcc0);
    shutdown();
}
void UiRandomEncounter::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x1016cec0);
    resize(&resizeArg);
}
const std::string &UiRandomEncounter::GetName() const {
    static std::string name("RandomEncounter-UI");
    return name;
}

bool UiRandomEncounter::HasPermissionToExit(){
	static auto hasPermissionToExit = temple::GetRef<BOOL(__cdecl)()>(0x1016CEB0);
	return (hasPermissionToExit() != FALSE) ? true : false;
}

void UiRandomEncounter::ShowExitWnd(){
	struct UiRndEncExitWidgets
	{
		LgcyWindow *wnd;
		LgcyButton *okBtn;
		LgcyButton *cancelBtn;
		LgcyButton *cancelBtn2;
	};
	auto exitWidgets = temple::GetRef<UiRndEncExitWidgets*>(0x10BF3550);
	auto wnd = exitWidgets->wnd;
	if (wnd){
		uiManager->SetHidden(wnd->widgetId, false);
	}
	temple::GetRef<int>(0x10BF3788) = 1;
}

//*****************************************************************************
//* Help-UI
//*****************************************************************************

UiHelp::UiHelp(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101317f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Help-UI");
    }
}
UiHelp::~UiHelp() {
    auto shutdown = temple::GetPointer<void()>(0x10130f30);
    shutdown();
}
void UiHelp::Reset() {
    auto reset = temple::GetPointer<void()>(0x10130f00);
    reset();
}
const std::string &UiHelp::GetName() const {
    static std::string name("Help-UI");
    return name;
}

bool UiHelp::AlertIsActive()
{
	return temple::GetRef<BOOL>(0x10C4D1D8) != 0;
}

//*****************************************************************************
//* SkillMastery-UI
//*****************************************************************************

UiSkillMastery::UiSkillMastery(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1016ce20);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system SkillMastery-UI");
    }
}
UiSkillMastery::~UiSkillMastery() {
    auto shutdown = temple::GetPointer<void()>(0x1016c270);
    shutdown();
}
void UiSkillMastery::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x1016a110);
    resize(&resizeArg);
}
const std::string &UiSkillMastery::GetName() const {
    static std::string name("SkillMastery-UI");
    return name;
}

//*****************************************************************************
//* UtilityBar-UI
//*****************************************************************************

UiUtilityBar::UiUtilityBar(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10110fe0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system UtilityBar-UI");
    }
}
UiUtilityBar::~UiUtilityBar() {
    auto shutdown = temple::GetPointer<void()>(0x10110520);
    shutdown();
}
void UiUtilityBar::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10111050);
    resize(&resizeArg);
}
void UiUtilityBar::Reset() {
    auto reset = temple::GetPointer<void()>(0x1010ee70);
    reset();
}
const std::string &UiUtilityBar::GetName() const {
    static std::string name("UtilityBar-UI");
    return name;
}

void UiUtilityBar::HideOpenedWindows(bool hideOptions)
{
	static auto hideWindows = temple::GetPointer<BOOL(BOOL)>(0x101156B0);
	hideWindows(hideOptions ? TRUE : FALSE);
}

void UiUtilityBar::Hide()
{
	static auto hide = temple::GetPointer<BOOL()>(0x1010EEC0);
	hide();
}

void UiUtilityBar::Show()
{
	static auto ui_utility_bar_show = temple::GetPointer<void()>(0x1010ee80);
	ui_utility_bar_show();
}

bool UiUtilityBar::IsRollHistoryVisible(){
	return temple::GetRef<int>(0x10BDDE5C) != 0;
}

bool UiUtilityBar::IsVisible(){
	return temple::GetRef<int>(0x10BD33F8) != 0;
}

/* 0x10121B20 */
void UiUtilityBar::DialogBtnHide()
{
	temple::GetRef<void(__cdecl)()>(0x10121B20)();
}

void UiUtilityBar::DialogHistoryBtnToggle()
{
	temple::GetRef<void(__cdecl)()>(0x10121AC0)();
}

//*****************************************************************************
//*DM-UI
//*****************************************************************************


UiDM::UiDM(const UiSystemConf& config){
	uiDm.DmWidgetsInit(config);
}

UiDM::~UiDM()
{
}

void UiDM::Reset()
{
}

void UiDM::ResizeViewport(const UiResizeArgs& resizeArgs)
{
}

const std::string& UiDM::GetName() const{
	static std::string name(Name);
	return name;
}

bool UiDM::IsVisible(){
	return dmSys.IsActive();
}

void UiDM::Show(){
	if (config.dungeonMaster)
		dmSys.Show();
}

void UiDM::Hide(){
	dmSys.Hide();
}

void UiDM::Toggle(){
	dmSys.Toggle();
}

void UiDM::HideButton() {
	uiDm.SetButtonVis(false);
}

void UiDM::ShowButton() {
	uiDm.SetButtonVis(true);
}

//*****************************************************************************
//* Track-UI
//*****************************************************************************

UiTrack::UiTrack(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10169d80);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Track-UI");
    }
}
UiTrack::~UiTrack() {
    auto shutdown = temple::GetPointer<void()>(0x10169e10);
    shutdown();
}
void UiTrack::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x101679e0);
    resize(&resizeArg);
}
const std::string &UiTrack::GetName() const {
    static std::string name("Track-UI");
    return name;
}

//*****************************************************************************
//* party_pool
//*****************************************************************************

UiPartyPool::UiPartyPool(const UiSystemConf &config) {

	// Fixes a bug in ui_partypool_init that previously only didn't cause issues because
	// ui_mm did create widget #0, and that widget was always hidden
	temple::GetRef<LgcyWidgetId>(0x10BF1BA4) = -1;

    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101675f0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system party_pool");
    }
}
UiPartyPool::~UiPartyPool() {
    auto shutdown = temple::GetPointer<void()>(0x10165c80);
    shutdown();
}
void UiPartyPool::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x101679a0);
    resize(&resizeArg);
}
void UiPartyPool::Reset() {
    auto reset = temple::GetPointer<void()>(0x10165cd0);
    reset();
}
bool UiPartyPool::SaveGame(TioFile *file) {
        auto save = temple::GetPointer<int(TioFile*)>(0x10165d10);
        return save(file) == 1;
}
bool UiPartyPool::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x10165da0);
        return load(&save) == 1;
}
const std::string &UiPartyPool::GetName() const {
    static std::string name("party_pool");
    return name;
}

void UiPartyPool::Show(bool ingame)
{
	static auto ui_partypool_show = temple::GetPointer<void(int ingame)>(0x10165e60);
	ui_partypool_show(ingame ? TRUE : FALSE);
}

void UiPartyPool::Refresh()
{
	static auto ui_party_refresh = temple::GetPointer<void()>(0x10134cb0);
	ui_party_refresh();
}

void UiPartyPool::SetAddRemoveBtnState(LgcyButtonState state)
{
	uiManager->SetButtonState(addRemoveBtnState, state);
}

static class UiPartyPoolFix : public TempleFix {
public:
	void apply() override {
		static LgcyWidgetHandleMsgFn orgSelectPoolCharMsg;
		orgSelectPoolCharMsg = replaceFunction<BOOL(LgcyWidgetId, TigMsg*)>(0x10165620, [](LgcyWidgetId widId, TigMsg* msg){
			auto result = orgSelectPoolCharMsg(widId, msg);
			if (result){
				if (static_cast<int>(party.GroupPCsLen()) >= uiSystems->GetPccPortrait().pcPortraitSlotCount){
					uiSystems->GetPartyPool().SetAddRemoveBtnState(LgcyButtonState::Disabled);
				}
			}
			return result;
		});
	}
} uiPartyPoolFix;

//*****************************************************************************
//* pcc_portrait
//*****************************************************************************

static class UiPccPortraitFix : public TempleFix {
public:
	void apply() override {

		// UiPcCreationRemovePc
		replaceFunction<void()>(0x10163150, []() {
			auto N = party.GroupPCsLen();
			auto &pccIdx = *uiSystems->GetPccPortrait().pcCreationIdx;
			if (pccIdx >= 0 && pccIdx < static_cast<int>(N)){
				auto pc = party.GroupPCsGetMemberN(pccIdx);
				party.ObjRemoveFromAllGroupArrays(pc);
				// set button state...
				uiManager->SetButtonState(uiSystems->GetPccPortrait().pcPortraitWidgIds[N - 1], LgcyButtonState::Disabled);
				objects.Destroy(pc);
			}
			pccIdx = -1;
		});

		// ui_msg_pc_creation_portraits
		static LgcyWidgetHandleMsgFn orgPcPortraitsMsgFunc;
		orgPcPortraitsMsgFunc = replaceFunction<BOOL(LgcyWidgetId, TigMsg*)>(0x101633a0, [](LgcyWidgetId widgetId, TigMsg *msg) {

			if (!uiSystems->GetPccPortrait().HandleMessage(widgetId, msg)) {
				return orgPcPortraitsMsgFunc(widgetId, msg);
			}
			else {
				return FALSE;
			}

		});

		// UiPcCreationPortraitsMainHide
		replaceFunction<int()>(0x10163030, []() {
			uiSystems->GetPccPortrait().Hide();
			return FALSE;
		});
		// PcPortraitsDeactivate
		replaceFunction<void()>(0x10163060, []() {
			return uiSystems->GetPccPortrait().Disable();
		});
		// PcPortraitsButtonActivateNext
		replaceFunction<void()>(0x10163090, []() {
			return uiSystems->GetPccPortrait().ButtonActivateNext();
		});
		// PcPortraitsRefresh
		replaceFunction<void(void)>(0x10163440, [](void) {
			return uiSystems->GetPccPortrait().Refresh();
		});

	}

	void WriteMaxPCPortraitValues(UiPccPortrait &system) {
		// 1016312F
		int writeVal = (int)&system.pcPortraitWidgIds[-1];
		write(0x1016312F + 3, &writeVal, 4);

		// ui_msg_pc_creation_portraits
		writeVal = system.pcPortraitSlotCount;
		write(0x101633B5 + 1, &writeVal, 1);
		writeVal = (int)&system.pcPortraitWidgIds;
		write(0x101633B7 + 1, &writeVal, 4);

		// ui_render_pc_creation_portraits
		writeVal = system.pcPortraitSlotCount;
		write(0x10163279 + 1, &writeVal, 1);
		writeVal = (int)&system.pcPortraitWidgIds;
		write(0x1016327B + 1, &writeVal, 4);

		writeVal = (int)&system.pcPortraitsMainId;
		write(0x101632E6 + 2, &writeVal, 4);
		write(0x1016337A + 2, &writeVal, 4);
		writeVal = (int)&system.pcPortraitBoxRects;
		write(0x101632F6 + 2, &writeVal, 4);
		writeVal = (int)&system.pcPortraitRects;
		write(0x10163329 + 2, &writeVal, 4);
	}
} uiPccPortraitFix;

UiPccPortrait::UiPccPortrait(const UiSystemConf &conf) {

	if (textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait.tga", uiPccPortraitTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_click.tga", uiPccPortraitClickTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_hover.tga", uiPccPortraitHoverTexture)
		|| textureFuncs.RegisterTexture("art\\interface\\pc_creation\\portrait_disabled.tga", uiPccPortraitDisabledTexture)
		)
	{
		throw TempleException("Unable to open PCCPortrait textures");
	}
	pcPortraitSlotCount = MAX_PC_CREATION_PORTRAITS;
	if (!config.maxPCsFlexible && config.maxPCs <= MAX_PC_CREATION_PORTRAITS && config.maxPCs > 0){
		pcPortraitSlotCount = config.maxPCs;
	}
	InitWidgets(conf.height);

	uiPccPortraitFix.WriteMaxPCPortraitValues(*this);

}
UiPccPortrait::~UiPccPortrait() {
	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		uiManager->RemoveChildWidget(pcPortraitWidgIds[i]);
	}
	uiManager->RemoveWidget(pcPortraitsMainId);
}
void UiPccPortrait::ResizeViewport(const UiResizeArgs& resizeArg) {
	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		uiManager->RemoveChildWidget(pcPortraitWidgIds[i]);
	}
	uiManager->RemoveWidget(pcPortraitsMainId);
	return InitWidgets(resizeArg.rect1.height);
}
const std::string &UiPccPortrait::GetName() const {
    static std::string name("pcc_portrait");
    return name;
}

void UiPccPortrait::Hide()
{
	uiManager->SetHidden(pcPortraitsMainId, true);
}

void UiPccPortrait::ButtonActivateNext()
{
	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		auto state = uiManager->GetButtonState(pcPortraitWidgIds[i]);
		if (state == LgcyButtonState::Disabled)
		{
			uiManager->SetButtonState(pcPortraitWidgIds[i], LgcyButtonState::Normal);
			if (i == pcPortraitSlotCount - 1)
				*uiPcPortraitsFullMaybe = 1;
			return;
		}
	}
	*uiPcPortraitsFullMaybe = 1;
}

void UiPccPortrait::Refresh()
{
	uiManager->SetHidden(pcPortraitsMainId, false);
	uiManager->BringToFront(pcPortraitsMainId);
	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		uiManager->SetButtonState(pcPortraitWidgIds[i], LgcyButtonState::Disabled);
	}
	*uiPcPortraitsFullMaybe = 0;

	if (party.GroupPCsLen())
	{
		for (int i = 0; i < pcPortraitSlotCount; i++)
		{
			uiManager->SetButtonState(pcPortraitWidgIds[i], LgcyButtonState::Normal);
		}
	}
	*pcCreationIdx = -1;
}

void UiPccPortrait::Disable()
{
	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		uiManager->SetButtonState(pcPortraitWidgIds[i], LgcyButtonState::Disabled);
	}
	*uiPcPortraitsFullMaybe = 0;
}

bool UiPccPortrait::HandleMessage(LgcyWidgetId widgetId, TigMsg * tigMsg)
{

	if (tigMsg->type != TigMsgType::WIDGET
		|| tigMsg->arg2 != 1
		|| (WidgetIdIndexOf(widgetId, pcPortraitWidgIds, pcPortraitSlotCount) == -1))
		return true;
	return false;

}

void UiPccPortrait::InitWidgets(int height)
{
	LgcyWindow pcPortraitsMain(10, height - 80, 650, 63);
	pcPortraitsMain.flags = 1;
	pcPortraitsMain.render = [](int widId) {};
	pcPortraitsMain.handleMessage = [](int widId, TigMsg* msg)->BOOL { return FALSE; };

	pcPortraitsMainId = uiManager->AddWindow(pcPortraitsMain);

	static auto ui_render_pc_creation_portraits = temple::GetPointer<void(LgcyWidgetId widgetId)>(0x10163270);
	static auto ui_msg_pc_creation_portraits = temple::GetPointer<BOOL(LgcyWidgetId widgetId, TigMsg *msg)>(0x101633a0);

	for (int i = 0; i < pcPortraitSlotCount; i++)
	{
		LgcyButton button;
		uiManager->ButtonInit(&button, 0, pcPortraitsMainId, pcPortraitsMain.x + 81 * i, pcPortraitsMain.y, 76, 63);

		pcPortraitBoxRects[i].x = button.x;
		pcPortraitRects[i].x = button.x + 4;

		pcPortraitBoxRects[i].y = button.y;
		pcPortraitRects[i].y = button.y + 4;


		pcPortraitBoxRects[i].width = button.width;
		pcPortraitBoxRects[i].height = button.height;

		pcPortraitRects[i].width = 51;
		pcPortraitRects[i].height = 45;

		button.render = ui_render_pc_creation_portraits;
		button.handleMessage = ui_msg_pc_creation_portraits;

		pcPortraitWidgIds[i] = uiManager->AddButton(button, pcPortraitsMainId);
		uiManager->SetButtonState(pcPortraitWidgIds[i], LgcyButtonState::Disabled);
	}
}


//*****************************************************************************
//* Party-UI
//*****************************************************************************

UiParty::UiParty(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10134bf0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Party-UI");
    }
}
UiParty::~UiParty() {
    auto shutdown = temple::GetPointer<void()>(0x10132760);
    shutdown();
}
void UiParty::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x101350b0);
    resize(&resizeArg);
}
void UiParty::Reset() {
    auto reset = temple::GetPointer<void()>(0x101350a0);
    reset();
}
const std::string &UiParty::GetName() const {
    static std::string name("Party-UI");
    return name;
}

void UiParty::Update()
{
	static auto ui_party_refresh = temple::GetPointer<int()>(0x10134cb0);
	ui_party_refresh();
}

void UiParty::UpdateAndShowMaybe()
{
	static auto ui_party_refresh2 = temple::GetPointer<int()>(0x10135000);
	ui_party_refresh2();
}

//*****************************************************************************
//* Formation-UI
//*****************************************************************************

UiFormation::UiFormation(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10125b90);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Formation-UI");
    }
}
UiFormation::~UiFormation() {
    auto shutdown = temple::GetPointer<void()>(0x10124d70);
    shutdown();
}
void UiFormation::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10125cb0);
    resize(&resizeArg);
}
const std::string &UiFormation::GetName() const {
    static std::string name("Formation-UI");
    return name;
}

//*****************************************************************************
//* Camping-UI
//*****************************************************************************

UiCamping::UiCamping(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1012fb60);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Camping-UI");
    }
}
UiCamping::~UiCamping() {
    auto shutdown = temple::GetPointer<void()>(0x1012edd0);
    shutdown();
}
void UiCamping::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x101302e0);
    resize(&resizeArg);
}
void UiCamping::Reset() {
    auto reset = temple::GetPointer<void()>(0x1012e310);
    reset();
}
bool UiCamping::SaveGame(TioFile *file) {
        auto save = temple::GetPointer<int(TioFile*)>(0x1012e330);
        return save(file) == 1;
}
bool UiCamping::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x1012e3b0);
        return load(&save) == 1;
}
const std::string &UiCamping::GetName() const {
    static std::string name("Camping-UI");
    return name;
}

BOOL UiCamping::Camp(int hoursToRest){

	auto restedCount = 0;
	auto canHeal = true;
	auto completedSuccessfully = TRUE;
	const unsigned int HOUR_IN_MS = 3600u * 1000u;
	const unsigned int HOUR_IN_ROUNDS = 600u;
	

	if (hoursToRest > 0) {

		spellSys.JammedSpellsCreateRef();

		auto sleepStatus = GetSleepStatus();
		auto healerMod = GetHealingAmountMod();
		

		while (restedCount < hoursToRest){
			
			auto addTime = temple::GetRef<void(__cdecl)(unsigned int)>(0x10062390);
			addTime(HOUR_IN_MS);
			RandomEncounterSetup randEncSetup;
			RandomEncounter *randEncOut;
			if (GetSleepStatus() == 1){
				randEncSetup.flags = 1; // sleep encounter
				randEncSetup.location.locx = 0;
				randEncSetup.location.locy = 0;

				auto encounterExists = temple::GetRef < BOOL(__cdecl)(RandomEncounterSetup*, RandomEncounter**)>(0x100461E0);
				if (encounterExists(&randEncSetup, &randEncOut)){
					completedSuccessfully = FALSE;
					auto createEncounter = temple::GetRef<void(__cdecl)(RandomEncounter*)>(0x100462F0);
					createEncounter(randEncOut);
					break;
				}
			}

			restedCount++;
		}

		// apply healing
		if (restedCount >= 8){
			if (GetSleepStatus() == 3){
				canHeal = false;
			}

			auto restPeriods = (restedCount - 8) / 24 + 1;
			auto restPeriodsOrg = restPeriods;
			if (gameSystems->GetMap().IsCurrentMapBedrest()){
				restPeriods *= 2;
			}

			for (auto partyIdx = 0u; partyIdx < party.GroupListGetLen(); partyIdx++){
				auto partyMember = party.GroupListGetMemberN(partyIdx);
				
				auto healAmt = GetHealingAmount(partyMember, restPeriods);
				if (restedCount >= 24) {
					healAmt += healerMod * restPeriodsOrg;
				}

				// heal damage
				if (canHeal){
					spellSys.SanitizeSpellSlots(partyMember);
					spellSys.SpellsPendingToMemorized(partyMember);
					objSystem->GetObject(partyMember)->ClearArray(obj_f_critter_spells_cast_idx);
					damage.Heal(partyMember, partyMember, Dice(0, 0, healAmt), D20A_NONE);
				}

				// heal subdual
				damage.HealSubdual(partyMember, healAmt);
				
				// dispatch NewDayRest event(s)
				if (canHeal){
					auto dispatcher = objSystem->GetObject(partyMember)->GetDispatcher();
					if (dispatcher->IsValid()){
						dispatcher->Process(enum_disp_type::dispTypeNewDay, DK_NEWDAY_REST, nullptr);
						for (auto iRestPeriod = 1; iRestPeriod < restPeriodsOrg; iRestPeriod++) {
							dispatcher->Process(enum_disp_type::dispTypeNewDay, DK_NEWDAY_REST, nullptr);
						}
					}
				}
			}
		}
	
		if (restedCount > 0) {
			spellSys.JammedSpellsPrune(restedCount * HOUR_IN_ROUNDS);
		}
	}


	auto sleepStatusUpdate = temple::GetRef<void(__cdecl)()>(0x10045850);
	sleepStatusUpdate();
	FadeArgs fadeArgs;
	fadeArgs.flags = 1;
	fadeArgs.countSthgUsually48 = 48;
	fadeArgs.transitionTime = 1.0;
	fade.PerformFade(fadeArgs);

	const int TUTORIAL_MAP_ID = 5117;
	if (gameSystems->GetMap().GetCurrentMapId() == TUTORIAL_MAP_ID && !scriptSys.GetGlobalFlag(3)){
		if (!tutorial.IsTutorialActive()){
			tutorial.Toggle();
		}
		tutorial.ShowTopic(20);
		scriptSys.SetGlobalFlag(3, 1);
	}
	return completedSuccessfully;
}

void UiCamping::SetTimeUntilHealed(){

	auto &campingMode = temple::GetRef<int>(0x10BE2B38);
	auto &hoursToRest = temple::GetRef<int>(0x10BE28FC);
	auto &daysToRest  = temple::GetRef<int>(0x10BE2AF0);
	
	campingMode = 1;
	hoursToRest = 0;
	daysToRest = 0;

	auto drawTimeToRest = temple::GetRef<void(__cdecl)()>(0x1012EA20);

	if (GetSleepStatus() == 3){
		drawTimeToRest();
		return;
	}


	auto healMod = GetHealingAmountMod();
	for (auto partyIdx =0u; partyIdx < party.GroupListGetLen(); partyIdx++)
	{
		auto handle = party.GroupListGetMemberN(partyIdx);
		if (!handle) continue;
		auto curHp = objects.GetHPCur(handle);
		auto maxHp = objects.StatLevelGet(handle, stat_hp_max);
		auto healQuantum = GetHealingAmount(handle, 1) + healMod;
		

		auto hours = 24 * ((healQuantum - curHp + maxHp - 1) / healQuantum );
		if (curHp < maxHp && hours < 24){
			hours = 24;
		}
		if (hours > hoursToRest){
			hoursToRest = hours;
		}
	}

	daysToRest = hoursToRest / 24;
	hoursToRest %= 24;
	
	drawTimeToRest();
}

int UiCamping::GetHealingAmount(objHndl handle, int restPeriods){

	auto healAmt = 0;
	
	auto lvl = objects.StatLevelGet(handle, stat_level);
	auto hdCount = objects.GetHitDiceNum(handle);
	auto healQuantum = max(hdCount, lvl);

	auto healerMod = GetHealingAmountMod();

	healAmt = healQuantum * restPeriods;
	

	return healAmt;
}

int UiCamping::GetHealingAmountMod(){

	auto healerMod = 0;
	if (GetSleepStatus()<= 1) { // safe resting place or camping in wilderness

							// find someone who can cast heal spells
		for (auto partyIdx = 0u; partyIdx < party.GroupListGetLen(); partyIdx++) {
			auto partyMember = party.GroupListGetMemberN(partyIdx);

			auto maxClrSpellLvl = spellSys.GetMaxSpellLevel(partyMember, stat_level_cleric);
			auto maxDrdSpellLvl = spellSys.GetMaxSpellLevel(partyMember, stat_level_druid);
			healerMod = max(healerMod, max(maxClrSpellLvl * 5, maxDrdSpellLvl * 4));
		}
	}

	return healerMod;
}

int UiCamping::GetSleepStatus(){
	return temple::GetRef<int(__cdecl)()>(0x10045BB0)();
}

//*****************************************************************************
//* Help-Inventory-UI
//*****************************************************************************

UiHelpInventory::UiHelpInventory(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10162f60);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Help-Inventory-UI");
    }
}
UiHelpInventory::~UiHelpInventory() {
    auto shutdown = temple::GetPointer<void()>(0x10162750);
    shutdown();
}
void UiHelpInventory::Reset() {
    auto reset = temple::GetPointer<void()>(0x10162730);
    reset();
}
const std::string &UiHelpInventory::GetName() const {
    static std::string name("Help-Inventory-UI");
    return name;
}

//*****************************************************************************
//* Party-Quickview-UI
//*****************************************************************************

const std::string &UiPartyQuickview::GetName() const {
    static std::string name("Party-Quickview-UI");
    return name;
}

//*****************************************************************************
//* Options-UI
//*****************************************************************************

UiOptions::UiOptions(const UiSystemConf &config) {
	auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x1011b640);
	if (!startup(&config)) {
		throw TempleException("Unable to initialize game system Options-UI");
	}
}
UiOptions::~UiOptions() {
	auto shutdown = temple::GetPointer<void()>(0x1011b0e0);
	shutdown();
}
void UiOptions::ResizeViewport(const UiResizeArgs& resizeArg) {
	auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10117540);
	resize(&resizeArg);
}
const std::string &UiOptions::GetName() const {
	static std::string name("Options-UI");
	return name;
}
void UiOptions::Show(bool fromMainMenu) {
	static auto ui_options_show = temple::GetPointer<void(int fromMainMenu)>(0x10119d20);
	ui_options_show(fromMainMenu ? TRUE : FALSE);
}

//*****************************************************************************
//* UI-Manager
//*****************************************************************************




#pragma pack(push, 1)
struct LgcyHotKeySpec {
	int field0;
	int field4;
	int modifier;
	int eventCode;
	int field10;
	int primaryKeyCode;
	int primaryOnDown;
	int secondaryKeyCode;
	int secondaryOnDown;
};
#pragma pack(pop)
using DfltHotKeySpecs = LgcyHotKeySpec[62];

enum class HotKeyModifier {
	None,
	Ctrl, // DIK_LCONTROL
	Shift, // DIK_LSHIFT
	Alt // DIK_LMENU
};

struct HotKeySpec {
	bool unkBool;
	bool unkBool2;
	HotKeyModifier modifier;
	InGameHotKey eventCode;
	int primaryKeyCode = 0;
	bool primaryOnDown = false;
	int secondaryKeyCode = 0;
	bool secondaryOnDown = false;

	HotKeySpec(bool unkBool1, bool unkBool2, HotKeyModifier modifier, InGameHotKey eventCode, int keyCode, bool onDown) {}
	HotKeySpec(bool unkBool1, bool unkBool2, HotKeyModifier modifier, InGameHotKey eventCode, int keyCode, bool onDown, int altKeyCode, bool altOnDown) {}
};

static std::vector<HotKeySpec> sDefaultHotKeys{
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection1, 0x2, 0}, // DIK_1
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection2, 0x3, 0}, // DIK_2
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection3, 0x4, 0}, // DIK_3
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection4, 0x5, 0}, // DIK_4
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection5, 0x6, 0}, // DIK_5
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection6, 0x7, 0}, // DIK_6
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection7, 0x8, 0}, // DIK_7
	HotKeySpec{true,  false, HotKeyModifier::Shift, InGameHotKey::TogglePartySelection8, 0x9, 0}, // DIK_8
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup1, 0x3b, 0}, // DIK_F1
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup2, 0x3c, 0}, // DIK_F2
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup3, 0x3d, 0}, // DIK_F3
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup4, 0x3e, 0}, // DIK_F4
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup5, 0x3f, 0}, // DIK_F5
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup6, 0x40, 0}, // DIK_F6
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup7, 0x41, 0}, // DIK_F7
	HotKeySpec{true,  false, HotKeyModifier::Ctrl, InGameHotKey::AssignGroup8, 0x42, 0}, // DIK_F8
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup1, 0x3b, 0}, // DIK_F1
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup2, 0x3c, 0}, // DIK_F2
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup3, 0x3d, 0}, // DIK_F3
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup4, 0x3e, 0}, // DIK_F4
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup5, 0x3f, 0}, // DIK_F5
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup6, 0x40, 0}, // DIK_F6
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup7, 0x41, 0}, // DIK_F7
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::RecallGroup8, 0x42, 0}, // DIK_F8
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::SelectAll, 0x29, 0}, // DIK_GRAVE
	HotKeySpec{false, false, HotKeyModifier::Shift, InGameHotKey::ToggleConsole, 0x29, 0}, // Shift + DIK_GRAVE
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::CenterOnChar, 0xc7, 0}, // DIK_HOME
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar1, 0x2, 0}, // DIK_1
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar2, 0x3, 0}, // DIK_2
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar3, 0x4, 0}, // DIK_3
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar4, 0x5, 0}, // DIK_4
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar5, 0x6, 0}, // DIK_5
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar6, 0x7, 0}, // DIK_6
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar7, 0x8, 0}, // DIK_7
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::SelectChar8, 0x9, 0}, // DIK_8
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ToggleMainMenu, 0x1, 0}, // DIK_ESCAPE
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::QuickLoad, 0x43, 0}, // DIK_F9
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::QuickSave, 0x58, 0, 0x57, 0}, // DIK_F12 or DIK_F11
	HotKeySpec{false, false, HotKeyModifier::Alt, InGameHotKey::Quit, 0x10, 0}, // DIK_Q
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::Screenshot, 0xb7, 0}, // DIK_SYSRQ (print screen)
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollUp, 0xc8, 1}, // DIK_UP
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollDown, 0xd0, 1}, // DIK_DOWN
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollLeft, 0xcb, 1}, // DIK_LEFT
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollRight, 0xcd, 1}, // DIK_RIGHT
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollUpArrow, 0xc8, 1}, // DIK_UP
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollDownArrow, 0xd0, 1}, // DIK_DOWN
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollLeftArrow, 0xcb, 1}, // DIK_LEFT
	HotKeySpec{true,  false, HotKeyModifier::None, InGameHotKey::ScrollRightArrow, 0xcd, 1}, // DIK_RIGHT
	HotKeySpec{true,  false, HotKeyModifier::Alt, InGameHotKey::ObjectHighlight, 0xf, 1}, // alt + DIK_TAB
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowInventory, 0x17, 0}, // DIK_I
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowLogbook, 0x26, 0}, // DIK_L
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowMap, 0x32, 0}, // DIK_M
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowFormation, 0x21, 0}, // DIK_F
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::Rest, 0x13, 0}, // DIK_R
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowHelp, 0x23, 0}, // DIK_H
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ShowOptions, 0x18, 0}, // DIK_O
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::ToggleCombat, 0x2e, 0}, // DIK_C
	HotKeySpec{false, false, HotKeyModifier::None, InGameHotKey::EndTurn, 0x39, 0, 0x1c, 0}, // DIK_SPACE or DIK_RETURN
	HotKeySpec{true,  true,  HotKeyModifier::Shift, InGameHotKey::EndTurnNonParty, 0x1c, 0}, // shift + DIK_RETURN
};

struct KeyHandleStruc {
	int count;
	int events[4];
};

UiKeyManager::UiKeyManager(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10143bd0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system UI-Manager");
    }

	//static auto& hotkeySpecs = temple::GetRef<LgcyHotKeySpec[62]>(0x10BE7020);
	/*for (int i = (int)InGameHotKey::SelectChar1; i <= (int)InGameHotKey::SelectChar8; ++i) {
		hotkeySpecs[i].primaryOnDown = 1; // not such a good idea because it normally only handles key up events (see HandleNonCombat msg)
	}*/
	
}
UiKeyManager::~UiKeyManager() {
    auto shutdown = temple::GetPointer<void()>(0x101431a0);
    shutdown();
}
void UiKeyManager::Reset() {
    auto reset = temple::GetPointer<void()>(0x10143190);
    reset();
}
const std::string &UiKeyManager::GetName() const {
    static std::string name("UI-Manager");
    return name;
}

int UiKeyManager::GetKeyEventModifier()
{
	if (hotkeys.IsKeyPressed(VK_LCONTROL) || hotkeys.IsKeyPressed(VK_RCONTROL)){
		return 29;
	}
	if (hotkeys.IsKeyPressed(VK_LSHIFT) || hotkeys.IsKeyPressed(VK_RSHIFT)) {
		return 42;
	}
	if (hotkeys.IsKeyPressed(VK_LMENU) || hotkeys.IsKeyPressed(VK_RMENU)) {
		return 56;
	}

	return 0;
}
struct KeyConfEvent
{
	int field0;
	int field4;
	int modifier;
	int eventName;
	int field10;
	int handlerId;
	int primary;
	int secondaryId;
	int aecondary;
};
bool UiKeyManager::DontHandle(int evtName){
	if (evtName <= 50){
		return mState == 1;
	}
	if (evtName == 56)
		return false;
	if (evtName == 55){
		return (mState == 1 || mState == 2);
	}
	if (evtName <= 60){
		return mState == 1;
	}
	return false;
}

bool UiKeyManager::HandleKeyEvent(const InGameKeyEvent & msg)
{
	static auto UiManagerKeyEventHandler = temple::GetPointer<BOOL(const InGameKeyEvent &kbMsg)>(0x10143d60);
	static auto &keycfgEvents = temple::GetRef<KeyConfEvent[61]>(0x10BE7020);
	
	auto res = true;
	int modifier = GetKeyEventModifier();
	auto evt = (int)GetKeyEvent(msg, modifier);
	auto evtName = keycfgEvents[evt].eventName;
	
	if (DontHandle(evtName)){
		return false;
	}
	if (msg.msg.arg1 == 0x54){ // stupid hack for Sitra's remote control keyboard
		evtName = 27;
	}
	switch (evtName){
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		return temple::GetRef<int(__cdecl)(const InGameKeyEvent &, int, int)>(0x10143310)(msg, modifier, evtName) != FALSE;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
		return temple::GetRef<int(__cdecl)(const InGameKeyEvent &, int, int)>(0x10143380)(msg, modifier, evtName) != FALSE;
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		return temple::GetRef<int(__cdecl)(const InGameKeyEvent &, int, int)>(0x101433B0)(msg, modifier, evtName) != FALSE;
	case 26:
		return temple::GetRef<int(__cdecl)()>(0x101433E0)() != FALSE; // select all
	case 27:
		tig->GetConsole().Toggle();
		return true;
	case 28: // center screen
		return temple::GetRef<int(__cdecl)()>(0x10143430)() != FALSE;
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
		return CharacterSelect(msg, modifier, evtName);
	case 37:
		return temple::GetRef<int(__cdecl)()>(0x101434E0)() != FALSE; // MainMenu toggle
	case 38:
		return temple::GetRef<int(__cdecl)()>(0x10143530)() != FALSE; // Quickload
	case 39:
		return temple::GetRef<int(__cdecl)()>(0x10143560)() != FALSE; // Quickload
	case 40:
		return temple::GetRef<int(__cdecl)()>(0x10143580)() != FALSE; // Screenshot
	case 51:
		return temple::GetRef<int(__cdecl)()>(0x10143820)() != FALSE; // Inventory
	case 52:
		return temple::GetRef<int(__cdecl)()>(0x101438C0)() != FALSE; // Logbook
	case 53:
		return temple::GetRef<int(__cdecl)()>(0x10143940)() != FALSE; // Map
	case 54:
		return temple::GetRef<int(__cdecl)()>(0x10143A40)() != FALSE; // Formation
	case 55:
		return temple::GetRef<int(__cdecl)()>(0x10143AC0)() != FALSE; // Rest
	case 56:
		if (!mDoYouWantToQuitActive)
			helpSys.ClickForHelpToggle();
		return true;
	case 0:
	case 1:
	case 41:
	case 50:
		return true;
	case 57:
		return temple::GetRef<int(__cdecl)()>(0x10143B40)() != FALSE; // Options
	case 58:
		return CombatToggle();
	default:
		return false;
	case 59:
	case 60:
		return true;
		
	}
	
	// return UiManagerKeyEventHandler(msg) != 0;
}

/* 0x101431F0 */
InGameHotKey UiKeyManager::GetKeyEvent(const InGameKeyEvent& msg, int modifier)
{
	/*auto result = temple::GetRef<int(__cdecl)(const InGameKeyEvent&, int)>(0x101431F0)(msg, modifier);
	return result;*/

	static auto& keyHandlings = temple::GetRef<KeyHandleStruc[255]>(0x10BE7900);
	static auto& hotkeySpecs = temple::GetRef<LgcyHotKeySpec[62]>(0x10BE7020);
	auto tigMsg = (TigKeyStateChangeMsg&)msg.msg;

	// Fix for using num keys for dialog responses causing character select change
	if (uiSystems->GetDlg().IsActive() /*&& mDownInDialog == -1*/ && tigMsg.down) { // note: when dialog is active, it should only get here when it's down anyway...
		mDownInDialog = tigMsg.key;
	}
	
	

	auto count = keyHandlings[tigMsg.key].count;
	
	for (auto i = 0; i < count; ++i) {
		auto evtId = keyHandlings[tigMsg.key].events[i];
		auto& entry = hotkeySpecs[evtId];
		if (entry.primaryKeyCode & tigMsg.key && LOBYTE(entry.primaryOnDown) == LOBYTE(tigMsg.down)
			|| entry.secondaryKeyCode & tigMsg.key && LOBYTE(entry.secondaryOnDown) == LOBYTE(tigMsg.down)) {

			if (entry.modifier == modifier) {

				// Fix for using num keys for dialog responses causing character select change
				if (mDownInDialog == tigMsg.key && !tigMsg.down) {
					mDownInDialog = -1;
					if ((int)entry.eventCode >= (int)InGameHotKey::SelectChar1
						&& (int)entry.eventCode <= (int)InGameHotKey::SelectChar8) {
						return InGameHotKey::None;
					}
				}

				return (InGameHotKey)entry.eventCode;
			}

		}
	}
	return InGameHotKey::None;
}

bool UiKeyManager::CharacterSelect(const InGameKeyEvent& msg, int modifier, int keyEvt){

	if (mDoYouWantToQuitActive)
		return true;

	auto charNumber = (uint32_t)(keyEvt - 29);
	if (charNumber >= party.GroupListGetLen() 
		|| combatSys.isCombatActive())
		return true;

	auto isDlgActive = temple::GetRef<int(__cdecl)()>(0x1014BAC0);
	if (isDlgActive())
		return true;

	auto dude = party.GroupListGetMemberN( charNumber);
	if (party.ObjIsAIFollower(dude))
		return true;

	auto uiCharState = uiSystems->GetChar().GetDisplayType();
	if (uiCharState == UiCharDisplayType::LevelUp)
		return true;


	if (uiSystems->GetChar().IsVisible()){
		uiSystems->GetChar().ShowForCritter(uiCharState, dude);
		return true;
	}

	party.CurrentlySelectedClear();
	party.AddToCurrentlySelected(dude);
	return true;
}

bool UiKeyManager::CombatToggle()
{
	if (mDoYouWantToQuitActive){
		return true;
	}
	if (!combatSys.isCombatActive()){
		auto leader = party.GetConsciousPartyLeader();
		combatSys.enterCombat(leader);
		combatSys.StartCombat(party.GetConsciousPartyLeader(), 1);
		return true;
	}

	if (combatSys.AllCombatantsFarFromParty()){
		combatSys.CritterExitCombatMode(party.GetConsciousPartyLeader());
		return true;
	}
	return false;
}
//static class UiKeyManagerHooks : public TempleFix {
//public:
//	void apply() override {
//		static LgcyWidgetHandleMsgFn orgRefreshKeyConfig;
//		orgRefreshKeyConfig = replaceFunction<BOOL(LgcyWidgetId, TigMsg*)>(0x10143270, [](LgcyWidgetId widId, TigMsg* msg) {
//			auto result = orgRefreshKeyConfig(widId, msg);
//			return result;
//			});
//	}
//} uiKeyManangerHooks;
//*****************************************************************************
//* Help Manager-UI
//*****************************************************************************

UiHelpManager::UiHelpManager(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10124840);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Help Manager-UI");
    }
}
UiHelpManager::~UiHelpManager() {
    auto shutdown = temple::GetPointer<void()>(0x10124870);
    shutdown();
}
void UiHelpManager::Reset() {
    auto reset = temple::GetPointer<void()>(0x10124870);
    reset();
}
bool UiHelpManager::SaveGame(TioFile *file) {
        auto save = temple::GetPointer<int(TioFile*)>(0x10124880);
        return save(file) == 1;
}
bool UiHelpManager::LoadGame(const UiSaveFile &save) {
        auto load = temple::GetPointer<int(const UiSaveFile*)>(0x101248b0);
        return load(&save) == 1;
}
const std::string &UiHelpManager::GetName() const {
    static std::string name("Help Manager-UI");
    return name;
}

//*****************************************************************************
//* Slider-UI
//*****************************************************************************

UiSlider::UiSlider(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x101626d0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system Slider-UI");
    }
}
UiSlider::~UiSlider() {
    auto shutdown = temple::GetPointer<void()>(0x10161840);
    shutdown();
}
void UiSlider::ResizeViewport(const UiResizeArgs& resizeArg) {
    auto resize = temple::GetPointer<void(const UiResizeArgs*)>(0x10160fa0);
    resize(&resizeArg);
}
const std::string &UiSlider::GetName() const {
    static std::string name("Slider-UI");
    return name;
}

//*****************************************************************************
//* written_ui
//*****************************************************************************

UiWritten::UiWritten(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10160ec0);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system written_ui");
    }
}
UiWritten::~UiWritten() {
    auto shutdown = temple::GetPointer<void()>(0x10160ee0);
    shutdown();
}
const std::string &UiWritten::GetName() const {
    static std::string name("written_ui");
    return name;
}

bool UiWritten::Show(objHndl handle)
{
	static auto ui_written_show = temple::GetPointer<BOOL(objHndl)>(0x10160f50);
	return ui_written_show(handle) != FALSE;
}

//*****************************************************************************
//* charmap_ui
//*****************************************************************************

UiCharmap::UiCharmap(const UiSystemConf &config) {
    auto startup = temple::GetPointer<int(const UiSystemConf*)>(0x10160d70);
    if (!startup(&config)) {
        throw TempleException("Unable to initialize game system charmap_ui");
    }
}
UiCharmap::~UiCharmap() {
    auto shutdown = temple::GetPointer<void()>(0x10160a50);
    shutdown();
}
const std::string &UiCharmap::GetName() const {
    static std::string name("charmap_ui");
    return name;
}

