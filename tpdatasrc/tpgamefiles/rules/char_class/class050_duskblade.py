from toee import *
import char_class_utils
import char_editor

###################################################

def GetConditionName(): # used by API
	return "Duskblade"

def GetCategory():
	return "Player's Handbook II"

def GetClassDefinitionFlags():
	return CDF_BaseClass | CDF_CoreClass

def GetClassHelpTopic():
	return "TAG_DUSKBLADES"

classEnum = stat_level_duskblade

###################################################

class_feats = {

1: (feat_armor_proficiency_light, feat_armor_proficiency_medium, feat_armor_proficiency_heavy, feat_shield_proficiency, feat_simple_weapon_proficiency, feat_martial_weapon_proficiency_all, "Arcane Attunement", "Duskblade Armored Mage",),
2: (feat_combat_casting,),
3: ("Arcane Channeling",),
5: ("Quick Cast",),
6: ("Spell Power",),
}

class_skills = (skill_alchemy, skill_climb, skill_concentration, skill_knowledge_all, skill_craft, skill_decipher_script, skill_jump, skill_ride, skill_sense_motive, skill_spellcraft, skill_swim)

spells_per_day = {
1:  (3, 2),
2:  (4, 3),
3:  (5, 4),
4:  (6, 5),
5:  (6, 5, 2),
6:  (6, 6, 3),
7:  (6, 6, 5),
8:  (6, 7, 6),
9:  (6, 7, 6, 2),
10: (6, 8, 7, 3),
11: (6, 8, 7, 5),
12: (6, 8, 8, 6),
13: (6, 9, 8, 6, 2),
14: (6, 9, 8, 7, 3),
15: (6, 9, 9, 7, 5),
16: (6, 10, 9, 8, 6),
17: (6, 10, 9, 8, 6, 2),
18: (6, 10, 10, 8, 7, 3),
19: (6, 10, 10, 9, 7, 5),
20: (6, 10, 10, 10, 8, 6)
#lvl 0  1   2   3   4  5  
}

spell_list = {
	0: (spell_acid_splash, spell_disrupt_undead, spell_ray_of_frost, spell_touch_of_fatigue),
	1: (spell_burning_hands, spell_cause_fear, spell_chill_touch, spell_color_spray, spell_expeditious_retreat, spell_jump, spell_magic_weapon, spell_obscuring_mist, spell_ray_of_enfeeblement, spell_endurance, spell_shocking_grasp, spell_true_strike),
	2: (spell_endurance, spell_bulls_strength, spell_cats_grace, spell_darkvision, spell_ghoul_touch, spell_melfs_acid_arrow, spell_see_invisibility, spell_spider_climb),
	3: (spell_keen_edge, spell_greater_magic_weapon, spell_protection_from_elements, spell_vampiric_touch),
	4: (spell_bigbys_interposing_hand, spell_dispel_magic, spell_enervation, spell_fire_shield, spell_phantasmal_killer, spell_shout),
	5: (spell_bigbys_clenched_fist, spell_chain_lightning, spell_disintegrate, spell_hold_monster, spell_polar_ray),
}

def IsEnabled():
	return 1

def GetDeityClass():
	return stat_level_sorcerer

def GetHitDieType():
	return 8
	
def GetSkillPtsPerLevel():
	return 2
	
def GetBabProgression():
	return base_attack_bonus_type_martial
	
def IsFortSaveFavored():
	return 1
	
def IsRefSaveFavored():
	return 0
	
def IsWillSaveFavored():
	return 1

# Spell casting
def GetSpellListType():
	return spell_list_type_special

def GetSpellSourceType():
	return spell_source_type_arcane

def GetSpellReadyingType():
	return spell_readying_innate

def HasArmoredArcaneCasterFeature():
	return 1

def GetSpellList():
	return spell_list

def GetSpellsPerDay():
	return spells_per_day

caster_levels = range(1, 21)
def GetCasterLevels():
	return caster_levels

def GetSpellDeterminingStat():
	return stat_intelligence

def IsClassSkill(skillEnum):
	return char_class_utils.IsClassSkill(class_skills, skillEnum)

def IsClassFeat(featEnum):
	return char_class_utils.IsClassFeat(class_feats, featEnum)

def GetClassFeats():
	return class_feats

def IsAlignmentCompatible(alignment):
	return 1
	
def ObjMeetsPrereqs(obj):
	abScore = obj.stat_base_get(stat_intelligence)
	if abScore > 10:
		return 1
	return 0

## Levelup callbacks

def IsSelectingSpellsOnLevelup( obj ):
	return 1
	
def InitSpellSelection( obj, classLvlNew = -1, classLvlIncrement = 1):
	classLvl = obj.stat_level_get(classEnum)
	if classLvlNew <= 0:
		classLvlNew = classLvl + 1
	maxSpellLvl = char_editor.get_max_spell_level( obj, classEnum, classLvlNew ) # this regards spell list extension by stuff like Mystic Theurge
	
	# Available Spells
	spAvail = char_editor.get_learnable_spells(obj, classEnum, maxSpellLvl)
	# add spell level labels
	for p in range(0,maxSpellLvl+1):
		spAvail.append(char_editor.KnownSpellInfo(spell_label_level_0 + p, 0, classEnum))
	spAvail.sort()
	char_editor.append_available_spells(spAvail)
	
	# newly taken class ()
	if classLvlNew == 1:
		spEnums = []
		spEnums.append(char_editor.KnownSpellInfo(spell_label_level_0, 0, classEnum)) # add "Level 0" label
		level0Spells = 2 + (obj.stat_level_get(stat_intelligence) - 10) / 2
		level0Spells = int(level0Spells)
		
		#Make sure not to add too many zero level slots to avoid lockup
		spellList = char_editor.get_learnable_spells(obj, classEnum, 0)
		spellAvailableCount = 0
		for spell in spellList:
			if not obj.is_spell_known(spell.spell_enum):
				spellAvailableCount = spellAvailableCount + 1
		level0Spells = min(level0Spells, spellAvailableCount)
		
		for p in range(0,level0Spells): # 2 + int bonus cantrips
			spEnums.append(char_editor.KnownSpellInfo(spell_new_slot_lvl_0, 3, classEnum))
		spEnums.append(char_editor.KnownSpellInfo(spell_label_level_1, 0, classEnum)) # add "Level 1" label
		for p in range(0,2): # 2 level 1 spells
			spEnums.append(char_editor.KnownSpellInfo(spell_new_slot_lvl_1, 3, classEnum))
		char_editor.append_spell_enums(spEnums)
		return 0
	
	# Incrementing class level
	spellListLvl = obj.stat_level_get(stat_spell_list_level, classEnum) + classLvlIncrement # the effective level for getting the number of spells known
	spEnums = char_editor.get_known_class_spells(obj, classEnum) # get all spells known for this class
	for spellLvl in range(0, maxSpellLvl+1):
		spEnums.append(char_editor.KnownSpellInfo(spell_label_level_0 + spellLvl, 0, classEnum))  # add label
	
	isReplacing = 0
	if spellListLvl >= 5 and (spellListLvl % 2) == 1: # spell replacement
		isReplacing = 1
	if char_editor.get_class_code() !=  classEnum: #grant this benefit only for strict levelup (also to prevent some headache...)
		isReplacing = 0
	
	if isReplacing:
		# mark as replaceable
		for p in range(0,len(spEnums)):
			spEnum = spEnums[p].spell_enum
			if spell_vacant <= spEnum <= spell_label_level_9:
				continue
			if spell_new_slot_lvl_0 <= spEnum <= spell_new_slot_lvl_9:
				continue
			if char_editor.get_spell_level(spEnum, classEnum) <= maxSpellLvl-2:
				spEnums[p].spell_status = 1 # marked as replaceable
	
	vacant_slot = char_editor.KnownSpellInfo(spell_vacant, 3, classEnum) # sets it to spell level -1
	spEnums.append(vacant_slot) #New spell of any level
	spEnums.sort()
	char_editor.append_spell_enums(spEnums)
	return 0

def LevelupCheckSpells( obj ):
	classLvl = obj.stat_level_get(classEnum)
	classLvlNew = classLvl + 1
	maxSpellLvl = char_editor.get_max_spell_level( obj, classEnum, classLvlNew )
	
	spell_enums = char_editor.get_spell_enums()
	for spInfo in spell_enums:
		if spInfo.spell_enum == spell_vacant:
			return 0
	return 1

def LevelupSpellsFinalize( obj, classLvlNew = -1 ):
	spEnums = char_editor.get_spell_enums()
	char_editor.spell_known_add(spEnums) # internally takes care of duplicates and the labels/vacant slots
	return
	