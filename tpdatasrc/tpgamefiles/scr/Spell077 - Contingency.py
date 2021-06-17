from toee import *
from utilities import *

def OnBeginSpellCast( spell ):
	print "Contingency OnBeginSpellCast"
	print "spell.target_list=", spell.target_list
	print "spell.caster=", spell.caster, " caster.level= ", spell.caster_level
	game.particles( "sp-evocation-conjure", spell.caster )

def	OnSpellEffect( spell ):
	print "Contingency OnSpellEffect"

	spell.duration = 14400 * spell.caster_level #One day per level
	target_item = spell.target_list[0]

	target_item.obj.condition_add_with_args( 'sp_contingency.py', spell.id, spell.duration, 0, 0, 0, 0, 0, 0)
	#target_item.partsys_id = game.particles( 'sp_contingency.py', target_item.obj )

def OnBeginRound( spell ):
	print "Contingency OnBeginRound"

def OnEndSpellCast( spell ):
	print "Contingency OnEndSpellCast"