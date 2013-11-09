/* xresmacro.h -- macros for X resources on Motif

   Copyright 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* These 4 don't do anything, they are just for the benefit of M4 */

#define XRES_COMMENT(s,n)
#define XRES_SPACE(n)
#define XRES_APPLICATION(n)
#define XRES_WIDGETOFFSETS(l,r,t,b)

#define XRES_LAYOUT(name, foreg, backg) name,xmPanedWindowWidgetClass
#define XRES_LISTWIDGET(name, foreg, backg, items)      name

/* Set up resources entries */

#define XRES_TYPE_BOOLEAN       XtRBoolean,sizeof(Boolean)
#define XRES_TYPE_INT           XtRInt,sizeof(int)
#define XRES_TYPE_STRING        XtRString,sizeof(String)

#define XRES_VALUE_BOOLEAN(val) XtRImmediate,(XtPointer)val
#define XRES_VALUE_INT(val)     XtRImmediate,(XtPointer)val
#define XRES_VALUE_STRING(val)  XtRString,val

#define XRES_RESOURCE(name, class, type, field, proginit, resinit)      { name, class, type, XtOffsetOf(vrec_t, field), proginit }

#define XRES_MENUSTART(name, foreg, backg, border)
#define XRES_MENUHDR(name, label, mnem)                 name
#define XRES_MENUITEM(name, label, mnem, acc, acctxt)   name
#define XRES_MENUEND()
#define XRES_TOOLSTART(name, foreg, backg, border)
#define XRES_TOOLBARITEM(name, label)                   name

#define XRES_STDDIALOG(name, title, foreg, backg)       name
#define XRES_DIALOG(name, title, foreg, backg)          name
#define XRES_LABELWIDGET(name, foreg, backg, label, align)      name
#define XRES_LABELGADGET(name, label)                   name,xmLabelGadgetClass
#define XRES_TOGGLEBUTTONGADGET(name, label)            name,xmToggleButtonGadgetClass
#define XRES_PUSHBUTTONWIDGET(name, label)              name
#define XRES_PUSHBUTTONGADGET(name, label)              name,xmPushButtonGadgetClass
