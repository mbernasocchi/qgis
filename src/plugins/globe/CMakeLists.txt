# set path to additional CMake modules
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

FIND_PACKAGE(OSG REQUIRED)
FIND_PACKAGE(OSGEARTH REQUIRED)
FIND_PACKAGE(Threads)

########################################################
# Files

SET (GLOBE_PLUGIN_SRCS
     globe_plugin.cpp
     qgsglobetilesource.cpp
     qgsglobeplugindialog.cpp
     qgsglobeinterface.cpp
     qgsglobevectorlayerproperties.cpp
     globefeatureidentify.cpp
     qgsglobefrustumhighlight.cpp
)

SET (GLOBE_PLUGIN_UIS
     qgsglobeplugindialog.ui
     qgsglobevectorlayerpropertiespage.ui
)

SET (GLOBE_PLUGIN_MOC_HDRS
     globe_plugin.h
     qgsglobeplugindialog.h
     qgsglobeinterface.h
     qgsglobevectorlayerproperties.h
)

SET (GLOBE_PLUGIN_HDRS
     qgsglobetilesource.h
     qgsosgearthfeaturesource.h
     qgsosgearthfeatureoptions.h
     qgsglobefeatureutils.h
     qgsglobeinterface.h
     qgsglobevectorlayerproperties.h
     globefeatureidentify.h
     qgsglobefrustumhighlight.h
)

SET (GLOBE_PLUGIN_RCCS  globe_plugin.qrc)

########################################################
# Build

IF(WIN32)
    ADD_DEFINITIONS("\"-DGLOBE_EXPORT=${DLLEXPORT}\"")
ELSE(WIN32)
    ADD_DEFINITIONS("-DGLOBE_EXPORT=")
ENDIF(WIN32)

QT4_WRAP_UI (GLOBE_PLUGIN_UIS_H  ${GLOBE_PLUGIN_UIS})

QT4_WRAP_CPP (GLOBE_PLUGIN_MOC_SRCS  ${GLOBE_PLUGIN_MOC_HDRS})

QT4_ADD_RESOURCES(GLOBE_PLUGIN_RCC_SRCS ${GLOBE_PLUGIN_RCCS})

ADD_LIBRARY (globeplugin SHARED ${GLOBE_PLUGIN_SRCS} ${GLOBE_PLUGIN_MOC_SRCS} ${GLOBE_PLUGIN_RCC_SRCS} ${GLOBE_PLUGIN_UIS_H} ${GLOBE_PLUGIN_HDRS})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${OSGEARTH_INCLUDE_DIR}
     ${OSG_INCLUDE_DIR}
     ${GEOS_INCLUDE_DIR}
     ${SIP_INCLUDE_DIR}
     ../../core ../../core/raster
     ../../gui
     ..
)

SET (OSGEARTH_LIBS
  ${OSGEARTH_LIBRARY}
  ${OSGEARTHFEATURES_LIBRARY}
  ${OSGEARTHUTIL_LIBRARY}
  ${OSGEARTHSYMBOLOGY_LIBRARY}
)
IF (HAVE_OSGEARTHQT)
    SET(OSGEARTH_LIBS
        ${OSGEARTH_LIBS}
        ${OSGEARTHQT_LIBRARY}
       )
ENDIF (HAVE_OSGEARTHQT)
TARGET_LINK_LIBRARIES(globeplugin
  qgis_core
  qgis_gui
  ${QT_QTOPENGL_LIBRARY}
  ${OSGDB_LIBRARY}
  ${OSGGA_LIBRARY}
  ${OSGUTIL_LIBRARY}
  ${OSG_LIBRARY}
  ${OSGQT_LIBRARY}
  ${OSGVIEWER_LIBRARY}
  ${OSGEARTH_LIBS}
  ${OPENTHREADS_LIBRARY}
  osgdb_osgearth_feature_qgis
)

###################################################
# osgEarth plugin
###################################################

SET(OSGEARTH_PLUGIN_PREFIX "")
IF(UNIX AND NOT WIN32 AND NOT APPLE)
  IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET(LIB_POSTFIX "64" CACHE STRING "suffix for 32/64 dir placement")
    MARK_AS_ADVANCED(LIB_POSTFIX)
  ENDIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE)
IF(NOT DEFINED LIB_POSTFIX)
  SET(LIB_POSTFIX "")
ENDIF(NOT DEFINED LIB_POSTFIX)
SET( DYNAMIC_OSGEARTH "ON" )
SET(OSGEARTH_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")

# SET(CMAKE_SHARED_MODULE_PREFIX ${OSGEARTH_PLUGIN_PREFIX})
SET(TARGET_DEFAULT_PREFIX "osgdb_")
SET(TARGET_DEFAULT_LABEL_PREFIX "Plugin")

INCLUDE( OsgEarthMacroUtils )

SET(TARGET_SRC
     qgsosgearthfeaturesource.cpp
)

SET(TARGET_H
    qgsosgearthfeatureoptions.h
)

SET(TARGET_COMMON_LIBRARIES ${TARGET_COMMON_LIBRARIES}
  qgis_core
  qgis_gui
#  ${QT_QTOPENGL_LIBRARY}
  ${OSGDB_LIBRARY}
#  ${OSGGA_LIBRARY}
#  ${OSGUTIL_LIBRARY}
  ${OSG_LIBRARY}
#  ${OSGQT_LIBRARY}
#  ${OSGVIEWER_LIBRARY}
  ${OSGEARTH_LIBRARY}
  ${OSGEARTHFEATURES_LIBRARY}
  ${OSGEARTHSYMBOLOGY_LIBRARY}
  ${OSGEARTHUTIL_LIBRARY}
  ${OPENTHREADS_LIBRARY}
)
SETUP_PLUGIN(osgearth_feature_qgis)

# to install public driver includes:
SET(LIB_NAME feature_qgis)
SET(LIB_PUBLIC_HEADERS ${TARGET_H})
INCLUDE(ModuleInstallOsgEarthDriverIncludes OPTIONAL)

ADD_SUBDIRECTORY(python)
########################################################
# Install

INSTALL(TARGETS globeplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})

INSTALL (FILES images/world.tif
         DESTINATION ${QGIS_DATA_DIR}/globe)

INSTALL (DIRECTORY images/gui
         DESTINATION ${QGIS_DATA_DIR}/globe)
