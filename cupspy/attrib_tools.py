# Copyright (C) 2011  Free Software Foundation
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

"""Utilities for expanding attributes lists.
"""

# List of attributes to include in description

printer_description_atts = ["charset-configured",
                            "charset-supported",
                            "color-supported",
                            "compression-supported",
                            "document-format-default",
                            "document-format-supported",
                            "generated-natural-language-supported",
                            "ipp-versions-supported",
                            "job-impressions-supported",
                            "job-k-octets-supported",
                            "job-media-sheets-supported",
                            "multiple-document-jobs-supported",
                            "multiple-operation-time-out",
                            "natural-language-configured",
                            "notify-attributes-supported",
                            "notify-lease-duration-default",
                            "notify-lease-duration-supported",
                            "notify-max-events-supported",
                            "notify-events-default",
                            "notify-events-supported",
                            "notify-pull-method-supported",
                            "notify-schemes-supported",
                            "operations-supported",
                            "pages-per-minute",
                            "pages-per-minute-color",
                            "pdl-override-supported",
                            "printer-alert",
                            "printer-alert-description",
                            "printer-current-time",
                            "printer-driver-installer",
                            "printer-info",
                            "printer-is-accepting-jobs",
                            "printer-location",
                            "printer-make-and-model",
                            "printer-message-from-operator",
                            "printer-more-info",
                            "printer-more-info-manufacturer",
                            "printer-name",
                            "printer-state",
                            "printer-state-message",
                            "printer-state-reasons",
                            "printer-up-time",
                            "printer-uri-supported",
                            "queued-job-count",
                            "reference-uri-schemes-supported",
                            "uri-authentication-supported",
                            "uri-security-supported"]

# List of attributes if none asked

printer_default_atts = ["copies-default",
                        "document-format-default",
                        "finishings-default",
                        "job-hold-until-default",
                        "job-priority-default",
                        "job-sheets-default",
                        "media-default",
                        "number-up-default",
                        "orientation-requested-default",
                        "sides-default"]

# Extra stuff for when we want "all"

remaining_atts = ["auth-info-required",
	          "copies-supported",
		  "cups-version",
		  "device-uri",
		  "finishings-supported",
		  "job-creation-attributes-supported",
		  "job-hold-until-supported",
		  "job-k-limit",
		  "job-page-limit",
		  "job-priority-supported",
		  "job-quota-period",
		  "job-settable-attributes-supported",
		  "job-sheets-supported",
		  "marker-change-time",
		  "media-bottom-margin-supported",
		  "media-col-supported",
		  "media-left-margin-supported",
		  "media-right-margin-supported",
		  "media-source-supported",
		  "media-supported",
		  "media-top-margin-supported",
		  "multiple-document-handling-supported",
		  "number-up-supported",
		  "orientation-requested-supported",
		  "output-bin-default",
		  "output-bin-supported",
		  "page-ranges-supported",
		  "port-monitor",
		  "port-monitor-supported",
		  "printer-commands",
		  "printer-error-policy",
		  "printer-error-policy-supported",
		  "printer-is-shared",
		  "printer-op-policy",
		  "printer-op-policy-supported",
		  "printer-resolution-default",
		  "printer-resolution-supported",
		  "printer-settable-attributes-supported",
		  "printer-state-change-time",
		  "printer-type",
		  "print-quality-default",
		  "print-quality-supported",
		  "server-is-sharing-printers",
		  "sides-supported"]

def expand_multi_attributes(val):
    """Expand multiple attribute names

    Currently these are 'all', 'printer-description' and 'printer-defaults'.
    """

    return expand_attributes(get_attributes(val))

def expand_attributes(attribute_names):
    """Return attribute_names with any group names expanded to their members.

    The group names are 'all', 'printer-description' and 'printer-defaults'.
    Any other attribute names are included unchanged in the result.
    """
    expanded_attributes = []
    for name in attribute_names:
        expanded_attributes.extend(expand_attribute(name))
    return expanded_attributes

def expand_attribute(name):
    """Return the list of attribute names that attribute_name expands to.

    The group names are 'all', 'printer-description' and 'printer-defaults'.

    If it's not a group name, a singleton list containing attribute_name is
    returned, e.g.:
     >>> expand_attribute('copies-default')
     ['copies-default']
    """
    if name == 'printer-description':
        return printer_description_atts
    elif name == 'printer-defaults':
        return printer_default_atts
    elif name == 'all':
        return printer_description_atts + printer_default_atts + remaining_atts
    else:
        return [name]

def get_attributes(val):
    """Return the list of names of attributes requested.
    val: The value object containing the list of attributes.
    """
    return [v[1] for v in val.value]
