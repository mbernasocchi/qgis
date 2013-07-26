%Module(name=qgis.plugins.globe,
        version=0,
        keyword_arguments="Optional")

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import QtXml/QtXmlmod.sip

%Import core/core.sip

%Import osg/osg.sip
%Import osgearth/symbology/symbology.sip
%Import osgearth/features/features.sip

%Include qgsglobefeatureutils.sip
