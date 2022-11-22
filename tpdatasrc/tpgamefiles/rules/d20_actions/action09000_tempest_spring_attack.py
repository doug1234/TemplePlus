from toee import *
import tpactions

def GetActionName():
	return "Tempest Spring Attack"

def GetActionDefinitionFlags():
	return D20ADF_TargetSingleExcSelf | D20ADF_Breaks_Concentration | D20ADF_TriggersCombat | D20ADF_UseCursorForPicking

def GetTargetingClassification():
	return D20TC_SingleExcSelf

def GetActionCostType():
	return D20ACT_NULL

def AddToSequence(d20action, action_seq, tb_status):

	if d20action.performer.d20_query(Q_Prone):
		d20aGetup = d20action
		d20aGetup.action_type = D20A_STAND_UP
		action_seq.add_action(d20aGetup)

	action_seq.add_action(d20action)
	return AEC_OK

