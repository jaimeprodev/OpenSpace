#Version
1.0

#Module
Touch	local webGui = asset.require('util/webgui'); webGui.setCefRoute("ontouch")	openspace.printFatal('Could not load scene due to missing module "touch"')

#Asset
base	
scene/solarsystem/planets/earth/earth	earthAsset
util/webgui	

#Property
setPropertyValueSingle	Scene.Pluto.Renderable.Enabled	false
setPropertyValueSingle	Scene.Charon.Renderable.Enabled	false
setPropertyValueSingle	Scene.PlutoBarycenterTrail.Renderable.Enabled	false

#Time
relative	-1d

#Camera
setNavigationState	earthAsset.Earth.Identifier			58.5877,16.1924,20000000			

#MarkNodes
Earth
Mars
Moon
