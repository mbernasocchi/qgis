%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip

%Import core/core.sip

%Import osgearth/symbology/symbology.sip
%Import osgearth/features/features.sip

%Import osg/osg/osg.sip
%Import osg/osgViewer/osgViewer.sip

%Module(name=qgis.globe,
        version=0,
        keyword_arguments="Optional")

%Include qgsglobefeatureutils.sip
%Include qgsglobeinterface.sip
