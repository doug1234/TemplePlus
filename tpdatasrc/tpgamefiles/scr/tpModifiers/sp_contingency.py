from templeplus.pymod import PythonModifier
from toee import *
import tpdp
from utilities import *
print "Registering sp_contingency"

# args: (0-8)
# 0 - spell_id
# 1 - duration
# 2 - mode (0 = waiting for spell selection, 1 = spell loaded)

contingencySpellSelectionEnum = 3000

#Swift Arcane Boost
def GetContingencyDescription(contingencyType):
	if contingencyType == 1:
		return "Combat Begin"
	elif contingencyType == 2:
		return "Take Damage"

def ContingencyTooltip(attachee, args, evt_obj):
	# Set the tooltip
	evt_obj.append("Contingency (" + str(args.get_arg(1)) + " rounds)")
	return 0
	
def ContingencyEffectTooltip(attachee, args, evt_obj):
	# Set the tooltip
	evt_obj.append(tpdp.hash("CONTINGENCY"), -2, " (" + str(args.get_arg(1)) + " rounds)")
	return 0
	
def ContingencyHasSpellActive(attachee, args, evt_obj):
	if evt_obj.data1 == 77:
		evt_obj.return_val = 1
	return 0
	
def ContingencyRadial(attachee, args, evt_obj):
	print "ContingencyRadial"

	contingencyType = args.get_param(2)
	
	print "Contingency Type"
	print contingencyType
	
	if contingencyType != 0:
		return 0
		
	print "Making menu!"
	
	radialParent = tpdp.RadialMenuEntryParent("Contingency")
	ContingencySpellSelectID = radialParent.add_child_to_standard(attachee, tpdp.RadialMenuStandardNode.Class)

	#Range is 1/3 caster level to a max of 6

	#Iterate over all effect types ()
	for contingencyType in range(1,3):
		effectName = GetContingencyDescription(contingencyType)
		
		#Add the effect to the arcane boost id menu
		effectNode = tpdp.RadialMenuEntryParent(effectName)
		effectID = effectNode.add_as_child(attachee, ContingencySpellSelectID)
		
		# create the spell level nodes (level must be greater than 1)
		spell_level_ids = []
		
		#Add the name for the arcane boost effect type sub menu
		for p in range(1,10):
			spell_level_node = tpdp.RadialMenuEntryParent(str(p))
			spell_level_ids.append( spell_level_node.add_as_child(attachee, effectID) )

		known_spells = attachee.spells_known

		for knSp in known_spells:
			if knSp.is_naturally_cast() and (knSp.spell_level > 0):
				spell_node = tpdp.RadialMenuEntryPythonAction(knSp, D20A_PYTHON_ACTION, contingencySpellSelectionEnum, contingencyType)
				spell_node.add_as_child(attachee, spell_level_ids[knSp.spell_level-1])

		mem_spells = attachee.spells_memorized
		for memSp in mem_spells:
			if (not memSp.is_used_up()) and (memSp.spell_level > 0):
				spell_node = tpdp.RadialMenuEntryPythonAction(memSp, D20A_PYTHON_ACTION, contingencySpellSelectionEnum, contingencyType)
				spell_node.add_as_child(attachee, spell_level_ids[memSp.spell_level-1])
	return 0
	
def ContingencyPerform(attachee, args, evt_obj):	
	#Save the type of contingencySpellSelectionEnum
	contingencyType = evt_obj.d20a.data1
	args.set_param(2, contingencyType)
	
	args.set_param(4, evt_obj.d20a.spell_data.spell_enum)
	args.set_param(5, evt_obj.d20a.spell_data.spell_class)
	args.set_param(6, evt_obj.d20a.spell_data.get_spell_level())
	args.set_param(7, evt_obj.d20a.spell_data.inven_idx)
	#args.set_param(8, evt_obj.d20a.spell_data.get_metamagic_data())
	
	#move to a function..., create spell data with the 4 parameters that were saved
	
	spell_packet = tpdp.SpellPacket(attachee, evt_obj.d20a.spell_data)
	
	print evt_obj.d20a.spell_data.spell_enum
	
	print "Set The Type!!"
	
	#Count the spell as used up
	spell_packet.debit_spell()
	
	return 0

Contingency = PythonModifier("sp_contingency.py", 8)
Contingency.AddHook(ET_OnGetTooltip, EK_NONE, ContingencyTooltip, ())
Contingency.AddHook(ET_OnGetEffectTooltip, EK_NONE, ContingencyEffectTooltip, ())
Contingency.AddHook(ET_OnD20Query, EK_Q_Critter_Has_Spell_Active, ContingencyHasSpellActive, ())
Contingency.AddHook(ET_OnBuildRadialMenuEntry, EK_NONE, ContingencyRadial, ())
Contingency.AddHook(ET_OnD20PythonActionPerform, contingencySpellSelectionEnum, ContingencyPerform, ())
#Start Combat Signal (LegacyCombatSystem::StartCombat should send)
#Take Damage Signal
Contingency.AddSpellDispellCheckHook()
Contingency.AddSpellCountdownStandardHook()


