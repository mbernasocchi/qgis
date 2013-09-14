# -*- coding: utf-8 -*-
#
#   Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
#    All rights reserved.
#
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the following conditions are met:
#        * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#        * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#        * Neither the name of the  Simon Edwards <simon@simonzone.com> nor the
#        names of its contributors may be used to endorse or promote products
#        derived from this software without specific prior written permission.
#
#    THIS SOFTWARE IS PROVIDED BY Simon Edwards <simon@simonzone.com> ''AS IS'' AND ANY
#    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#    DISCLAIMED. IN NO EVENT SHALL Simon Edwards <simon@simonzone.com> BE LIABLE FOR ANY
#    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# FindPyQt.py
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


pyqtcfg = {
'pyqt_bin_dir':      '/home/marco/dev/android-python27_host/output/bin',
'pyqt_config_args':  '--destdir /home/marco/dev/android-python27_host/output/lib/python2.7/site-packages --bindir /home/marco/dev/android-python27_host/output/bin --sipdir /home/marco/dev/android-python27_host/output/share/sip --confirm-license',
'pyqt_mod_dir':      '/home/marco/dev/android-python27_host/output/lib/python2.7/site-packages/PyQt4',
'pyqt_modules':      'QtCore QtGui QtNetwork QtDeclarative QtOpenGL QtScript QtScriptTools QtSql QtSvg QtTest QtWebKit QtXml QtXmlPatterns QtDesigner',
'pyqt_sip_dir':      '/home/marco/dev/android-python27_host/output/share/sip',
'pyqt_sip_flags':    '-x VendorID -t WS_X11 -x PyQt_NoPrintRangeBug -t Qt_4_7_0 -x Py_v3 -g',
'pyqt_version':      0x040800,
'pyqt_version_str':  '4.8',
'qt_data_dir':       '/usr/share/qt4',
'qt_dir':            '/usr',
'qt_edition':        'free',
'qt_framework':      0,
'qt_inc_dir':        '/usr/include/qt4',
'qt_lib_dir':        '/usr/lib/x86_64-linux-gnu',
'qt_threaded':       1,
'qt_version':        0x040803,
'qt_winconfig':      'shared'}

print("pyqt_version:%06.0x" % pyqtcfg['pyqt_version'])
print("pyqt_version_num:%d" % pyqtcfg['pyqt_version'])
print("pyqt_version_str:%s" % pyqtcfg['pyqt_version_str'])

pyqt_version_tag = ""
in_t = False
for item in pyqtcfg['pyqt_sip_flags'].split(' '):
    if item=="-t":
        in_t = True
    elif in_t:
        if item.startswith("Qt_4"):
            pyqt_version_tag = item
    else:
        in_t = False
print("pyqt_version_tag:%s" % pyqt_version_tag)

print("pyqt_mod_dir:%s" % pyqtcfg['pyqt_mod_dir'])
print("pyqt_sip_dir:%s" % pyqtcfg['pyqt_sip_dir'])
print("pyqt_sip_flags:%s" % pyqtcfg['pyqt_sip_flags'])
print("pyqt_bin_dir:%s" % pyqtcfg['pyqt_bin_dir'])
