# Copyright (C) 2009  Free Software Foundation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

# Originally written by John Collins <jmc@xisl.com>.

import string, conf

# These are a load of default parameters for CUPSPY
# We don't include them in conf.py or invoke it from there as we don't want to carry around all the
# baggage when cupspy is running.

Default_command = dict(Prefix='gspl-pr -s --verbose', Copies='-c %d', Title='-h %s', User='-U %s', Form='-f %s', GSPrinter='-f %s')

Default_attrs = [ ('printer-current-time', 'IPP_TAG_DATE', 'NOW'),
                  ('printer-error-policy', 'IPP_TAG_NAME', 'stop-printer'),
                  ('printer-is-accepting-jobs', 'IPP_TAG_BOOLEAN', 1),
                  ('printer-is-shared', 'IPP_TAG_BOOLEAN', 1),
                  ('printer-op-policy', 'IPP_TAG_NAME', 'default'),
                  ('printer-state', 'IPP_TAG_ENUM', 3),
                  ('printer-state-change-time', 'IPP_TAG_INTEGER', 'NOW-3600'),
                  ('printer-state-message', 'IPP_TAG_TEXT', ""),
                  ('printer-state-reasons', 'IPP_TAG_KEYWORD', 'none'),
                  ('printer-type', 'IPP_TAG_ENUM', 135244),
                  ('printer-up-time', 'IPP_TAG_INTEGER', 'NOW-7200'),
                  ('printer-uri-supported', 'IPP_TAG_URI', 'MAKELOCALURI'),
                  ('queued-job-count', 'IPP_TAG_INTEGER', 0),
                  ('uri-authentication-supported', 'IPP_TAG_KEYWORD', 'requesting-user-name'),
                  ('uri-security-supported', 'IPP_TAG_KEYWORD', 'none'),
                  ('printer-name', 'IPP_TAG_NAME', 'PRINTERNAME'),
                  ('printer-location', 'IPP_TAG_TEXT', "Local Printer"),
                  ('printer-info', 'IPP_TAG_TEXT', ""),
                  ('printer-more-info', 'IPP_TAG_URI', 'MAKENETURI'),
                  ('job-quota-period', 'IPP_TAG_INTEGER', 0),
                  ('job-k-limit', 'IPP_TAG_INTEGER', 0),
                  ('job-page-limit', 'IPP_TAG_INTEGER', 0),
                  ('job-sheets-default', 'IPP_TAG_NAME', 'none'),
                  ('device-uri', 'IPP_TAG_URI', 'MAKEDEVURI'),
                  ('color-supported', 'IPP_TAG_BOOLEAN', 1),
                  ('pages-per-minute', 'IPP_TAG_INTEGER', 12),
                  ('printer-make-and-model', 'IPP_TAG_TEXT', ""),
                  ('media-supported', 'IPP_TAG_KEYWORD', ['A4',
                                                          'BrA4_B',
                                                          'Letter',
                                                          'BrLetter_B',
                                                          'Legal',
                                                          'Executive',
                                                          'B5',
                                                          'A5',
                                                          'A6',
                                                          'BrA6_B',
                                                          'PostC4x6',
                                                          'BrPostC4x6_B',
                                                          'IndexC5x8',
                                                          'BrIndexC5x8_B',
                                                          'PhotoL',
                                                          'BrPhotoL_B',
                                                          'Photo2L',
                                                          'BrPhoto2L_B',
                                                          'PostCard',
                                                          'BrHagaki_B',
                                                          'DoublePostCardRotated',
                                                          'EnvC5',
                                                          'EnvDL',
                                                          'Env10',
                                                          'EnvMonarch',
                                                          'EnvYou4']),
                  ('media-default', 'IPP_TAG_KEYWORD', 'A4'),
                  ('port-monitor', 'IPP_TAG_KEYWORD', 'none'),
                  ('port-monitor-supported', 'IPP_TAG_KEYWORD', 'none'),
                  ('finishings-supported', 'IPP_TAG_ENUM', 3),
                  ('finishings-default', 'IPP_TAG_ENUM', 3),
                  ('document-format-supported', 'IPP_TAG_MIMETYPE', ['application/octet-stream',
                                                                     'application/pdf',
                                                                     'application/postscript',
                                                                     'application/vnd.cups-postscript',
                                                                     'application/vnd.cups-raw',
                                                                     'application/vnd.hp-hpgl',
                                                                     'application/x-cshell',
                                                                     'application/x-csource',
                                                                     'application/x-perl',
                                                                     'application/x-shell',
                                                                     'image/gif',
                                                                     'image/jpeg',
                                                                     'image/png',
                                                                     'image/tiff',
                                                                     'image/x-bitmap',
                                                                     'image/x-photocd',
                                                                     'image/x-portable-anymap',
                                                                     'image/x-portable-bitmap',
                                                                     'image/x-portable-graymap',
                                                                     'image/x-portable-pixmap',
                                                                     'image/x-sgi-rgb',
                                                                     'image/x-sun-raster',
                                                                     'image/x-xbitmap',
                                                                     'image/x-xpixmap',
                                                                     'text/html',
                                                                     'text/plain']),
                  ('copies-default', 'IPP_TAG_INTEGER', 1),
                  ('job-hold-until-default', 'IPP_TAG_KEYWORD', 'no-hold'),
                  ('job-priority-default', 'IPP_TAG_INTEGER', 50),
                  ('number-up-default', 'IPP_TAG_INTEGER', 1),
                  ('orientation-requested-default', 'IPP_TAG_ENUM', 3),
                  ('charset-configured', 'IPP_TAG_CHARSET', 'utf-8'),
                  ('charset-supported', 'IPP_TAG_CHARSET', ['us-ascii', 'utf-8']),
                  ('compression-supported', 'IPP_TAG_KEYWORD', ['none', 'gzip']),
                  ('copies-supported', 'IPP_TAG_RANGE', [1, 100]),
                  ('document-format-default', 'IPP_TAG_MIMETYPE', 'application/octet-stream'),
                  ('generated-natural-language-supported', 'IPP_TAG_LANGUAGE', 'en-us'),
                  ('ipp-versions-supported', 'IPP_TAG_KEYWORD', [1.0, 1.1]),
                  ('job-hold-until-supported', 'IPP_TAG_KEYWORD', ['no-hold',
                                                                  'indefinite',
                                                                  'day-time',
                                                                  'evening',
                                                                  'night',
                                                                  'second-shift',
                                                                  'third-shift',
                                                                  'weekend']),
                  ('job-priority-supported', 'IPP_TAG_INTEGER', 100),
                  ('job-sheets-supported', 'IPP_TAG_NAME', ['none',
                                                            'classified',
                                                            'confidential',
                                                            'mls',
                                                            'secret',
                                                            'selinux',
                                                            'standard',
                                                            'te',
                                                            'topsecret',
                                                            'unclassified']),
                  ('multiple-document-handling-supported', 'IPP_TAG_KEYWORD', [ 'separate-documents-uncollated-copies',
                                                                                'separate-documents-collated-copies' ]),
                  ('multiple-document-jobs-supported', 'IPP_TAG_BOOLEAN', 0),
                  ('multiple-operation-time-out', 'IPP_TAG_INTEGER', 60),
                  ('natural-language-configured', 'IPP_TAG_LANGUAGE', 'en-us'),
                  ('notify-attributes-supported', 'IPP_TAG_KEYWORD', ['printer-state-change-time',
                                                                      'notify-lease-expiration-time',
                                                                      'notify-subscriber-user-name']),
                  ('notify-lease-duration-default', 'IPP_TAG_INTEGER', 86400),
                  ('notify-lease-duration-supported', 'IPP_TAG_RANGE', [0, 2147483647]),
                  ('notify-max-events-supported', 'IPP_TAG_INTEGER', 100),
                  ('notify-events-default', 'IPP_TAG_KEYWORD', 'job-completed'),
                  ('notify-events-supported', 'IPP_TAG_KEYWORD', ['job-completed',
                                                                  'job-config-changed',
                                                                  'job-created',
                                                                  'job-progress',
                                                                  'job-state-changed',
                                                                  'job-stopped',
                                                                  'printer-added',
                                                                  'printer-changed',
                                                                  'printer-config-changed',
                                                                  'printer-deleted',
                                                                  'printer-finishings-changed',
                                                                  'printer-media-changed',
                                                                  'printer-modified',
                                                                  'printer-restarted',
                                                                  'printer-shutdown',
                                                                  'printer-state-changed',
                                                                  'printer-stopped',
                                                                  'server-audit',
                                                                  'server-restarted',
                                                                  'server-started',
                                                                  'server-stopped']),
                  ('notify-pull-method-supported', 'IPP_TAG_KEYWORD', 'ippget'),
                  ('notify-schemes-supported', 'IPP_TAG_KEYWORD', ['mailto', 'testnotify']),
                  ('number-up-supported', 'IPP_TAG_INTEGER', [1, 2, 4, 6, 9, 16]),
                  ('operations-supported', 'IPP_TAG_ENUM', [2, 4, 5, 6, 8, 9, 10,
                                                            11, 12, 13, 16, 17, 18,
                                                            20, 22, 23, 24, 25, 26,
                                                            27, 28, 34, 35, 16385,
                                                            16386, 16387, 16388, 16389,
                                                            16390, 16391, 16392, 16393,
                                                            16394, 16395, 16396, 16397, 16398]),
                  ('orientation-requested-supported', 'IPP_TAG_ENUM', [3, 4, 5, 6]),
                  ('page-ranges-supported', 'IPP_TAG_BOOLEAN', 1),
                  ('pdl-override-supported', 'IPP_TAG_KEYWORD', 'not-attempted'),
                  ('printer-error-policy-supported', 'IPP_TAG_NAME', ['abort-job', 'retry-job', 'stop-printer']),
                  ('printer-op-policy-supported', 'IPP_TAG_NAME', 'default')]


def init_defaults(confstr):
    """Initialise default parameters from above tables"""

    for name,val in dict.items(Default_command):
        confstr.defaults.defs[name] = val

    for attritem in Default_attrs:
        name, type, vals = attritem
        if isinstance(vals, list):
            vals = string.join(map(str,vals), ' ')
        if type == 'IPP_TAG_TEXT':
            vals = '"' + vals + '"'
        confstr.defaults.setattr(name, type, vals)
