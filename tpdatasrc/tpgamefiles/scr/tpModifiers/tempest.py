from templeplus.pymod import PythonModifier
from toee import *
import tpdp
import char_class_utils
import tpactions

###################################################

def GetConditionName():
	return "Tempest"

print "Registering " + GetConditionName()

classEnum = stat_level_swashbuckler
classSpecModule = __import__('class090_tempest')
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

classSpecObj = PythonModifier(GetConditionName(), 0)
classSpecObj.AddHook(ET_OnToHitBonusBase, EK_NONE, OnGetToHitBonusBase, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_FORTITUDE, OnGetSaveThrowFort, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_REFLEX, OnGetSaveThrowReflex, ())
classSpecObj.AddHook(ET_OnSaveThrowLevel, EK_SAVE_WILL, OnGetSaveThrowWill, ())

#Checks for a load greater than light or armor greater than light (to enable various abilities)
def TempestEncomberedCheck(obj):
	#Light armor or no armor
	armor = obj.item_worn_at(5)
	if armor != OBJ_HANDLE_NULL:
		armorFlags = armor.obj_get_int(obj_f_armor_flags)
		if (armorFlags != ARMOR_TYPE_LIGHT) and (armorFlags !=  ARMOR_TYPE_NONE):
			return 1
	return 0
	
#Check if the weapons is usable with finesse 
def IsTwoWeaponFighting(obj):
	weaponPrimary = obj.item_worn_at(item_wear_weapon_primary)
	weaponSecondary = obj.item_worn_at(item_wear_weapon_secondary)
	
	#Weapons must not be the same
	if weaponPrimary == weaponSecondary:
		return 0
	
	#Both hands must have a weapon
	if weaponPrimary == OBJ_HANDLE_NULL or weaponSecondary == OBJ_HANDLE_NULL:
		return 0

	return 1
	
#Add an attack bonus from the opposite weapon if they don't already have it
def AddBonusFromOtherWeapon(attachee, weaponUsed, baseFeat, bonus, bonus_list):
	#Must be two weapon fighting
	if not IsTwoWeaponFighting(attachee):
		return 0
		
	#Must not already have the feat for the current weapon
	weaponUsedType = weaponUsed.obj_get_int(obj_f_weapon_type)
	featCheckUsed = baseFeat + weaponUsedType
	if (attachee.has_feat(featCheckUsed)):
		return 0

	#Must have the feat for the other weapon
	weaponPrimary = attachee.item_worn_at(item_wear_weapon_primary)
	weaponSecondary = attachee.item_worn_at(item_wear_weapon_secondary)
	
	otherWeapon = weaponPrimary
	if (otherWeapon == weaponUsed):
		otherWeapon = weaponSecondary
		
	weaponTypeOther = otherWeapon.obj_get_int(obj_f_weapon_type) 
	featCheckOther = weaponTypeOther + baseFeat
	if not attachee.has_feat(featCheckOther):
		return 0
	
	#Add the bonus
	weaponType = weaponUsed.obj_get_int(obj_f_weapon_type) 
	bonus_list.add_from_feat(bonus, 12, 114,baseFeat+weaponType)
	
	return 1

#Tempest Abilities

# Tempest Defense

def TempestACBonus(attachee, args, evt_obj):
	#Must not be encumbered
	if TempestEncomberedCheck(attachee):
		return 0
	
	if not IsTwoWeaponFighting(attachee):
		return 0

	classLvl = attachee.stat_level_get(classEnum)
	if classLvl < 3:
		bonval = 1
	elif classLvl < 5:
		bonval = 2
	else:
		bonval = 3
	evt_obj.bonus_list.add(bonval, 0, "Tempest Defense AC Bonus" ) #Unnamed Bonus
	return 0

dervishACBonus = PythonModifier("Tempest Defense", 2) #Spare, Spare
dervishACBonus.MapToFeat("Tempest Defense")
dervishACBonus.AddHook(ET_OnGetAC, EK_NONE, TempestACBonus, ())

# Tempest Ambidexterity

def TempestAmbidexterityAttack(attachee, args, evt_obj):
	#Only apply for full attack and checks from the character sheet
	if (evt_obj.attack_packet.get_flags() & D20CAF_FINAL_ATTACK_ROLL != 0) and (evt_obj.attack_packet.get_flags() & D20CAF_FULL_ATTACK == 0):
		return 0

	if TempestEncomberedCheck(attachee):
		return 0
		
	if not IsTwoWeaponFighting(attachee):
		return 0
		
	classLvl = attachee.stat_level_get(classEnum)
	if classLvl < 4:
		bonval = 1
	else:
		bonval = 2
	
	#Bonus of 2 for the on and off hand will result in the same bonus as a light weapon
	evt_obj.bonus_list.add_from_feat(bonval, 0, 114, "Tempest Ambidexterity")
	return 0

tempestAmbidexterity = PythonModifier("Tempest Ambidexterity", 2) #Spare, Spare
tempestAmbidexterity.MapToFeat("Tempest Ambidexterity")
tempestAmbidexterity.AddHook(ET_OnToHitBonus2, EK_NONE, TempestAmbidexterityAttack, ())

# Tempest Two-Weapon Versatility
def TempestTwoWeaponVersatilityAttackBonus(attachee, args, evt_obj):
	weaponUsed = evt_obj.attack_packet.get_weapon_used()
	if weaponUsed == OBJ_HANDLE_NULL:
		return 0

	AddBonusFromOtherWeapon(attachee, weaponUsed, feat_weapon_focus_gauntlet, 1, evt_obj.bonus_list)
	AddBonusFromOtherWeapon(attachee, weaponUsed, feat_greater_weapon_focus_gauntlet, 1, evt_obj.bonus_list)
	
	return 0
	
def TempestTwoWeaponVersatilityDamageBonus(attachee, args, evt_obj):
	weaponUsed = evt_obj.attack_packet.get_weapon_used()
	if weaponUsed == OBJ_HANDLE_NULL:
		return 0

	AddBonusFromOtherWeapon(attachee, weaponUsed, feat_weapon_specialization, 2, evt_obj.bonus_list)
	AddBonusFromOtherWeapon(attachee, weaponUsed, feat_greater_weapon_specialization, 2, evt_obj.bonus_list)
		
	return 0
	
def TempestTwoWeaponVersatilityCritical(attachee, args, evt_obj):
	weaponUsed = evt_obj.attack_packet.get_weapon_used()
	if weaponUsed == OBJ_HANDLE_NULL:
		return 0
		
	critRange = weaponUsed.obj_get_int(obj_f_weapon_crit_range)
	AddBonusFromOtherWeapon(attachee, weaponUsed, feat_improved_critical_gauntlet, critRange, evt_obj.bonus_list)
	
	return 0

tempestVersatility = PythonModifier("Tempest Two-Weapon Versatility", 2) #Spare, Spare
tempestVersatility.MapToFeat("Tempest Two-Weapon Versatility")
tempestVersatility.AddHook(ET_OnToHitBonus2, EK_NONE, TempestTwoWeaponVersatilityAttackBonus, ())
tempestVersatility.AddHook(ET_OnDealingDamage, EK_NONE, TempestTwoWeaponVersatilityDamageBonus, ())
tempestVersatility.AddHook(ET_OnGetCriticalHitRange, EK_NONE, TempestTwoWeaponVersatilityCritical, ())


#Tempest Spring Attack

tempestSpringAttack = PythonModifier("Tempest Two-Weapon Spring Attack", 3) #Move Distance, Spare, Spare
tempestSpringAttack.MapToFeat("Tempest Two-Weapon Spring Attack")
