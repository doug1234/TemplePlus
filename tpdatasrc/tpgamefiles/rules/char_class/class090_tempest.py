from toee import *
import char_class_utils
import char_editor

#Complete Adventurer, p. 81

###################################################

def GetConditionName(): # used by API
	return "Tempest"

def GetCategory():
	return "Complete Adventurer"

def GetClassDefinitionFlags():
	return CDF_None

def GetClassHelpTopic():
	return "TAG_TEMPESTS"

classEnum = stat_level_tempest

###################################################

class_feats = {

1: ("Tempest Defense",),
2: ("Tempest Ambidexterity",),
3: ("Tempest Two-Weapon Versatility",),
5: ("Tempest Two-Weapon Spring Attack",),
}

class_skills = (skill_balance, skill_climb, skill_craft, skill_jump, skill_tumble)

def IsEnabled():
	return 1

def GetDeityClass():
	return stat_level_fighter

def GetHitDieType():
	return 10
	
def GetSkillPtsPerLevel():
	return 2
	
def GetBabProgression():
	return base_attack_bonus_type_martial
	
def IsFortSaveFavored():
	return 1
	
def IsRefSaveFavored():
	return 0
	
def IsWillSaveFavored():
	return 0

def GetSpellListType():
	return spell_list_type_none

def IsClassSkill(skillEnum):
	return char_class_utils.IsClassSkill(class_skills, skillEnum)

def IsClassFeat(featEnum):
	return char_class_utils.IsClassFeat(class_feats, featEnum)

def GetClassFeats():
	return class_feats
	
def IsAlignmentCompatible( alignment):
	return 1

def ObjMeetsPrereqs( obj ):
	#Maximum number of levels is 5
	classLvl = obj.stat_level_get(classEnum)
	if classLvl >= 5:
		return 0

	if obj.get_base_attack_bonus() < 6:
		return 0
	
	if not obj.has_feat(feat_dodge):
		return 0
		
	if not obj.has_feat(feat_mobility):
		return 0
		
	if not obj.has_feat(feat_spring_attack):
		return 0
		
	if not obj.has_feat(feat_two_weapon_fighting) and not obj.has_feat(feat_two_weapon_fighting_ranger):
		return 0
		
	if not obj.has_feat(feat_improved_two_weapon_fighting) and not obj.has_feat(feat_improved_two_weapon_fighting_ranger):
		return 0
		
	return 1
	
def IsSelectingFeatsOnLevelup( obj ):
	return 0
	
def LevelupGetBonusFeats( obj ):
	return
	
def LevelupSpellsFinalize( obj, classLvlNew = -1 ):
	return 0
