#! /usr/bin/python
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

import pygtk
import gtk
import gobject
import string
import re
import sys
import os
import pwd
import socket
import conf
import printeratts

def error_dlg(message):
    """Error message dialog"""
    dlg = gtk.MessageDialog(parent=None,
                            flags=gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT,
                            type=gtk.MESSAGE_ERROR,
                            buttons=gtk.BUTTONS_OK,
                            message_format=message)
    dlg.run()
    dlg.destroy()

ui_string = """<ui>
  <menubar name='Menubar'>
    <menu action='FileMenu'>
      <menuitem action='New'/>
      <menuitem action='Open'/>
      <separator/>
      <menuitem action='Save'/>
      <menuitem action='SaveAs'/>
      <separator/>
      <menuitem action='Close'/>
      <menuitem action='Quit'/>
    </menu>
    <menu action='ParamsMenu'>
      <menuitem action='Localaddr'/>
      <menuitem action='Log'/>
      <menuitem action='Directs'/>
      <menuitem action='Timeout'/>
      <separator/>
      <menuitem action='Defptr'/>
      <menuitem action='Defuser'/>
    </menu>
    <menu action='PtrMenu'>
      <menuitem action='Add'/>
      <menuitem action='Delete'/>
      <menuitem action='Update'/>
    </menu>
    <menu action='HelpMenu'>
      <menuitem action='About'/>
    </menu>
  </menubar>
  <toolbar name='Toolbar'>
    <toolitem action='New'/>
    <toolitem action='Open'/>
    <toolitem action='Save'/>
    <separator/>
    <toolitem action='Defptr'/>
    <separator/>
    <toolitem action='Add'/>
    <toolitem action='Delete'/>
    <toolitem action='Update'/>
    <separator/>
    <toolitem action='Quit'/>
  </toolbar>
</ui>"""

def setfilt(dlg):
    """Add filter to file chooser dialog"""
    filter = gtk.FileFilter()
    filter.set_name("All CUPSPY conf files")
    filter.add_pattern("*.conf")
    dlg.add_filter(filter)

def inserted_cb(treemodel, path, iter, w):
    """Callback for row inserted"""
    w.dirty = True

def deleted_cb(treemodel, path, w):
    """Callback for row deleted"""
    w.dirty = True

def activated_cb(view, path, col, win):
    """Callback for row double-clicked"""
    win.file_editptr_cb(path)

class Ptrdlg(gtk.Dialog):
    """New or edit printer dialog"""
    def __init__(self, title="Add printer"):
        gtk.Dialog.__init__(self, title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))

        tab = gtk.Table(6, 2)
        self.vbox.pack_start(tab)

        # Printer name

        tab.attach(gtk.Label("Emulated Printer name"), 0, 1, 0, 1, ypadding=5)
        self.cups_printer_name = gtk.Entry()
        tab.attach(self.cups_printer_name, 1, 2, 0, 1, ypadding=5)

        # Description

        tab.attach(gtk.Label("Description"), 0, 1, 1, 2, ypadding=5)
        self.description = gtk.Entry()
        tab.attach(self.description, 1, 2, 1, 2, ypadding=5)

        # Actual name

        tab.attach(gtk.Label("Actual Printer name"), 0, 1, 2, 3, ypadding=5)
        self.gs_printer_name = gtk.Entry()
        tab.attach(self.gs_printer_name, 1, 2, 2, 3, ypadding=5)

        # Form type

        tab.attach(gtk.Label("Form type"), 0, 1, 3, 4, ypadding=5)
        self.form_type = gtk.Entry()
        tab.attach(self.form_type, 1, 2, 3, 4, ypadding=5)

        # media supported/default

        tab.attach(gtk.Label("Media default"), 0, 1, 4, 5, ypadding=5)
        self.media_supp = gtk.combo_box_entry_new_text()
        tab.attach(self.media_supp, 1, 2, 4, 5, ypadding=5)

        # document format supported/default

        tab.attach(gtk.Label("Doc format default"), 0, 1, 5, 6, ypadding=5)
        self.doc_format = gtk.combo_box_entry_new_text()
        tab.attach(self.doc_format, 1, 2, 5, 6, ypadding=5)
        self.show_all()

    def setup_cbentry(self, field, config, attr, curr=""):
        """Set up combo box entry from default attributes"""
        mlist = config.get_default_attribute(attr)
        mlist.sort()
        for p in mlist:
            field.append_text(p)
        if len(curr) != 0:
            field.child.set_text(curr)

    def all_set(self):
        """Check all the fields are filled in"""
        if not re.search("^\w+$", self.cups_printer_name.get_text()):
            error_dlg("Invalid printer name")
            return False
        if not re.search("^\w*$", self.gs_printer_name.get_text()):
            error_dlg("Invalid GS Printer Name")
            return False
        if len(self.description.get_text()) == 0:
            error_dlg("No description")
            return False
        if not re.search("^[-\w.]+$", self.form_type.get_text()):
            error_dlg("Invalid form type")
            return False
        if len(self.media_supp.child.get_text()) == 0:
            error_dlg("No media supported")
            return False
        if len(self.doc_format.child.get_text()) == 0:
            error_dlg("No doc format")
            return False
        return True

class LocaddrDlg(gtk.Dialog):
    """Dialog box for setting local address"""
    def __init__(self, title="Set local address for URIs"):
        gtk.Dialog.__init__(self, title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
        self.locaddr = gtk.Entry()
        self.vbox.pack_start(self.locaddr)
        button = gtk.Button("Convert to/from IP")
        self.vbox.pack_start(button)
        button.connect('clicked', self.tofromip, self)
        self.show_all()

    def tofromip(self, p1, p2):
        """Reset box to IP from host name or vice versa"""
        current = self.locaddr.get_text()
        if len(current) == 0:
            error_dlg("No address yet")
            return
        if '0' <= current[0] <= '9':
            try:
                atup = socket.gethostbyaddr(current)
                repl = atup[0]
            except socket.herror:
                error_dlg("invalid IP - " + current)
                return
        else:
            try:
                repl = socket.gethostbyname(current)
            except socket.gaierror:
                error_dlg("unknown host - " + current)
                return
        self.locaddr.set_text(repl)

class DefUdlg(gtk.Dialog):
    """Dialog box for setting default user"""
    def __init__(self, title="Set default user name for jobs"):
        gtk.Dialog.__init__(self, title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
        self.userbox = gtk.combo_box_new_text()
        self.vbox.pack_start(self.userbox)
        self.show_all()

class LogDlg(gtk.Dialog):
    """Dialog box for setting logging level"""
    def __init__(self, title="Set logging level"):
        gtk.Dialog.__init__(self, title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
        self.loglevel = gtk.combo_box_new_text()
        for lev in ('Errors only', 'Warning unimplemented', 'Notices (null emulation)', 'Information', 'Debug'):
            self.loglevel.append_text(lev)
        self.vbox.pack_start(self.loglevel)
        self.show_all()

class ToDlg(gtk.Dialog):
    """Dialog box for setting timeout values"""
    def __init__(self, title="Set timeout value"):
        gtk.Dialog.__init__(self, title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT, (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
        self.timeout = gtk.SpinButton()
        self.timeout.set_numeric(True)
        self.timeout.set_digits(2)
        self.timeout.set_range(0.01, 1000.00)
        self.timeout.set_increments(0.01, 1.00)
        self.vbox.pack_start(self.timeout)
        self.show_all()

class Window(gtk.Window):
    """Main window class"""
    def __init__(self, filename=""):
        gtk.Window.__init__(self)
        self.set_position(gtk.WIN_POS_CENTER)
        self.set_title('Setup CUPSPY printers')
        self.connect('delete-event', self.delete_event_cb)
        self.set_size_request(550, 200)
        vbox = gtk.VBox()
        self.add(vbox)

        self.create_ui()
        vbox.pack_start(self.ui.get_widget('/Menubar'), expand=False)
        vbox.pack_start(self.ui.get_widget('/Toolbar'), expand=False)

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        vbox.pack_start(sw)

        self.tree_model = gtk.ListStore(gobject.TYPE_BOOLEAN, gobject.TYPE_STRING, gobject.TYPE_STRING)
        self.tree_model.connect('row-inserted', inserted_cb, self)
        self.tree_model.connect('row-deleted', deleted_cb, self)
        self.tree_view = gtk.TreeView(self.tree_model)
        self.tree_view.set_reorderable(True)
        self.tree_view.connect('row-activated', activated_cb, self)
        rend = gtk.CellRendererToggle()
        col = self.tree_view.insert_column_with_attributes(-1, "Default", rend, active=0)
        col.set_resizable(True)
        rend = gtk.CellRendererText()
        col = self.tree_view.insert_column_with_attributes(-1, "Name", rend, text=1)
        col.set_resizable(True)
        rend = gtk.CellRendererText()
        col = self.tree_view.insert_column_with_attributes(-1, "Description", rend, text=2)
        col.set_resizable(True)
        sw.add(self.tree_view)
        status = gtk.Statusbar()
        vbox.pack_end(status, expand=False)
        self.dirty = False
        self.has_data = False
        self.filename = ""
        self.config_data = conf.Conf()
        if len(filename) != 0:
            self.load_file(filename)
        else:
            self.config_data.setup_defaults()

    def load_file(self, filename):
        """Load up a config file"""

        self.config_data = conf.Conf()
        try:
            self.config_data.parse_conf_file(filename)
            self.filename = filename
            defptr = self.config_data.default_printer()
            plist = self.config_data.list_printers()
            self.tree_model.clear()
            for p in plist:
                isdef = p == defptr
                inf = self.config_data.get_attribute_value(p, 'printer-info')
                if not inf:
                    inf = ""
                self.tree_model.append((isdef, p, inf))
            self.dirty = False
            self.has_data = True
        except conf.ConfError as msg:
            error_dlg(msg.args[0])
            self.filename = ""
            self.config_data.setup_defaults()
            self.dirty = False
            self.has_data = False

    def check_dirty(self):
        """Check for changes before clobbering them"""
        if not self.dirty:
            return False
        dlg = gtk.MessageDialog(parent=None,
                                flags=gtk.DIALOG_MODAL|gtk.DIALOG_DESTROY_WITH_PARENT,
                                type=gtk.MESSAGE_QUESTION,
                                buttons=gtk.BUTTONS_YES_NO,
                                message_format="Unsaved data - continue?")
        resp = dlg.run()
        dlg.destroy()
        return resp != gtk.RESPONSE_YES;

    def check_data(self):
        """Check for data actually set before trying to save things"""
        if self.has_data:
            if len(self.config_data.default_printer()) == 0:
                error_dlg("No default printer")
                return False
            return True
        error_dlg("No printer information yet")
        return False

    def create_ui(self):
        """Set up menu"""
        ag = gtk.ActionGroup('WindowActions')
        actions = [
            ('FileMenu', None, '_File'),
            ('New',      gtk.STOCK_NEW, '_New', '<control>N', 'Create a new config file', self.file_new_cb),
            ('Open',     gtk.STOCK_OPEN, '_Open', '<control>O', 'Open a config file', self.file_open_cb),
            ('Save',     gtk.STOCK_SAVE, '_Save', '<control>S', 'Save a config file', self.file_save_cb),
            ('SaveAs',   gtk.STOCK_SAVE_AS, 'Save _As', '<shift><control>S', 'Save a config file in new file', self.file_saveas_cb),
            ('ParamsMenu', None, 'P_arameters'),
            ('Localaddr', None, 'Local _Address', '<shift>A', 'Set local address in URIs', self.locaddr_cb),
            ('Log',      None, '_Logging', None, 'Log file settings', self.par_log_cb),
            ('Directs',  None, '_Directory for PPD files', None, 'Configure directory for PPD files', self.par_direct_cb),
            ('Timeout',  None, '_Timeout', None, 'Timeout setting for socket', self.par_timeout_cb),
            ('Defptr',   gtk.STOCK_PRINT, 'Def _ptr', None, 'Set printer as default', self.par_defptr_cb),
            ('Defuser',  None, 'Default _User', '<shift>U', 'Set default user name', self.defuser_cb),
            ('PtrMenu',  None, '_Printers'),
            ('Add',      gtk.STOCK_GOTO_FIRST, '_Add', '<control>A', 'Add a new printer', self.file_newptr_cb),
            ('Delete',   gtk.STOCK_DELETE, '_Delete', 'Delete', 'Delete printer', self.file_delptr_cb),
            ('Update',   gtk.STOCK_EDIT, '_Update', '<control>E', 'Edit printer', self.file_editptr_cb),
            ('Close',    gtk.STOCK_CLOSE, '_Close', '<control>W', 'Close the current config file', self.file_close_cb),
            ('Quit',     gtk.STOCK_QUIT, '_Quit', '<control>Q', 'Quit program', self.file_quit_cb),
            ('HelpMenu', None, '_Help'),
            ('About',    None, '_About', None, 'About application', self.help_about_cb),
            ]
        ag.add_actions(actions)
        self.ui = gtk.UIManager()
        self.ui.insert_action_group(ag, 0)
        self.ui.add_ui_from_string(ui_string)
        self.add_accel_group(self.ui.get_accel_group())

    def get_plist(self):
        """Get list of printers in the order moved to"""
        result = []
        iter = self.tree_model.get_iter_first()
        while iter:
            result.append(self.tree_model.get_value(iter, 1))
            iter = self.tree_model.iter_next(iter)
        return result

    def file_new_cb(self, action):
        """Start new file of config data"""
        if self.check_dirty():
            return
        self.tree_model.clear()
        self.has_data = False
        self.dirty = False
        self.filename = ""
        self.config_data = conf.Conf()
        self.config_data.setup_defaults()

    def file_save_cb(self, action):
        """Save config data file"""
        if len(self.filename) == 0:
            self.file_saveas_cb(action)
            return
        if not self.check_data():
            return
        dlg = gtk.FileChooserDialog("Save..",
                                    None,
                                    gtk.FILE_CHOOSER_ACTION_SAVE,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
        dlg.set_default_response(gtk.RESPONSE_OK)
        dlg.set_filename(self.filename)
        setfilt(dlg)
        while dlg.run() == gtk.RESPONSE_OK:
            fn = dlg.get_filename()
            if fn is None:
                continue
            try:
                self.config_data.set_printer_list(self.get_plist())
                self.config_data.write_config(fn)
                self.filename = fn
                self.dirty = False
                break
            except conf.ConfError as msg:
                error_dlg(msg.args[0])
        dlg.destroy()

    def file_saveas_cb(self, action):
        """Save As routine for config data file"""
        if not self.check_data():
            return
        dlg = gtk.FileChooserDialog("Save As..",
                                    None,
                                    gtk.FILE_CHOOSER_ACTION_SAVE,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
        dlg.set_default_response(gtk.RESPONSE_OK)
        if len(self.filename) == 0:
            fn = os.getcwd()
            fn += '/cupspy.conf'
            dlg.set_filename(fn)
        else:
            dlg.set_filename(self.filename)
        setfilt(dlg)
        while dlg.run() == gtk.RESPONSE_OK:
            fn = dlg.get_filename()
            if fn is None:
                continue
            try:
                self.config_data.set_printer_list(self.get_plist())
                self.config_data.write_config(fn)
                self.filename = fn
                self.dirty = False
                break
            except conf.ConfError as msg:
                error_dlg(msg.args[0])
        dlg.destroy()

    def file_newptr_cb(self, action):
        """Add printer"""
        dlg = Ptrdlg()
        mdef = self.config_data.get_default_attribute('media-default')
        if isinstance(mdef,list) and len(mdef) == 1:
            mdef = mdef[0]
        else:
            mdef = ""
        dlg.setup_cbentry(dlg.media_supp, self.config_data, 'media-supported', mdef)
        dlg.setup_cbentry(dlg.doc_format, self.config_data, 'document-format-supported')
        while dlg.run() == gtk.RESPONSE_OK:
            if dlg.all_set():
                pname = dlg.cups_printer_name.get_text()
                try:
                    self.config_data.add_printer(pname)
                    inf = dlg.description.get_text()
                    self.config_data.set_attribute_value(pname, 'printer-info', inf)
                    self.config_data.set_attribute_value(pname, 'media-default', dlg.media_supp.child.get_text())
                    self.config_data.set_attribute_value(pname, 'document-format-default', dlg.doc_format.child.get_text())
                    self.config_data.set_param_value(pname, "Form", dlg.form_type.get_text())
                    self.config_data.set_param_value(pname, "GSPrinter", string.strip(dlg.gs_printer_name.get_text()))
                    self.tree_model.append((False, pname, inf))
                    self.has_data = True
                    break
                except conf.ConfError as msg:
                    error_dlg(msg.args[0])
                    continue
        dlg.destroy()

    def file_editptr_cb(self, action):
        """Edit printer"""
        model, sel = self.tree_view.get_selection().get_selected()
        if sel is None:
            return
        pname = model.get_value(sel, 1)
        dlg = Ptrdlg()
        dlg.cups_printer_name.set_text(pname)
        dlg.form_type.set_text(self.config_data.get_param_value(pname, 'Form'))
        gsp = self.config_data.get_param_value(pname, 'GSPrinter')
        if gsp == ':':
            gsp = ""
        dlg.gs_printer_name.set_text(gsp)
        dlg.setup_cbentry(dlg.media_supp, self.config_data, 'media-supported', self.config_data.get_attribute_value(pname, 'media-default'))
        dlg.setup_cbentry(dlg.doc_format, self.config_data, 'document-format-supported', self.config_data.get_attribute_value(pname, 'document-format-default'))
        dlg.description.set_text(self.config_data.get_attribute_value(pname, 'printer-info'))
        while dlg.run() == gtk.RESPONSE_OK:
            if dlg.all_set():
                newpname = dlg.cups_printer_name.get_text()
                inf = dlg.description.get_text()
                self.config_data.set_attribute_value(pname, 'printer-info', inf)
                self.config_data.set_attribute_value(pname, 'media-default', dlg.media_supp.child.get_text())
                self.config_data.set_attribute_value(pname, 'document-format-default', dlg.doc_format.child.get_text())
                self.config_data.set_param_value(pname, "Form", dlg.form_type.get_text())
                self.config_data.set_param_value(pname, "GSPrinter", string.strip(dlg.gs_printer_name.get_text()))
                try:
                    if newpname != pname:
                        self.config_data.rename_printer(pname, newpname)
                        self.tree_model.set_value(sel, 1, newpname)
                except conf.ConfError as msg:
                    error_dlg(msg.args[0])
                self.tree_model.set_value(sel, 2, inf)
                self.dirty = True
                break
        dlg.destroy()

    def file_delptr_cb(self, action):
        """Delete printer"""
        model, sel = self.tree_view.get_selection().get_selected()
        if sel is not None:
            pname = model.get_value(sel, 1)
            model.remove(sel)
            self.config_data.del_printer(pname)

    def defuser_cb(self, action):
        """Define default user"""
        dlg = DefUdlg()
        ul = [p.pw_name for p in pwd.getpwall()]
        ul.sort()
        for u in ul:
            dlg.userbox.append_text(u)
        try:
            ind = ul.index(self.config_data.default_user())
            dlg.userbox.set_active(ind)
        except ValueError:
            pass
        if dlg.run() == gtk.RESPONSE_OK:
            self.config_data.set_default_user(ul[dlg.userbox.get_active()])
            self.dirty = True
        dlg.destroy()

    def locaddr_cb(self, action):
        """Set local address"""
        dlg = LocaddrDlg()
        dlg.locaddr.set_text(self.config_data.serverip())
        while dlg.run() == gtk.RESPONSE_OK:
            ip = dlg.locaddr.get_text()
            if len(ip) == 0:
                error_dlg("No IP or host name given")
                continue
            self.config_data.set_serverip(ip)
            self.dirty = True
            break
        dlg.destroy()

    def par_log_cb(self, action):
        """Reset log parameter"""
        dialog = LogDlg()
        dialog.loglevel.set_active(self.config_data.log_level())
        if  dialog.run() == gtk.RESPONSE_OK:
            self.config_data.set_log_level(dialog.loglevel.get_active())
            self.dirty = True
        dialog.destroy()

    def par_timeout_cb(self, action):
        """Reset timeout parameter"""
        dialog = ToDlg()
        dialog.timeout.set_value(self.config_data.timeout_value())
        if  dialog.run() == gtk.RESPONSE_OK:
            self.config_data.set_timeout_value(dialog.timeout.get_value())
            self.dirty = True
        dialog.destroy()

    def par_direct_cb(self, action):
        """Set up PPD directory"""
        dialog = gtk.FileChooserDialog("PPD Directory", self,
                                       gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                       (gtk.STOCK_OPEN, gtk.RESPONSE_OK,
                                        gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
        dialog.set_default_response(gtk.RESPONSE_OK)
        dialog.set_current_folder(self.config_data.ppddir())
        if  dialog.run() == gtk.RESPONSE_OK:
            self.config_data.set_ppddir(dialog.get_current_folder())
            self.dirty = True
        dialog.destroy()

    def par_defptr_cb(self, action):
        """Set up default printer"""
        model, sel = self.tree_view.get_selection().get_selected()
        if sel is not None:
            pname = model.get_value(sel, 1)
            if pname != self.config_data.default_printer():
                self.config_data.set_default_printer(pname)
                iter = model.get_iter_first()
                while iter:
                    model.set_value(iter, 0, False)
                    iter = model.iter_next(iter)
                model.set_value(sel, 0, True)
                self.dirty = True

    def file_open_cb(self, action):
        """Open config file"""
        if self.check_dirty(): return
        dialog = gtk.FileChooserDialog("Open..", self,
                                       gtk.FILE_CHOOSER_ACTION_OPEN,
                                       (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                        gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)

        filter = gtk.FileFilter()
        filter.set_name("All CUPSPY conf files")
        filter.add_pattern("*.conf")
        dialog.add_filter(filter)
        if dialog.run() == gtk.RESPONSE_OK:
            self.load_file(dialog.get_filename())
        dialog.destroy()

    def file_close_cb(self, action):
        """Close program"""
        if not self.check_dirty():
            gtk.main_quit()

    def file_quit_cb(self, action):
        """Quit program"""
        if not self.check_dirty():
            raise SystemExit

    def help_about_cb(self, action):
        """About box"""
        dialog = gtk.MessageDialog(self, (gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT), gtk.MESSAGE_INFO, gtk.BUTTONS_OK, "CUPSPY setup Rel 1")
        dialog.run()
        dialog.destroy()

    def delete_event_cb(self, window, event):
        """Delete action"""
        if not self.check_dirty():
            gtk.main_quit()

if __name__ == '__main__':
    if len(sys.argv) > 1:
        w = Window(sys.argv[1])
    else:
        w = Window()
    w.show_all()
    gtk.main()
