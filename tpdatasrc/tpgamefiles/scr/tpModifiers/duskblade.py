from templeplus.pymod import PythonModifier
from toee import *
import tpdp
import char_class_utils
import tpactions

###################################################

def GetConditionName():
	return "Duskblade"

print "Registering " + GetConditionName()

classEnum = stat_level_duskblade
classSpecModule = __import__('class050_duskblade')
###################################################


#### standard callbacks - BAB and Save values
def OnGetToHitBonusBase(attachee, args, evt_obj):
	classLvl = attachee.stat_level_get(classEnum)
	babvalue = game.get_bab_for_class(classEnum, classLvl)
	evt_obj.bonus_list.add(babvalue, 0, 137) # untyped, description: "Class"
	return 0

def OnGetSaveThrowFort(attachee, args, evt_obj):
	value = char_class_utils.SavingThrowLevel(classEnum, attachee, D20_Save_Fortitude)
	evt_obj.bonus_list.add(value, 0, 137)
	return 0

def OnGetSaveThrowReflex(attachee, args, evt_obj):
	value = char_class_utils.SavingThrowLevel(classEnum, attachee, D20_Save_Reflex)
	evt_obj.bonus_list.add(value, 0, 137)
	return 0

def OnGetSaveThrowWill(attachee, args, evt_obj):
	value = char_class_utils.SavingThrowLevel(classEnum, attachee, D20_Save_Will)
	evt_obj.bonus_list.add(value, 0, 137)
	return 0

### Spell casting
def OnGetBaseCasterLevel(attachee, args, evt_obj):
	if evt_obj.arg0 != classEnum:
		return 0
	classLvl = attachee.stat_level_get(classEnum)
	evt_obj.bonus_list.add(classLvl, 0, 137)
	return 0

def OnInitLevelupSpellSelection(attachee, args, evt_obj):
	if evt_obj.arg0 != classEnum:
		return 0
	classSpecModule.InitSpellSelection(attachee)
	return 0
	
def OnLevelupSpellsCheckComplete(attachee, args, evt_obj):
	if evt_obj.arg0 != classEnum:
		return 0
	if not classSpecModule.LevelupCheckSpells(attachee):
		evt_obj.bonus_list.add(-1, 0, 137) # denotes incomplete spell selection
	return 1

def OnLevelupSpellsFinalize(attachee, args, evt_obj):
	if evt_obj.arg0 != classEnum:
		return 0
	classSpecModule.LevelupSpellsFinalize(attachee)
	return

def ArcaneSpellFailure(attachee, args, evt_obj):
	if evt_obj.data1 != classEnum and evt_obj.data1 != stat_level_wizard:
		return 0

	equip_slot = evt_obj.data2
	item = attachee.item_worn_at(equip_slot)

	if item == OBJ_HANDLE_NULL:
		return 0

	evt_obj.return_val += item.obj_get_int(obj_f_armor_arcane_spell_failure)
	return 0

classSpecObj = PythonModifier(GetConditionName(), 0)
classSpecObj.AddHook(ET_OnToHitBonusBase, EK_NONE, OnGetToHitBonusBase, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_FORTITUDE, OnGetSaveThrowFort, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_REFLEX, OnGetSaveThrowReflex, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_WILL, OnGetSaveThrowWill, ())
classSpecObj.AddHook(ET_OnGetBaseCasterLevel, EK_NONE, OnGetBaseCasterLevel, ())
classSpecObj.AddHook(ET_OnLevelupSystemEvent, EK_LVL_Spells_Activate, OnInitLevelupSpellSelection, ())
classSpecObj.AddHook(ET_OnLevelupSystemEvent, EK_LVL_Spells_Finalize, OnLevelupSpellsFinalize, ())
classSpecObj.AddHook(ET_OnLevelupSystemEvent, EK_LVL_Spells_Check_Complete, OnLevelupSpellsCheckComplete, ())
classSpecObj.AddHook(ET_OnD20Query, EK_Q_Get_Arcane_Spell_Failure, ArcaneSpellFailure, ())

#Armored Mage

def DuskbladeSpellFailure(attachee, args, evt_obj):
	#Only effects spells cast as a duskblade
	if evt_obj.data1 != classEnum:
		return 0

	equip_slot = evt_obj.data2
	item = attachee.item_worn_at(equip_slot)

	if item == OBJ_HANDLE_NULL:
		return 0
		
	if equip_slot == item_wear_armor: # duskblade can cast in light armor (and medium armor at level 4 or greater, heavy shelds at level 7) with no spell failure
		duskbladeLevel = attachee.stat_level_get(classEnum)
		armor_flags = item.obj_get_int(obj_f_armor_flags)
		if attachee.d20_query("Improved Armored Casting"):
			if (armor_flags & ARMOR_TYPE_NONE) or (armor_flags == ARMOR_TYPE_LIGHT) or (armor_flags == ARMOR_TYPE_MEDIUM) or (duskbladeLevel > 4):
				return 0
		else:
			if (armor_flags & ARMOR_TYPE_NONE) or (armor_flags == ARMOR_TYPE_LIGHT) or ((armor_flags == ARMOR_TYPE_MEDIUM) and (duskbladeLevel > 4)):
				return 0
	
	if equip_slot == item_wear_shield:  # duskblade can cast with a light shield (or buclker) with no spell failure
		shieldFailure = item.obj_get_int(obj_f_armor_arcane_spell_failure)
		if shieldFailure <= 5: #Light shields and bucklers have 5% spell failure
			return 0
		if duskbladeLevel > 7:
			if shieldFailure <= 15:
				return 0
			
	evt_obj.return_val += item.obj_get_int(obj_f_armor_arcane_spell_failure)
	return 0

armoredMage = PythonModifier("Duskblade Armored Mage", 2) #Spare, Spare
armoredMage.MapToFeat("Duskblade Armored Mage")
armoredMage.AddHook(ET_OnD20Query, EK_Q_Get_Arcane_Spell_Failure, DuskbladeSpellFailure, ())

#Arcane Attunement (cast dancing lights, detect magic, flare, ghost sound, and read magic)

ArcaneAttunementEnum = 2900

def ArcaneAttunementStrikeRadial(attachee, args, evt_obj):
	
	radial_parent = tpdp.RadialMenuEntryParent("Arcane Attunement")
	ArcaneAttunementId = radial_parent.add_child_to_standard(attachee, tpdp.RadialMenuStandardNode.Feats)
	
	spell_data = PySpellStore(spell_detect_magic, domain_special, 0)
	radialAction = tpdp.RadialMenuEntryPythonAction(spell_data, D20A_PYTHON_ACTION, ArcaneAttunementEnum,0)
	radialAction.add_as_child(attachee, ArcaneAttunementId)

	spell_data = PySpellStore(spell_flare, domain_special, 0)
	radialAction = tpdp.RadialMenuEntryPythonAction(spell_data, D20A_PYTHON_ACTION, ArcaneAttunementEnum,0)
	radialAction.add_as_child(attachee, ArcaneAttunementId)
	
	spell_data = PySpellStore(spell_read_magic, domain_special, 0)
	radialAction = tpdp.RadialMenuEntryPythonAction(spell_data, D20A_PYTHON_ACTION, ArcaneAttunementEnum,0)
	radialAction.add_as_child(attachee, ArcaneAttunementId)
	
	return 0

def OnArcaneAttunementCheck(attachee, args, evt_obj):
	charges = args.get_arg(0)

	#Check charges reamaining
	if (charges < 1):
		evt_obj.return_val = AEC_OUT_OF_CHARGES
		return 0
		
	return 1

def OnArcaneAttunementPerform(attachee, args, evt_obj):
	charges = args.get_arg(0)
	if charges > 0:
		charges = charges - 1
		args.set_arg(0, charges)
		cur_seq = tpactions.get_cur_seq()
		evt_obj.d20a.filter_spell_targets(cur_seq.spell_packet)
		new_spell_id = tpactions.get_new_spell_id()
		tpactions.register_spell_cast(cur_seq.spell_packet, new_spell_id)
		evt_obj.d20a.spell_id = new_spell_id
		cur_seq.spell_action.spell_id = new_spell_id
		tpactions.trigger_spell_effect(new_spell_id)
	return 0
	
def ArcaneAttunementNewDay(attachee, args, evt_obj):
	intValue = attachee.stat_level_get(stat_intelligence)
	charges = (intValue - 10)/2 + 3
	
	#One charge per day
	args.set_arg(0, charges)

	return 0

arcaneAttunement = PythonModifier("Arcane Attunement", 2) #Charges, Spare
arcaneAttunement.MapToFeat("Arcane Attunement")
arcaneAttunement.AddHook(ET_OnD20PythonActionCheck, ArcaneAttunementEnum, OnArcaneAttunementCheck, ())
arcaneAttunement.AddHook(ET_OnD20PythonActionPerform, ArcaneAttunementEnum, OnArcaneAttunementPerform, ())
arcaneAttunement.AddHook(ET_OnBuildRadialMenuEntry, EK_NONE, ArcaneAttunementStrikeRadial, ())
arcaneAttunement.AddHook(ET_OnConditionAdd, EK_NONE, ArcaneAttunementNewDay, ())
arcaneAttunement.AddHook(ET_OnNewDay, EK_NEWDAY_REST, ArcaneAttunementNewDay, ())

#Arcane Channeling

ArcaneChannelingEnum = 2901

def ArcaneChannelingNewDay(attachee, args, evt_obj):
	
	#Clear everything on a new day
	args.set_arg(0, 0)
	args.set_arg(1, 0)
	args.set_arg(2, 0)
	args.set_arg(3, 0)
	return 0

def ArcaneChannelingRadial(attachee, args, evt_obj):
	radial_parent = tpdp.RadialMenuEntryParent("Arcane Channeling")
	ArcaneChannelingId = radial_parent.add_child_to_standard(attachee, tpdp.RadialMenuStandardNode.Feats)

	spell_level_ids = []
	for p in range(0,10):
		spell_level_node = tpdp.RadialMenuEntryParent(str(p))
		spell_level_ids.append( spell_level_node.add_as_child(attachee, ArcaneChannelingId) )
	
	#Should be checking casting time and metamagic for spontanious cast
	
	known_spells = attachee.spells_known
	for knSp in known_spells:
		if knSp.is_naturally_cast() and attachee.can_cast_spell(knSp) and knSp.is_touch_attack_spell():
			spell_node = tpdp.RadialMenuEntryPythonAction(knSp, D20A_PYTHON_ACTION, ArcaneChannelingEnum,0)
			spell_node.add_as_child(attachee, spell_level_ids[knSp.spell_level])
	
	#Duskblade spells were coming up as memorized also due to the way they were picked... need to do something about that
	mem_spells = attachee.spells_memorized
	for memSp in mem_spells:
		if not memSp.is_used_up() and not memSp.is_naturally_cast() and memSp.is_touch_attack_spell() and attachee.can_cast_spell(memSp):
			spell_node = tpdp.RadialMenuEntryPythonAction(memSp, D20A_PYTHON_ACTION, ArcaneChannelingEnum, 0)
			spell_node.add_as_child(attachee, spell_level_ids[memSp.spell_level])
	
	return 0
	
def OnArcaneChannelPerform(attachee, args, evt_obj):
	cur_seq = tpactions.get_cur_seq()
	effectID = cur_seq.spell_packet.get_touch_attach_charge_enum()
	if effectID <= 0:
		return 0
	new_spell_id = tpactions.get_new_spell_id()
	tpactions.register_spell_cast(cur_seq.spell_packet, new_spell_id)
	evt_obj.d20a.spell_id = new_spell_id
	cur_seq.spell_action.spell_id = new_spell_id
	cur_seq.spell_packet.debit_spell()
	tpactions.trigger_spell_effect(new_spell_id)
	args.set_arg(0, 1) #Enable Ability
	args.set_arg(1, effectID) #Set the effect ID
	args.set_arg(2, 1) #Default to single attack
	args.set_arg(3, 1) #Apply cost mod flag
	return 0

	
def ArcaneChannelingOnDamage(attachee, args, evt_obj):
	#Must not be a ranged attack
	if evt_obj.attack_packet.get_flags() & D20CAF_RANGED:
		return 0
	
	#Check the eanbled flag
	if not args.get_arg(0):
		return 0
	
	target = evt_obj.attack_packet.target
	if target == OBJ_HANDLE_NULL:
		return 0
	
	attachee.trigger_touch_effect(target)
	
	effectID = args.get_arg(1)
	attachee.remove_spell_condition(effectID)
	
	#Add the condition back for full round
	fullRoundAction = args.get_arg(2)
	if fullRoundAction:
		attachee.add_spell_condition(effectID)
	else:
		args.set_arg(0, 0) #Disable ability since this is a single attack
	
	return 0
	
def ArcaneChannelingCostMod(attachee, args, evt_obj):
	#Primary weapon must be empty or a meelee weapon
	item = attachee.item_worn_at(item_wear_weapon_primary)
	
	if item <> OBJ_HANDLE_NULL:
		weaponType = item.obj_get_int(obj_f_weapon_type)
		if not game.is_melee_weapon(weaponType):
			return 0
		
	#Check the eanbled flag
	if not args.get_arg(0):
		return 0
	
	doCostMod = args.get_arg(3) #Check if the cost mod should be applied
	if not doCostMod:
		return 0

	#One standard attack can be done for free
	if evt_obj.d20a.action_type == tpdp.D20ActionType.StandardAttack:
		evt_obj.cost_new.action_cost = 0
		args.set_arg(2,0)
		return 0
	
	#Discount the full round to a move action (since a standard action has been charged)
	if evt_obj.d20a.action_type == tpdp.D20ActionType.FullAttack:
		classLvl = attachee.stat_level_get(classEnum)
		if classLvl > 12:
			evt_obj.cost_new.action_cost = 1 #Move Action
			args.set_arg(2,1)
			
	return 0
	
def ArcaneChannelingEndTurn(attachee, args, evt_obj):
	#For a full round action, make sure the condition is gone at then end
	
	fullRoundAction = args.get_arg(2)
	if fullRoundAction:
		effectID = args.get_arg(1)
		attachee.remove_spell_condition(effectID)
	
	args.set_arg(3, 0) #Disable cost mod (only effects the round it was used)

	return 0

arcaneChanneling = PythonModifier("Arcane Channeling", 4) #Enabled Flag, Spell Effect ID, Action Type Flag, Do Cost Mod
arcaneChanneling.MapToFeat("Arcane Channeling")
arcaneChanneling.AddHook(ET_OnBuildRadialMenuEntry, EK_NONE, ArcaneChannelingRadial, ())
arcaneChanneling.AddHook(ET_OnD20PythonActionPerform, ArcaneChannelingEnum, OnArcaneChannelPerform, ())
arcaneChanneling.AddHook(ET_OnConditionAdd, EK_NONE, ArcaneChannelingNewDay, ())
arcaneChanneling.AddHook(ET_OnNewDay, EK_NEWDAY_REST, ArcaneChannelingNewDay, ())
arcaneChanneling.AddHook(ET_OnDealingDamage, EK_NONE, ArcaneChannelingOnDamage, ())
arcaneChanneling.AddHook(ET_OnActionCostMod, EK_NONE, ArcaneChannelingCostMod, ())
arcaneChanneling.AddHook(ET_OnD20PythonSignal, "End Turn", ArcaneChannelingEndTurn, ())

def HitArcaneChannelingBeginRound(attachee, args, evt_obj):
	args.condition_remove() #Always disapears at the begining of the round
	return 0
	
def AlreadyHitArcaneChanneling(attachee, args, evt_obj):
	queryHandle1 = querySpellID = evt_obj.data1
	queryHandle2 = querySpellID = evt_obj.data2
	currentHandle1 = args.get_arg(0)
	currentHandle2 = args.get_arg(1)
	
	if queryHandle1 == currentHandle1 and queryHandle2 == currentHandle2:
		evt_obj.return_val = 1
		
	return 0

hitArcaneChanneling = PythonModifier("Hit Arcane Channeling", 4) #Upper, Lower, Spare, Spare
hitArcaneChanneling.AddHook(ET_OnBeginRound, EK_NONE, HitArcaneChannelingBeginRound, ())
hitArcaneChanneling.AddHook(ET_OnD20PythonQuery, "Hit Arcane Channeling", AlreadyHitArcaneChanneling, ())

#Quick Cast

def QuickCastRadial(attachee, args, evt_obj):
	#Add a checkbox to turn on and off quick cast if there is a carge available, otherwise just don't show it
	if args.get_arg(0):
		checkboxQuickCast = tpdp.RadialMenuEntryToggle("Quick Fast", "TAG_INTERFACE_HELP")
		checkboxQuickCast.link_to_args(args, 1)
		checkboxQuickCast.add_child_to_standard(attachee, tpdp.RadialMenuStandardNode.Feats)
	return 0

def QuickCastNewDay(attachee, args, evt_obj):
	#One charge per 5 levels
	classLvl = attachee.stat_level_get(classEnum)
	charges = int(classLvl/5)
	args.set_arg(0, charges)
	
	#Set the checkbox to off at the begining of the day
	args.set_arg(1, 0)
	
	return 0

def QuickCastMetamagicUpdate(attachee, args, evt_obj):
	#Check for a charge
	charges = args.get_arg(0)
	if charges < 1:
		return 0
	
	#Check if quick cast is turned on
	if not args.get_arg(1):
		return 0
	
	#Get the metamagic info
	metaMagicData = evt_obj.meta_magic
	
	#Don't quicken more than once
	if metaMagicData.get_quicken() < 1:
		metaMagicData.set_quicken(1)
	
	return 0
	
	
def QuickCastDeductCharge(attachee, args, evt_obj):
	#Check for a charge and the enable flag
	charges = args.get_arg(0)
	if charges < 1 or not args.get_arg(1):
		return 0
		
	#Decriment the charges
	charges = charges - 1
	args.set_arg(0, charges)

	return 0

quickCast = PythonModifier("Quick Cast", 4) #Charges, Toggeled On, Full Round Mode, Spare
quickCast.MapToFeat("Quick Cast")
quickCast.AddHook(ET_OnBuildRadialMenuEntry, EK_NONE, QuickCastRadial, ())
quickCast.AddHook(ET_OnConditionAdd, EK_NONE, QuickCastNewDay, ())
quickCast.AddHook(ET_OnNewDay, EK_NEWDAY_REST, QuickCastNewDay, ())
quickCast.AddHook(ET_OnMetaMagicMod, EK_NONE, QuickCastMetamagicUpdate, ())
quickCast.AddHook(ET_OnD20PythonSignal, "Sudden Metamagic Deduct Charge", QuickCastDeductCharge, ())

#Spell Power
def SpellPowerDamageBeginRound(attachee, args, evt_obj):
	#The condition is removed after 1000 rounds.  No time limit was given in the decrtipion but it seems like it should go away at some point
	numRounds = args.get_arg(0)
	roundsToReduce = evt_obj.data1
	if numRounds - roundsToReduce < 1:
		args.set_arg(1, numRounds - roundsToReduce)
		return 0

	args.condition_remove()
	return 0
	
	
def SpellPowerDamageFromSpellQuery(attachee, args, evt_obj):
	condObject = args.get_obj_from_args(0)
	queryObject = evt_obj.get_obj_from_args() #Need to add... query call from magic armor merge...
	if (condObject == queryObject):
		evt_obj.return_val = 1

	return 0

spellPowerDamage = PythonModifier("Spell Power Damage", 4, False) #Duration, Upper Handle, Lower Handle, Spare
spellPowerDamage.AddHook(ET_OnBeginRound, EK_NONE, SpellPowerDamageBeginRound, ())
spellPowerDamage.AddHook(ET_OnD20PythonQuery, "Spell Power Damage Taken", SpellPowerDamageFromSpellQuery, ())

def SpellPowerOnDamage(attachee, args, evt_obj):
	target = evt_obj.attack_packet.target
	if target == OBJ_HANDLE_NULL:
		return 0
	
	#Must not be a ranged attack
	if evt_obj.attack_packet.get_flags() & D20CAF_RANGED:
		return 0
	
	hasCondition = target.d20_query_with_object("Spell Power Damage", attachee)
	if not hasCondition:
		target.condition_add_with_object("Spell Power Damage", 1000, attachee) #<---- Need to add...
	
	return 0
	
def SpellPowerSpellResistanceBonus(attachee, args, evt_obj):
	target = evt_obj.target
	if not target.d20_query_with_object("Spell Power Damage Taken", attachee):
		return 0
	
	duskBladeLevel = attachee.stat_level_get(classEnum)
	if duskBladeLevel < 11:
		evt_obj.bonus_list.add(2, 0, "Spell Power")
	if duskBladeLevel < 16:
		evt_obj.bonus_list.add(3, 0, "Spell Power")
	if duskBladeLevel < 18:
		evt_obj.bonus_list.add(4, 0, "Spell Power")
	else:
		evt_obj.bonus_list.add(5, 0, "Spell Power")
	return 0

spellPower = PythonModifier("Spell Power", 4) #Spare, Spare, Spare, Spare
spellPower.MapToFeat("Spell Power")
spellPower.AddHook(ET_OnDealingDamage, EK_NONE, SpellPowerOnDamage, ())
spellPower.AddHook(ET_OnSpellResistanceCheckBonus, EK_NONE, SpellPowerSpellResistanceBonus, ())
