//------------------------------------------------------------------------
//
// Remember we can't use "pop" or "push" because Gatesware doesn't
// recognise them.

//************************************************************************
//************************************************************************
//		Location of self
//		****************
//************************************************************************
//************************************************************************

Self_location = "/gsjsfunc.js";

//************************************************************************
//************************************************************************

//========================================================================
//
//  Standard stuff to refresh window on completion

var Compl_win, IntervalID;

function check_compl()  {
    if  (top.List.Compl_win.closed)  {
	top.List.window.clearInterval(top.List.IntervalID);
	top.List.document.location.reload();
    }
}

function compl_goback()  {
    if  (top.List.Compl_win.closed)  {
	top.List.window.clearInterval(top.List.IntervalID);
	top.List.document.location = urlnoarg(top.List.document.location);
    }
}

function timer_setup(w, fun, cchk)  {
    top.List.Compl_win = w;
    top.List.IntervalID = top.List.window.setInterval(fun? "compl_goback()": "check_compl()", cchk);
}

//========================================================================
//
//  Handy arithmetic functions
//
//------------------------------------------------------------------------

function rounddown(n, tobase)
{
    return  Math.floor(n / tobase) * tobase;
}

function roundup(n, tobase)
{
    return  Math.ceil(n / tobase) * tobase;
}

function modulo(n, base)
{
    return  n - rounddown(n, base);
}

function roundtomidday(dat) {
    return  rounddown(dat, 24 * 3600000) + 12 * 3600000;
}

//========================================================================
//
// Functions to manipulate URLs
//
//------------------------------------------------------------------------

function urldir(l)  {
    var hr = l.href;
    var ind = hr.lastIndexOf('/');
    return  ind >= 0? hr.substr(0, ind+1): hr;
}

function urlnoarg(l) {
    var hr = l.href, sr = l.search, ind;
    if  (sr != ""  &&  (ind = hr.lastIndexOf(sr)) >= 0)
	hr = hr.substr(0, ind);
    return  hr;
}

function urlandfirstarg(l)  {
    var hr = l.href;
    var ind = hr.indexOf('&');
    return  ind > 0?  hr.substr(0, ind):  hr;
}

function basename(pth)  {
    var broken = pth.split("/");
    return  broken[broken.length-1];
}

function nosuffix(fn)  {
    return  fn.replace(/\..*$/, "");
}

function is_remote(hr)  {
    var ind = hr.lastIndexOf('/');
    var st = ind >= 0? hr.substr(ind+1, 1): hr;
    return  st == "r";
}

function remote_vn(loc, prog)  {
    return is_remote(loc.href)? "r" + prog: prog;
}

function doc_makenewurl(doc, prog)
{
    var loc = doc.location;
    var result = urldir(loc) + remote_vn(loc, prog);
    var sstr = loc.search.substr(1);
    var aind = sstr.indexOf('&');
    if  (aind > 0)
	sstr = sstr.substr(0, aind);
    return  result + '?' + sstr;
}

function makenewurl(prog)
{
    return  doc_makenewurl(top.List.document, prog);
}

function esc_itemlist(itemlist)  {
    var  escitemlist = new Array();
    for  (var cnt = 0;  cnt < itemlist.length;  cnt++)
	escitemlist = escitemlist.concat(escape(itemlist[cnt]));
    return  '&' + escitemlist.join('&');
}

function removechars(str, ch)
{
    var  ind;

    while  ((ind = str.indexOf(ch)) >= 0)
	str = str.substr(0, ind) + str.substr(ind+1);
    return  str;
}

// De-htmlify text
function fixdescr(descr)  {
   return   removechars(removechars(descr, '<'), '>');
}

//========================================================================
//
//	Form creation routines
//
//========================================================================

function text_box(doc, name, sz, maxl)  {
    doc.writeln("<input type=text name=", name, " size=", sz, " maxlength=", maxl, ">");
}

function text_box_init(doc, name, sz, maxl, initv)  {
    doc.writeln("<input type=text name=", name, " size=", sz, " maxlength=", maxl, " value=\"", initv, "\">");
}

function checkbox_item(doc, name, descr)  {
    doc.writeln("<input type=checkbox name=", name, " value=1>", descr);
}

function checkbox_item_chk(doc, name, descr, chk)  {
    doc.write("<input type=checkbox name=", name, " value=1");
    if  (chk)
	doc.write(" checked");
    doc.writeln(">", descr);
}

function radio_button(doc, name, val, descr, isdef)  {
    doc.write("<input type=radio name=", name, " value=");
    if  (isnumeric(val))
	doc.write(val);
    else
	doc.write("\"", val, "\"");
    if  (isdef)
	doc.write(" checked");
    doc.writeln(">", descr);
}

function submit_button(doc, name, buttontxt, clickrout)  {
    doc.writeln("<input type=submit name=", name, " value=\"", buttontxt, "\" onclick=\"", clickrout, ";\">");
}

function action_buttons(doc, actbutttxt)  {
    doc.writeln("<input type=submit name=Action value=\"", actbutttxt, "\">");
    submit_button(doc, "Action", "Cancel", "window.close();return false");
}

function hidden_value(doc, name, value)  {
    doc.writeln("<input type=hidden name=", name, " value=\"", value, "\">");
}

//========================================================================
//
// Get standard bits and pieces from standard places
//
//========================================================================

function get_chgitemlist()  {
    return  esc_itemlist(document.js_func_form.js_itemlist.value.split(','));
}

function get_chgurl()  {
    return  document.js_func_form.js_url.value;
}

//========================================================================
//
// Cookie handling
//
//------------------------------------------------------------------------

function matchcookname(nam, str)
{
    var name = nam + '=', start = 0;
    while ((start = str.indexOf(name, start)) >= 0)  {
	if  (start > 0)  {
	    var  c = str.charAt(start-1);
	    if  ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))  {
		start += name.length;
		continue;
	    }
	}
	return  start + name.length;
    }
    return  -1;
}

function cookval(nam, str)
{
    var ind = matchcookname(nam, str);
    if  (ind < 0)
	return  0;
    var endp = str.indexOf(';', ind);
    return  str.substring(ind, endp < 0? str.length: endp);
}
    
// Get user code and cookies for current program name

function getcookies(pn)  {
    var allcookies = document.cookie;
    var result = new Object();
    if  (allcookies == "")
	return  result;
    var cv = cookval("uc", allcookies);
    if  (cv)
	result.usercode = parseInt(cv);
    cv = cookval(pn, allcookies);
    if  (cv)  {
	var varr = cv.split('&');
	for  (var j = 0;  j < varr.length;  j++)  {
	    var b = varr[j].split(':');
	    result[b[0]] = unescape(b[1]);
	}
    }
    return  result;
}

// Store cookies for program name.

function storecookie(name, val, days)  {
    var expdate = new Date((new Date()).getTime() + days*24*3600000);
    document.cookie = name + '=' + val + '; expires=' + expdate.toGMTString();
}

function storecookies(obj, pn, days) {
    if  (obj.usercode)
	storecookie("uc", obj.usercode, days);
    var carr = new Array();
    for  (var el in obj)  {
	if  (el == "usercode")
	    continue;
	carr = carr.concat(el + ':' + escape(obj[el]));
    }
    if  (carr.length == 0)
	carr = carr.concat("xxx:1");
    storecookie(pn, carr.join('&'), days);
}

//========================================================================
//
// Login type stuff
//
//------------------------------------------------------------------------

// Check for all blanks

function isblank(sv)  {
    var s = sv.value;	
    for (var i = 0;  i < s.length;  i++)  {
	var c = s.charAt(i);
	if  (c != ' '  &&  c != '\t'  &&  c != '\n')
	    return  false;
    }
    return  true;
}

function isnumeric(str)  {
    if  (str.length <= 0)
	return  false;
    for  (var i = 0;  i < str.length;  i++)  {
	var c = str.charAt(i);
	if  (c < '0' || c > '9')
	    return  false;
    }
    return  true;
}

function logincheck(f)  {
    if  (isblank(f.login))  {
	alert("You didn't give a login name");
	f.login.focus();
	return false;
    }
    if  (isblank(f.passwd))  {
	alert("You didn't give a password");
	f.passwd.focus();
	return false;
    }
    return  true;
}

function dologinform(prog)  {
    document.writeln("<script src=\"", Self_location, "\"></script>");
    document.writeln('<form name="loginform" method=post onSubmit="return logincheck(document.loginform);" action="',
		     prog, '?login">');
    document.writeln("<pre>");
    var ind = prog.lastIndexOf('/');
    var st = ind >= 0? prog.substr(ind+1, 1): prog;
    if  (st == "r")
	document.writeln('Host name:  <INPUT TYPE=TEXT NAME=desthost size=20 maxlength=70>');
    document.writeln('Login name: <INPUT TYPE=TEXT NAME=login size=20 maxlength=20>');
    document.writeln('Password:   <INPUT TYPE=PASSWORD NAME=passwd size=20 maxlength=20>');
    document.writeln("</pre>");
    document.writeln('<input type=submit name="LoginU" value="Login as User">\n</form>');
    //    document.loginform.login.focus();
}

function defloginopt(def, prog)  {
    if  (def)  {
	document.write('<br>If you want to log in as the default user ');
	document.writeln('<a href="', prog, '">click here</a>');
    }
}

//  Invoke program after we've got the relevant cookie list

function invokeprog(prog, cooklist)  {
    var aarr = new Array();
    aarr = aarr.concat(cooklist.usercode.toString());
    for (var arg in cooklist)  {
	if  (arg == "usercode" || arg == "xxx")
	    continue;
	aarr = aarr.concat(escape(arg + '=' + cooklist[arg]));
    }
    window.location = prog + '?' + aarr.join('&');
}

// If no login, see if we've got a usercode saved up from the last time.
// If not, do login form.

function ifnologin() {
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    if  (cooklist.usercode)
	invokeprog(prog, cooklist);
    else
	dologinform(prog);
}

function relogin()  {
    dologinform(urlnoarg(top.List.document.location));
    //return  false;
}

function invoke_opt_prog(optname)  {
    var prog = urlnoarg(top.List.document.location);
    var ucode = top.List.document.location.search;
    if  (ucode == "")  {
	alert("No search? Probable bug");
	return  false;
    }
    ucode = parseInt(ucode.substr(1));
    top.List.window.location = prog + '?' + ucode + '&' + optname;
    return  false;
}

//========================================================================
//
// View operations
//
//------------------------------------------------------------------------

function page_viewnext(n)  {
    var targpage = savepage + n;
    if  (targpage < 0)
	targpage = 1;
    else  if  (targpage > savenpages)
	targpage = savenpages;
    var loc = document.location;
    var prog = urlnoarg(loc), sstr = loc.search.substr(1);
    var aind = sstr.indexOf('&');
    if  (aind > 0)
	sstr = sstr.substr(0, aind);
    sstr += '&' + escape(savejobnum) + '&' + targpage;
    document.location = prog + '?' + sstr;
    return  false;
}

function page_viewheader(jn, jt, pg, np, sp, ep, hp, okmod) {
    viewpages = true;
    savejobnum = jn;
    savejobtit = jt;
    savepage = pg;
    savenpages = np;
    savestart = sp;
    saveend = ep;
    savehalt = hp;
    saveokmod = okmod;
    document.write("<table width=\"100%\"><tr><th align=left>");
    if (jt == "")
	document.write("Job number ", jn);
    else
	document.write("<I>", jt, "</I> (", jn, ")");
    document.write("</th><th align=right>Page ", pg, "/", np);
    if (pg == sp)
	document.write("\tStart page");
    if (pg == ep)
	document.write("\tEnd page");
    if (pg == hp)
	document.write("\tPrinting halted here");
    document.writeln("</th></tr></table><hr>");
}

function viewheader(jn, jt, okmod) {
    viewpages = false;
    savejobnum = jn;
    savejobtit = jt;
    saveokmod = okmod;
    document.write("<table width=\"100%\"><tr><th align=left>");
    if (jt == "")
	document.write("Job number ", jn);
    else
	document.write("<I>", jt, "</I> (", jn, ")");
    document.writeln("</th><th align=right>All</th></tr></table><hr>");
}

function set_pageto(argname, page, jn)  {
    document.location = doc_makenewurl(document, "sqccgi") + '&' + escape(argname + '=' + page) + '&' + escape(jn);
    return  false;
}

function view_pageaction(doc, descr, func)  {
    submit_button(doc, "viewaction", descr, func);
}

function view_changepage(doc, descr, func)  {
    view_pageaction(doc, descr, "return " + func);
}

function view_setchangepage(doc, descr, pn, spage)
{
    view_changepage(doc, descr, "set_pageto('" + pn + "', " + spage + ",'" + savejobnum + "')");
}

function viewfooter() {
    document.writeln("<hr>\n<form method=get>");
    if  (viewpages  &&  savepage > 1)  {
	view_changepage(document, "First page", "page_viewnext(-1000000)");
	if  (savepage > 2)
	    view_changepage(document, "Previous page", "page_viewnext(-1)");
    }
    view_pageaction(document, "Quit", "window.close()");
    if  (viewpages)  {
	if  (savepage < savenpages)  {
	    if  (savenpages - savepage > 1)
		view_changepage(document, "Next page", "page_viewnext(1)");
	    view_changepage(document, "Last page", "page_viewnext(1000000)");
	}
	if  (saveokmod)  {
	    if  (savepage != savestart)
		view_setchangepage(document, "Set start", "sp", savepage);
	    if  (savepage != saveend)
		view_setchangepage(document, "Set end", "ep", savepage);
	    if  (savehalt != 0)  {
		if  (savepage != savehalt)
		    view_setchangepage(document, "Set halted at", "hatp", savepage);
		view_setchangepage(document, "Reset halted at", "hatp", 0);
	    }
	}
    }
    document.writeln("</form>");
}

function dispvwin(url, jnum, pnum)
{
    var narg = '&' + escape(jnum);
    if  (pnum != "")
	    narg += '&' + escape(pnum);
    url += narg;
    var jnc = jnum, ind = 0;
    for (ind = jnc.indexOf(':', ind);  ind >= 0;  ind = jnc.indexOf(':', ind))
	jnc = jnc.substr(0, ind) + "_" + jnc.substr(ind+1);
    open(url, jnc, "status=yes,resizable=yes,scrollbars=yes");
}

//========================================================================
//
// Utility functions for "main screen"
//
//------------------------------------------------------------------------

function listclicked()  {
    var result = new Array(), els = top.List.document.listform.elements, nam = "";
    for (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "checkbox")  {
	    nam = tag.name;
	    if  (tag.checked)
		result = result.concat(tag);
	}
    }
    if  (result.length == 0)
	alert("No " + nam + " list items selected.\nPlease select some");
    return  result;
}

function clearboxes()  {
    var els = top.List.document.listform.elements;
    for  (var cnt = 0;  cnt < els.length; cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "checkbox")
	    tag.checked = false;
    }
}

//function gethidden(s)  {
//    var els = document.listform.elements;
//    for  (var cnt = 0;  cnt < els.length; cnt++)  {
//	var tag = els[cnt];
//	if  (tag.type == "hidden"  &&  tag.name == s)
//	    return  tag.value;
//    }
//    return  null;
//}

//========================================================================
//
// View functions
//
//------------------------------------------------------------------------

function viewclicked(prog)  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var jnumlist = new Array(), jpnumlist = new Array(), novplist = new Array(), norvlist = new Array();
    for (var elcnt = 0; elcnt < joblist.length; elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 3) != 3)  {
	    if  (!(perm & 1))
		novplist = novplist.concat(num);
	    if  (!(perm & 2))
		norvlist = norvlist.concat(num);
	}
	else  if  (perm & 0x8000)
		jpnumlist = jpnumlist.concat(num);
	else
		jnumlist = jnumlist.concat(num);
    }
    if  (novplist.length > 0  ||  norvlist.length > 0)  {
	var msg = "You do not have permisssion to view some of the jobs.\n";
	if  (novplist.length > 0)  {
	    msg += "\nFor the following you do not have view permission for\n";
	    msg += "other user's jobs.\n\n";
	    msg += novplist.join(", ");
	}
	if  (norvlist.length > 0)  {
	    msg += "\nFor the following you do not have view permission for\n";
	    msg += "jobs on other hosts.\n\n";
	    msg += norvlist.join(", ");
	}
	alert(msg);
    }

    var url = makenewurl(prog);

    var jpcnt, jp;
    for (jpcnt = 0;  jpcnt < jpnumlist.length;  jpcnt++)  {
	jp = jpnumlist[jpcnt];
	dispvwin(url, jp, "1");
    }
    for (jpcnt = 0;  jpcnt < jnumlist.length;  jpcnt++)  {
	jp = jnumlist[jpcnt];
	dispvwin(url, jp, "");
    }
}

//========================================================================
//
//  Delete functions
//
//------------------------------------------------------------------------

function delclicked()  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var nplist = new Array(), plist = new Array(), nodplist = new Array(), nordlist = new Array();
    for (var elcnt = 0; elcnt < joblist.length; elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 6) != 6)  {
	    if  (!(perm & 4))
		nodplist = nodplist.concat(num);
	    if  (!(perm & 2))
		nordlist = nordlist.concat(num);
	}
	else  if  (perm & 0x4000)
		plist = plist.concat(num);
	else
		nplist = nplist.concat(num);
    }
    if  (nodplist.length > 0  ||  nordlist.length > 0)  {
	var msg = "You do not have permisssion to delete some of the jobs.\n";
	if  (nodplist.length > 0)  {
	    msg += "\nFor the following you do not have delete permission for\n";
	    msg += "other user's jobs.\n\n";
	    msg += nodplist.join(", ");
	}
	if  (nordlist.length > 0)  {
	    msg += "\nFor the following you do not have delete permission for\n";
	    msg += "jobs on other hosts.\n\n";
	    msg += nordlist.join(", ");
	}
	alert(msg);
	return;
    }
    if  (nplist.length > 0)  {
	    var msg = "The following jobs have not been printed.\n\n";
	    msg += nplist.join(", ");
	    msg += "\n\nPlease confirm that they are to be deleted.";
	    if  (confirm(msg))
		    plist = plist.concat(nplist);
    }
    if  (plist.length == 0)
	return;
    vreswin(makenewurl("sqdcgi") + esc_itemlist(plist), 300, 400);
}

//========================================================================
//
//  Change utils, mostly form building
//
//------------------------------------------------------------------------

// Check that jobs are changable, and build an argument suitable
// for change forms

function make_changearg(jl, descr)  {
    var oklist = new Array(), nocplist = new Array(), norclist = new Array();
    for (var elcnt = 0; elcnt < jl.length; elcnt++)  {
	var el = jl[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 10) != 10)  {
	    if  (!(perm & 8))
		nocplist = nocplist.concat(num);
	    if  (!(perm & 2))
		norclist = norclist.concat(num);
	}
	else
		oklist = oklist.concat(num);
    }
    if  (nocplist.length > 0  ||  norclist.length > 0)  {
	var msg = "You do not have permisssion to change some of the " + descr + "s.\n";
	if  (nocplist.length > 0)  {
	    msg += "\nFor the following you do not have change permission for\n";
	    msg += "other user's " + descr + "s.\n\n";
	    msg += nocplist.join(", ");
	}
	if  (norclist.length > 0)  {
	    msg += "\nFor the following you do not have change permission for\n";
	    msg += descr + "s on other hosts.\n\n";
	    msg += norclist.join(", ");
	}
	alert(msg);
	return  "";
    }
    return  oklist.join(',');
}

function clearchanges()  {
    for  (var el in Changedfields)
	Changedfields[el] = false;
    return  true;
}

function makeclick(nam)  {
    return  function()  {  Changedfields[nam] = true;  }
}

function update_corr_text(form, name, val)  {
    var els = form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "text"  &&  tag.name == name)  {
	    tag.value = val;
	    Changedfields[name] = true;	// Do we need this???
	    return;
	}
    }
}

function update_select(name)  {
    var form = document.js_func_form;
    var els = form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "select-one"  &&  tag.name == name)  {
	    var ind = tag.name.lastIndexOf("_upd");
	    update_corr_text(form, tag.name.substr(0, ind), tag.options[tag.selectedIndex].value);
	    return;
	}
    }
}

function makeupd(nam)  {
    return  function()  { update_select(nam); }
}

function record_formchanges(form)  {
    Changedfields = new Object();
    var els = form.elements;
    for (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	switch  (tag.type)  {
	case  "checkbox":
	case  "radio":
//	    tag.onclick = new Closure(function()  { Changedfields[name] = true; }, tag);
	    tag.onclick = makeclick(tag.name);
	    break;
	case  "select-one":
	    if  (tag.name.lastIndexOf("_upd") > 0)  {
		tag.onchange = makeupd(tag.name);
		break;
	    }
	case  "text":
//	    tag.onchange = new Closure(function()  { Changedfields[name] = true; }, tag);
	    tag.onchange = makeclick(tag.name);
	    break;
	}
    }
}

function updboxsel(d, name, list)  {
    d.writeln("<select name=", name, "_upd size=1>");
    for  (var cnt = 0;  cnt < list.length;  cnt++)  {
	var fld = list[cnt];
	d.writeln("<option value=\"", fld, "\">", (fld == ""? "(none)": fld));
    }
    d.writeln("</select>");
}

function fnumsel(doc, nam, begin, end, step, deflt)
{
    doc.writeln("<select name=", nam, " size=1>");
    for  (var cnt = begin;  cnt <= end;  cnt += step)  {
	doc.write("<option");
	if  (cnt == deflt)
	    doc.write(" selected");
	doc.writeln(" value=", cnt, ">", Math.floor(cnt/step));
    }
    doc.writeln("</select>");
}

function fnumselect(doc,nam,begin,end,deflt)
{
    if  (end - begin > 1000)  {
	fnumsel(doc, nam + "_1000", rounddown(begin, 1000), rounddown(end, 1000), 1000, rounddown(deflt, 1000));
	fnumselect(doc, nam, 0, 999, modulo(deflt, 1000));
    }
    else  if  (end - begin > 100)  {
	fnumsel(doc, nam + "_100", rounddown(begin, 100), rounddown(end, 100), 100, rounddown(deflt, 100));
	fnumselect(doc, nam, 0, 99, modulo(deflt, 100));
    }
    else  if  (end - begin > 10)  {
	fnumsel(doc, nam + "_10", rounddown(begin, 10), rounddown(end, 10), 10, rounddown(deflt, 10));
	fnumselect(doc, nam, 0, 9, modulo(deflt, 10));
    }
    else
        fnumsel(doc, nam, begin, end, 1, deflt);
}

function hourtimeout(doc, nam, deflt)
{
    fnumselect(doc, nam + "_d", 0, Math.floor(32767/24), Math.floor(deflt/24));
    doc.write("days");
    fnumsel(doc, nam + "_h", 0, 23, 1, modulo(deflt, 24));
    doc.write("hours");
}

function timedelay(doc, nam, deflt)
{
    fnumselect(doc, nam + "_d", 0, 366, Math.floor(deflt/(24*3600)));
    doc.write("days");
    fnumselect(doc, nam + "_h", 0, 23, Math.floor(modulo(deflt, 24*3600)/3600));
    doc.write(":");
    fnumselect(doc, nam + "_m", 0, 59, Math.floor(modulo(deflt, 3600)/60));
    doc.write(":");
    fnumselect(doc, nam + "_s", 0, 59, modulo(deflt, 60));
}

function prtdate(dat)  {
    var dw = [ "Sun", "Mon", "Tues", "Wednes", "Thurs", "Fri", "Satur" ];
    var mn = [ "January", "February", "March", "April", "May", "June",
	     "July", "August", "September", "October", "November", "December" ];
    // Kludge to get round Bug in older versions of Netscape
    if  (dat.getTimezoneOffset() == -1440)
	return dw[dat.getUTCDay()] + "day " + dat.getUTCDate() + " " + mn[dat.getUTCMonth()];
    return dw[dat.getDay()] + "day " + dat.getDate() + " " + mn[dat.getMonth()];
}

function dateto(doc, deftim, nam, ndays, plussec)
{
    doc.writeln("<select name=", nam, "_d size=1>");
    var today = new Date();
    var todaymid = roundtomidday(today.getTime());
    var nday = new Date(deftim);
    var ndaymid = roundtomidday(deftim);
    for  (var cnt = 0;  cnt < ndays;  cnt++)  {
	doc.write("<option ");
	if  (todaymid == ndaymid)
	    doc.write("selected ");
	doc.writeln("value=", cnt, ">", prtdate(today));
	deftim += 3600000 * 24;
	nday.setTime(deftim);
	todaymid += 3600000 * 24;
	today.setTime(today.getTime() + 3600000 * 24);
    }
    doc.writeln("</select>");
    fnumselect(doc, nam + "_h", 0, 23, nday.getHours());
    doc.write(":");
    fnumselect(doc, nam + "_m", 0, 59, nday.getMinutes());
    if  (plussec)  {
	doc.write(":");
	fnumselect(doc, nam + "_s", 0, 59, nday.getSeconds());
    }
}

function nprefix(n)  {
    var ind = n.indexOf('_');
    return  ind >= 0? n.substr(0, ind): n;
}

function suffmult(n) {
    var ind = n.indexOf('_');
    if  (ind < 0)
	return  1;
    switch  (n.charAt(ind+1))  {
    default:
	return  1;
    case  'd':
	return  3600 * 24;
    case  'h':
	return  3600;
    case  'm':
	return  60;
    case  's':
	return  1;
    }
}

function listchanged(form)  {
    var cnames = new Array();
    for  (var el in Changedfields)
	if  (Changedfields[el])
	    cnames = cnames.concat(el);

    var resobj = new Object();
    if  (cnames.length == 0)
	return  resobj;

    var prefnobj = new Object();
    for  (var cnt = 0;  cnt < cnames.length;  cnt++)
	prefnobj[nprefix(cnames[cnt])] = true;

    var els = form.elements, nam, val;

    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	switch  (tag.type)  {
	default:
	    continue;
	case  "checkbox":
	    nam = nprefix(tag.name);
	    if  (!prefnobj[nam])
		continue;
	    if  (resobj[nam])  {
		if  (tag.checked)
		    resobj[nam] += parseInt(tag.value);
	    }
	    else
		resobj[nam] = tag.checked? parseInt(tag.value): 0;
	    break;
	case  "radio":
	    if  (!prefnobj[tag.name])
		continue;
	    if  (tag.checked)
		resobj[tag.name] = tag.value;
	    break;
	case  "text":
	    if  (!prefnobj[tag.name])
		continue;
	    resobj[tag.name] = tag.value;
	    break;
	case  "select-one":
	    if  (tag.name.lastIndexOf("_upd") >= 0)
		continue;
	    nam = nprefix(tag.name);
	    if  (!prefnobj[nam])
		continue;
	    val = tag.options[tag.selectedIndex].value * suffmult(tag.name);
	    if  (resobj[nam])
		resobj[nam] += val;
	    else
		resobj[nam] = val;
	    break;
	}
    }
    return  resobj;
}

//  May need to grab selections if a guy has said he wants it but hasn't
//  changed the values from defaults.

function getdefselected(form, nam)  {
    var els = form.elements, val = 0;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "select-one"  &&  nprefix(tag.name) == nam)
	    val += tag.options[tag.selectedIndex].value * suffmult(tag.name);
    }
    return  val;
}

//  Specific to Class codes - set values as per argument

function setclass(v)  {
    var els = document.js_func_form.elements;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var  tag = els[cnt];
	if  (tag.type == "checkbox"  &&  tag.name == "class")  {
	   if  (tag.value & v)
		tag.checked = true;
	   else
		tag.checked = false;
	}
    }
    Changedfields["class"] = true;
    return  false;
}

//  Write out class code checkboxes for document "d",
//  from 2^fs to 2^ts with letter given by "lett",
//  default checked given by bitmap "defc" and
//  override permission given by "co".
//  If "co" is not set make the checkboxes ineffective.

function putccodes(d, fs, ts, lett, defc, co)  {

    //  Want even spacing

    d.write("<tt>");
    for  (var shft = fs;  shft <= ts;  shft++, lett++)  {
	d.write("<input type=checkbox name=class value=");

	//  Alas Javascript can be buggy in that it doesn't
	//  notice 1 << 31 has integer overflowed and it
	//  puts it in with a - sign.

	var t = 1 << shft;
	var tstr = t.toString();
	if  (tstr.charAt(0) == '-')
		tstr = tstr.substr(1);

	if  (t & defc)		// Checked if default
	    d.write(tstr, " checked>");
	else  if  (co)		// Class override - allow setting
	    d.write(tstr, ">");		
	else			// No class override - no effect
	    d.write("0>");
	d.writeln(String.fromCharCode(lett));
    }
    d.write("</tt>");
}

function cc_bitsetup(d)  {
    var cover = parseInt(top.List.document.listform.privs.value) & 0x20;
    var defclass = parseInt(top.List.document.listform.classcode.value);
    tabrowleft(d, false);
    d.write("Class codes");
    nextcol(d);
    putccodes(d, 0, 7, 65, defclass, cover);

    tabrowleft(d, true);
    nextcol(d);
    putccodes(d, 8, 15, 73, defclass, cover);

    tabrowleft(d, true);
    nextcol(d);
    putccodes(d, 16, 23, 97, defclass, cover);

    tabrowleft(d, true);
    nextcol(d);
    putccodes(d, 24, 31, 105, defclass, cover);

    tabrowleft_cs(d, true, 2);
    if  (cover  &&  defclass != 0xffffffff)
	submit_button(d, "Setc", "Set max", "return setclass(0xffffffff)");
    submit_button(d, "Setc", "Set default", "return setclass(" + defclass + ")");
    submit_button(d, "Setc", "Clear all", "return setclass(0)");
    endrow(d);
}

// Wrap html construct such as "H1" round string.

function htmlwrite(ht, str)
{
    return  "<" + ht + ">" + str + "</" + ht + ">";
}

// Initialise change form with name "nam", title "tit", background colour "bgcol"
// header "hdr", width "wid" and height "ht".

function initformwnd(nam, tit, bgcol, hdr, wid, ht, cchk)  {
    var w = open("", nam, "width=" + wid + ",height=" + ht);
    w.document.writeln(htmlwrite("head", htmlwrite("title", tit)));
    if  (bgcol != "")
	w.document.writeln("<body bgcolor=\"", bgcol, "\">");
    else
	w.document.writeln("<body>");
    w.document.writeln("<script src=\"", Self_location, "\"></script>");
    w.document.write(htmlwrite("h1", hdr));
    timer_setup(w, false, cchk);
    return  w;
}

// Also stuff the word "Change" in from of title and header.

function chgformwnd(nam, tit, bgcol, hdr, wid, ht, cchk)  {
    return  initformwnd(nam, "Change " + tit, bgcol, "Change " + hdr, wid, ht, cchk);
}

// Return a suitable string to start a change form and table.

function chngformstart(d, cf)  {
    d.writeln("<form name=js_func_form method=get onSubmit=\"return ", cf, "();\">");
    d.writeln("<table>");
}

// Return a suitable string to end a change form and table.

function chngformend(doc, joblist, pn)  {
    endtable(doc);
    action_buttons(doc, "Make Changes");
    doc.writeln("<input type=reset onclick=\"return clearchanges();\">");
    hidden_value(doc, "js_itemlist", joblist);
    hidden_value(doc, "js_url", makenewurl(pn));
    doc.writeln("</form>\n<script>\nrecord_formchanges(document.js_func_form);\n</script>\n</body>");
}

function endrow(doc)
{
    doc.writeln("</td></tr>");
}

function endtable(doc)
{
    endrow(doc);
    doc.writeln("</table>");
}

function nextcol(doc)
{
    doc.write("</td><td>");
}

function nextcol_cs(doc, n)
{
    doc.write("</td><td colspan=" + n + ">");
}

function tabrowleft(doc, endlast)  {
    if  (endlast)
	endrow(doc);
    doc.writeln("<tr align=left><td>");
}

function tabrowleft_cs(doc, endlast, n)  {
    if  (endlast)
	endrow(doc);
    doc.writeln("<tr align=left><td colspan=" + n + ">");
}

function input_fnumselect(doc, descr, name, mn, mx, df)  {
    doc.writeln(descr);
    nextcol(doc);
    fnumselect(doc, name, mn,  mx, df);
}

function argpush(al, nam, val)  {
    return  al + '&' + escape(nam + '=' + val);
}

function vreswin(cmd, width, height)  {
    var w = open(cmd, "jsreswin", "width=" + width + ",height=" + height);
    timer_setup(w, false, 5000);
    return  false;
}

//========================================================================
//
//	Format changes
//
//------------------------------------------------------------------------

function do_formats()  {
    invoke_opt_prog('listformat');
}

// Lay down radio buttons for left/right/centre

function lf_align_ins(letter, deflt_al, albut, descr)
{
    radio_button(document, "al_" + letter, albut, descr, deflt_al == albut);
}

// Lay down a button to add format code "letter", align "align" and given
// description.

function format_button(letter, descr)  {
    document.write("<input type=button name=", letter, ' value="', fixdescr(descr),
		   '" onclick="format_click(document.fmtform.', letter, ');">\n');
}

function list_format_code(letter, align, descr)  {
    tabrowleft(document, false);
    format_button(letter, descr);
    nextcol(document);
    document.writeln("Align");
    lf_align_ins(letter, align, 'L', "Left");
    lf_align_ins(letter, align, 'C', "Centre");
    lf_align_ins(letter, align, 'R', "Right");
    endrow(document);
    Format_lookup[letter] = descr;
}

function existing_formats(def_fmt)  {
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var  ef = cooklist.format, hdr = "";
    if  (typeof ef != "string")  {
	ef = def_fmt;
	hdr = "(default) ";
    }
    document.writeln("</table><hr><h2>Previous ", hdr, "formats</h2><table>");
    var cnt = 0, lnum = 0;
    while  (lnum < ef.length)  {
	var al = ef.substr(lnum, 1);
	lnum++;
	if  (lnum >= ef.length)
	    break;
	if  (al != 'L'  &&  al != 'R'  &&  al != 'C')
	    continue;
	var fmt = ef.substr(lnum, 1);
	lnum++;
	var descr = Format_lookup[fmt];
	fmt += 'orig';
	cnt++;
	tabrowleft(document, false);
	document.write(cnt);
	nextcol(document);
	format_button(fmt, descr);
	nextcol(document);
	document.writeln("Align");
	lf_align_ins(fmt, al, 'L', "Left");
	lf_align_ins(fmt, al, 'C', "Centre");
	lf_align_ins(fmt, al, 'R', "Right");
	endrow(document);
    }
}

function find_alchecked(letter)
{
    var els = document.fmtform.elements;
    var allet = "al_" + letter;
    for  (var cnt = 0;  cnt < els.length;  cnt++)  {
	var tag = els[cnt];
	if  (tag.type == "radio" && tag.name == allet  &&  tag.checked)
	    return  tag.value;
    }
    return  "L";
}

// Clicking a button, remember which button and the alignment

function format_click(letterbutt)  {
    if  (Fmtbuttarray.length == 0)
	Fmtreswin = open("", "fmtbutts", "width=400,height=500,resizable=yes,scrollbars=yes");
    else
	Fmtreswin.document.open();	//  Should clear existing one
    var lett = letterbutt.name;		//  (May have suffix "orig" if from existing
    var algn = find_alchecked(lett);
    Fmtbuttarray = Fmtbuttarray.concat(algn + lett.substr(0,1) + letterbutt.value);

    //  Now display the result

    var doc = Fmtreswin.document;
    doc.writeln("<head><title>Display format codes</title></head><body><h1>Display codes</h1>");
    doc.writeln("<table><tr align=left><th>Code</th><th>Al</th><th>Descr</th></tr>");
    for  (var cnt = 0;  cnt < Fmtbuttarray.length;  cnt++)  {
	var ent = Fmtbuttarray[cnt];
	lett = ent.substr(1, 1);
	algn = ent.substr(0, 1);
	ent = ent.substr(2);
	tabrowleft(doc, false);
	doc.write(lett);
	nextcol(doc);
	doc.write(algn);
	nextcol(doc);
	doc.write(ent);
	endrow(doc);
    }
    doc.writeln("</table>");
    Fmtreswin.focus();
}

// Store away the formats set up

function setformats(ndays)  {
    if  (Fmtbuttarray.length == 0)  {
	alert("No formats selected yet");
	return  false;
    }
    Fmtreswin.close();
    var fmtstring = "";
    for  (var cnt = 0;  cnt < Fmtbuttarray.length;  cnt++)  {
	var  ent = Fmtbuttarray[cnt];
	fmtstring += ent.substr(0, 2);
    }
    Fmtbuttarray = new Array();
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    cooklist.format = fmtstring;
    storecookies(cooklist, pname, ndays);
    invokeprog(prog, cooklist);
    return  false;		// Not needed as invokeprog doesn't return - but....
}

// Cancel what we've done if anything.

function cancformats()  {
    if  (Fmtbuttarray.length != 0)  {
	Fmtreswin.close();
	Fmtbuttarray = new Array();
    }
    history.go(-1);
    return  false;
}

function resetformat(ndays)  {
    if  (Fmtbuttarray.length != 0)  {
	Fmtreswin.close();
	Fmtbuttarray = new Array();
    }
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var newcooks = new Object();
    var arr = new Array();
    for (var el in cooklist)  {
	if  (el != "format")
	    newcooks[el] = cooklist[el];
	arr.push(el);
    }
    storecookies(newcooks, pname, ndays);
    invokeprog(prog, newcooks);
    return  false;
}

//========================================================================
//
//	Option-setting
//
//------------------------------------------------------------------------

function do_options()  {
    invoke_opt_prog("listopts");
}

function optsave(ndays)  {
    var clist = listchanged(document.optform);
    var prog = urlnoarg(document.location);
    var pname = nosuffix(basename(document.location.pathname));
    var cooklist = getcookies(pname);
    var newcooks = new Object();
    if  (cooklist.usercode)
	newcooks.usercode = cooklist.usercode;
    if  (cooklist.format)
	newcooks.format = cooklist.format;
    for (var el in clist)
	newcooks[el] = clist[el];
    storecookies(newcooks, pname, ndays);
    invokeprog(prog, newcooks);
    return  false;
}

//========================================================================
//========================================================================
//
//	Change functions are product-specific
//
//------------------------------------------------------------------------
//------------------------------------------------------------------------

function gsj_formpri()  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;

    var jlstring = make_changearg(joblist, "job");
    if  (jlstring.length == 0)
        return;

    top.List.Jlstring = jlstring;
    var firstj = jlstring;
    var ind = firstj.indexOf(',');
    if  (ind > 0)
	firstj = firstj.substr(0, ind);

    var w = open(urlandfirstarg(top.List.document.location) + '&listprfm&prfm_preamble&gsj_formpri2&' + escape(firstj),
		 "gsjc", "width=480,height=360");
    timer_setup(w, false, 5000);
}

function gsj_formpri2(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist)
{
    var lwin = window.opener;
    var lwindoc = window.opener.document;
    var privs = parseInt(lwindoc.listform.privs.value);
    var jlstring = window.opener.Jlstring;

    chngformstart(document, "gschanges");
    tabrowleft(document, false);
    document.write("Job title");
    nextcol(document);
    text_box_init(document, "hdr", 20, 30, title);
    nextcol(document);
    checkbox_item_chk(document, "noh", "Suppress header", supph);
    tabrowleft(document, true);
    document.write("Form type");
    nextcol(document);
    if  (privs & 0x2)  {
	text_box_init(document, "form", 20, 34, defform);
	nextcol(document);
	updboxsel(document, "form", formlist);
    }
    else
	document.write("<i>(No change permission)</i>");
    tabrowleft(document, true);
    document.write("Destination");
    nextcol(document);
    if  (privs & 0x4)  {
	text_box_init(document, "ptr", 20, 58, defptr);
	nextcol(document);
	updboxsel(document, "ptr", ptrlist);
    }
    else
	document.write("<i>(No change permission)</i>");

    var maxcps = parseInt(lwindoc.listform.copies.value);
    if  (privs & 0x10)
	    maxcps = 255;
    tabrowleft(document, true);
    input_fnumselect(document, "Copies", "cps", 0, maxcps, defcps > maxcps? maxcps: defcps);
    endrow(document);
    if  (privs & 0x8)  {
	    var plist = lwindoc.listform.prio.value;
	    var minp = 100, maxp = 200, defp = 150, ind = plist.indexOf(',');
	    if  (ind >= 0)  {
		    defp = parseInt(plist);
		    plist = plist.substr(ind+1);
		    ind = plist.indexOf(',');
		    if  (ind >= 0)  {
			    minp = parseInt(plist);
			    plist = plist.substr(ind+1);
			    maxp = parseInt(plist);
		    }
	    }
	    if  (privs & 0x10)  {
		    minp = 1;
		    maxp = 255;
	    }
	    tabrowleft(document, false);
	    input_fnumselect(document, "Priority", "pri", minp, maxp, defpri < minp || defpri > maxp? defp: defpri);
	    endrow(document);
    }

    document.writeln("</table>");
    action_buttons(document, "Make Changes");
    document.writeln("<input type=reset onclick=\"return clearchanges();\">");
    hidden_value(document, "js_itemlist", jlstring);
    hidden_value(document, "js_url", doc_makenewurl(lwindoc, "sqccgi"));
    document.writeln("</form>\n<script>\nrecord_formchanges(document.js_func_form);\n</script>\n</body>");
}

function gsj_pages()  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;

    var jlstring = make_changearg(joblist, "job");
    if  (jlstring.length == 0)
        return;

    var w = chgformwnd("gsjc",
		       "GNUspool Page options",
		       "#FFFFCF",
		       "Job page options",
		       460, 360, 5000);

    var d = w.document;

    chngformstart(d, "gschanges");
    tabrowleft(d, false);
    radio_button(d, "oddp", "all", "All pages", true);
    tabrowleft(d, true);
    radio_button(d, "oddp", "odd", "Odd pages only", false);
    tabrowleft(d, true);
    radio_button(d, "oddp", "even", "Even pages only", false);
    tabrowleft(d, true);
    radio_button(d, "oddp", "odd_revoe", "Odd then even pages", false);
    tabrowleft(d, true);
    radio_button(d, "oddp", "even_revoe", "Even then odd pages", false);
    tabrowleft(d, true);
    d.writeln("Post-processing options");
    nextcol(d);
    text_box(d, "flags", 20, 62);
    chngformend(d, jlstring, "sqccgi");
}

function gsj_class()  {
    var joblist = listclicked();

    if  (joblist.length == 0)
	return;

    var jlstring = make_changearg(joblist, "job");
    if  (jlstring.length == 0)
        return;

    var w = chgformwnd("gsjc",
		       "GNUspool Job Class Codes",
		       "#FFFFCF",
		       "class codes",
		       500, 500, 5000);

    var d = w.document;
    chngformstart(d, "gschanges");
    cc_bitsetup(d);
    tabrowleft(d, false);
    d.writeln("Post to user");
    nextcol(d);
    text_box(d, "puser", 11, 11);
    tabrowleft(d, true);
    checkbox_item(d, "mail", "Mail Results");
    nextcol(d);
    checkbox_item(d, "wrt", "Write Results");
    tabrowleft(d, true);
    checkbox_item(d, "mattn", "Mail Attention");
    nextcol(d);
    checkbox_item(d, "wattn", "Write Attention");
    tabrowleft(d, true);
    checkbox_item(d, "loco", "Local only");
    chngformend(d, jlstring, "sqccgi");
}

function gsj_retain()  {
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var jlstring = make_changearg(joblist, "job");
    if  (jlstring.length == 0)
        return;

    var w = chgformwnd("gsjc",
		       "GNUspool Job Retention",
		       "#FFFFCF",
		       "Job retention",
		       750, 450, 5000);

    var d = w.document;

    chngformstart(d, "gschanges");
    tabrowleft(d, false);
    d.writeln("Printed timeout");
    nextcol(d);
    hourtimeout(d, "pto", 24);
    tabrowleft(d, true);
    d.writeln("Unprinted timeout");
    nextcol(d);
    hourtimeout(d, "npto", 168);
    tabrowleft_cs(d, true, 2);
    checkbox_item(d, "retn", "Retain on queue after printing");
    tabrowleft_cs(d, true, 2);
    checkbox_item(d, "printed", "Printed (or pretend it has been)");
    tabrowleft(d, true);
    radio_button(d, "hold", "n", "No hold", true);
    tabrowleft(d, true);
    radio_button(d, "hold", "holdfor", "Hold for", false);
    nextcol(d);
    timedelay(d, "holdfor", 3600);
    tabrowleft(d, true);
    radio_button(d, "hold", "holdto", "Hold until", false);
    nextcol(d);
    var todnow = new Date();
    dateto(d, todnow.getTime() + 3600000, "holdto", 14, true);
    chngformend(d, jlstring, "sqccgi");
}

function gschanges()  {
    var clist = listchanged(document.js_func_form);
    var arglist = "", errs = 0;
    var nn, today, tim;
    for  (var el in clist)
	switch  (el)  {
	default:
	    alert("Forgotten about arg type " + el);
	    continue;
	case  "hdr":
	case  "form":
	case  "ptr":
	case  "flags":
	case  "puser":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "pri":
	    if  (clist[el] <= 0)  {
	        alert("Priority cannot be 0 or less");
		errs++;
	        break;
	    }
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "cps":
	    if  (clist[el] > 255)  {
	        alert("Copies or priority cannot be > 255");
		errs++;
	        break;
	    }
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "pto":
	case  "npto":
	    nn = Math.floor(clist[el] / 3600);
	    if  (nn  > 32767)  {
	        alert("Timeout times cannot be > 32767 hours");
		errs++;
		break;
	    }
	    arglist = argpush(arglist, el, nn);
	    break;
	case  "holdfor":
	case  "holdto":
	    continue;
	case  "hold":
	    switch  (clist[el])  {
	    default:
		arglist = argpush(arglist, "hold", 0);
		continue;
	    case  "holdfor":
		nn = clist[clist[el]]? clist[clist[el]]: getdefselected(document.js_func_form, clist[el]);
		if  (nn <= 0)  {
		    arglist = argpush(arglist, "hold", 0);
		    continue;
		}
		today = new Date();
		tim = Math.round((today.getTime() + nn * 1000) / 1000);
		break;
	    case  "holdto":
		nn = clist[clist[el]]? clist[clist[el]]: getdefselected(document.js_func_form, clist[el]);
		if  (nn <= 0)  {
		    arglist = argpush(arglist, "hold", 0);
		    continue;
		}
		today = new Date();
		tim = Math.floor((today.getTime() / (24 * 3600000))) * 24 * 3600 + nn;
		break;
	    }
	    arglist = argpush(arglist, "hold", tim);
	    break;
	case  "class":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "noh":
	case  "mail":
	case  "wrt":
	case  "mattn":
	case  "wattn":
	case  "loco":
	case  "retn":
	case  "printed":
	    arglist = argpush(arglist, el, clist[el]? "y": "n");
	    break;
	case  "oddp":
	    switch  (clist[el])  {
	    default:
	    case  "all":
		arglist = argpush(arglist, "oddp", "n");
		arglist = argpush(arglist, "evenp", "n");
		arglist = argpush(arglist, "revoe", "n");
		break;
	    case  "odd":
		arglist = argpush(arglist, "oddp", "n");
		arglist = argpush(arglist, "evenp", "y");
		arglist = argpush(arglist, "revoe", "n");
		break;
	    case  "even":
		arglist = argpush(arglist, "oddp", "y");
		arglist = argpush(arglist, "evenp", "n");
		arglist = argpush(arglist, "revoe", "n");
		break;
	    case  "odd_revoe":
		arglist = argpush(arglist, "oddp", "n");
		arglist = argpush(arglist, "evenp", "y");
		arglist = argpush(arglist, "revoe", "y");
		break;
	    case  "even_revoe":
		arglist = argpush(arglist, "oddp", "y");
		arglist = argpush(arglist, "evenp", "n");
		arglist = argpush(arglist, "revoe", "y");
		break;
	    }
	    break;
	}
    if  (arglist.length == 0  ||  errs > 0)  {
	alert("No changes made");
	return  false;
    }
    document.location.replace(get_chgurl() + arglist + get_chgitemlist());
    return  false;
}

//========================================================================
//
//	Create GNUspool Jobs
//
//========================================================================

// Create from scratch

function gsj_createinit1()  {
    var w = open(urlandfirstarg(top.List.document.location) + '&listprfm&crinit_preamble&gsj_createinit2',
		 "gsjc", "width=550,height=400");
    timer_setup(w, false, 5000);
}

function gsj_createrest(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist)
{
    var lwin = window.opener;
    var lwindoc = window.opener.document;
    var privs = parseInt(lwindoc.listform.privs.value);
    document.writeln("<table>");
    tabrowleft(document, false);
    document.write("Job title");
    nextcol(document);
    text_box_init(document, "hdr", 20, 30, title);
    nextcol(document);
    checkbox_item_chk(document, "noh", "Suppress header", supph);
    tabrowleft(document, true);
    document.write("Form type");
    nextcol(document);
    text_box_init(document, "form", 20, 34, defform);
    nextcol(document);
    updboxsel(document, "form", formlist);
    tabrowleft(document, true);
    document.write("Destination");
    nextcol(document);
    text_box_init(document, "ptr", 20, 58, defptr);
    nextcol(document);
    updboxsel(document, "ptr", ptrlist);
    var maxcps = parseInt(lwindoc.listform.copies.value);
    if  (privs & 0x10)
	    maxcps = 255;
    tabrowleft(document, true);
    input_fnumselect(document, "Copies (0 to hold)", "cps", 0, maxcps, defcps > maxcps? maxcps: defcps);
    endrow(document);
    var plist = lwindoc.listform.prio.value;
    var minp = 100, maxp = 200, defp = 150, ind = plist.indexOf(',');
    if  (ind >= 0)  {
	defp = parseInt(plist);
	plist = plist.substr(ind+1);
	ind = plist.indexOf(',');
	if  (ind >= 0)  {
	    minp = parseInt(plist);
	    plist = plist.substr(ind+1);
	    maxp = parseInt(plist);
	}
    }
    tabrowleft(document, false);
    input_fnumselect(document, "Priority", "pri", minp, maxp, defpri);
    endtable(document);
    document.writeln("<input type=file name=jobfile size=50>");
    action_buttons(document, "Submit Job");
    document.writeln("<input type=reset onclick=\"return clearchanges();\">");
    document.writeln("</form>\n<script>\nrecord_formchanges(document.js_func_form);\n</script>\n</body>");
}

function gsj_createinit2(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist)
{
    document.writeln("<form name=js_func_form method=post enctype=\"multipart/form-data\" action=\"",
		     doc_makenewurl(window.opener.document, "sqcrcgi"), "\">");
    gsj_createrest(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist);
}

// Create as clone of existing job

function gsj_createclone1()
{
    var joblist = listclicked();
    if  (joblist.length == 0)
	return;
    var firstj = joblist[0].value;
    var ind = firstj.indexOf(',');
    if  (ind > 0)
	firstj = firstj.substr(0, ind);
    top.List.Jlstring = firstj;
    var w = open(urlandfirstarg(top.List.document.location) + '&listprfm&crclone_preamble&gsj_createclone2&' + escape(firstj),
		 "gsjc", "width=480,height=400");
    timer_setup(w, false, 5000);
}

function gsj_createclone2(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist)
{
    document.writeln("<form name=js_func_form method=post enctype=\"multipart/form-data\" action=\"",
		     doc_makenewurl(window.opener.document, "sqcrcgi"),
		     '&', escape(window.opener.Jlstring), "\">");
    gsj_createrest(title, supph, defcps, defpri, defform, defptr, formlist, ptrlist);
}

//========================================================================
//
//	GNUspool Printers
//
//========================================================================

function gsp_action(code)  {
    var ptrlist = listclicked();
    if  (ptrlist.length == 0)
	return;
    var proclist = new Array(), noproclist = new Array(), nopermlist = new Array(), noremplist = new Array();
    for  (var elcnt = 0;  elcnt < ptrlist.length;  elcnt++)  {
	var el = ptrlist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & 10) != 10)  {
	    if  (!(perm & 8))
		noremplist = noremplist.concat(num);
	    if  (!(perm & 2))
		nopermlist = nopermlist.concat(num);
	}
	else if  (perm & 0x2000)
	    proclist = proclist.concat(num);
	else
	    noproclist = noproclist.concat(num);
    }
    if  (nopermlist.length > 0  ||  noremplist.length > 0)  {
	var msg = "You do not have permisssion to change some of the " + descr + "s.\n";
	if  (nopermlist.length > 0)  {
	    msg += "\nFor the following you do not have access permission for\n";
	    msg += "the printers.\n\n";
	    msg += nopermlist.join(", ");
	}
	if  (noremplist.length > 0)  {
	    msg += "\nFor the following you do not have permission to access\n";
	    msg += "printers on other hosts.\n\n";
	    msg += noremplist.join(", ");
	}
	alert(msg);
	return;
    }
    if  (code == "start")  {
	if  (proclist.length > 0)  {
	    var msg = "The following printer(s) are already running.\n\n";
	    msg += proclist.join(", ");
	    alert(msg);
	    return;
	}
	proclist = noproclist;
    }
    else  if  (noproclist.length > 0)  {
	var msg = "The following printer(s) are not running.\n\n";
	msg += noproclist.join(", ");
	alert(msg);
	return;
    }

    var url = makenewurl("spccgi") + '&' + code + esc_itemlist(proclist);
    var w = open(url, "Action", "resizable=yes,height=400,width=300");
    timer_setup(w, false, 5000);
}

function somerunning(pl)  {
    var proclist = new Array();
    for  (var elcnt = 0;  elcnt < pl.length;  elcnt++)  {
	var el = pl[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var num = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  (perm & 0x2000)
	    proclist = proclist.concat(num);
    }
    if  (proclist.length > 0)  {
	var msg = "The following printer(s) are running.\nPlease stop them first.\n\n";
	msg += proclist.join(", ");
	alert(msg);
	return  true;
    }
    return  false;
}

function gsp_formdev()  {
    var ptrlist = listclicked();
    if  (ptrlist.length == 0)
	return;
    if  (somerunning(ptrlist))
	return;
    var plstring = make_changearg(ptrlist, "printer");
    if  (plstring.length == 0)
        return;

    top.List.Plstring = plstring;
    var firstp = plstring;
    var ind = firstp.indexOf(',');
    if  (ind > 0)
	firstp = firstp.substr(0, ind);

    var w = open(urlandfirstarg(top.List.document.location) + '&listprfm&prfm_preamble&gsp_formdev2&' + escape(firstp),
		 "gspc", "width=480,height=360");
    timer_setup(w, false, 5000);
}

function gsp_formdev2(pname, pform, pdev, pdescr, formlist)
{
    var lwin = window.opener;
    var lwindoc = window.opener.document;
    var privs = parseInt(lwindoc.listform.privs.value);
    var plstring = window.opener.Plstring;

    chngformstart(document, "gspchanges");
    tabrowleft(document, false);
    document.write("Printer:");
    nextcol(document);
    document.write(pname);
    tabrowleft(document, true);
    document.write("Form type");
    nextcol(document);
    text_box_init(document, "form", 20, 34, pform);
    nextcol(document);
    updboxsel(document, "form", formlist);
    tabrowleft(document, true);
    document.write("Device");
    nextcol(document);
    text_box_init(document, "dev", 20, 29, pdev);
    tabrowleft(document, true);
    document.write("Description");
    nextcol(document);
    text_box_init(document, "descr", 20, 41, pdescr);
    endrow(document);
    document.writeln("</table>");
    action_buttons(document, "Make Changes");
    document.writeln("<input type=reset onclick=\"return clearchanges();\">");
    hidden_value(document, "js_itemlist", plstring);
    hidden_value(document, "js_url", doc_makenewurl(lwindoc, "spccgi"));
    document.writeln("</form>\n<script>\nrecord_formchanges(document.js_func_form);\n</script>\n</body>");
}

function gsp_class()  {
    var ptrlist = listclicked();
    if  (ptrlist.length == 0)
	return;
    if  (somerunning(ptrlist))
	return;
    var plstring = make_changearg(ptrlist, "printer");
    if  (plstring.length == 0)
        return;

    var w = chgformwnd("gspc",
		       "GNUspool Printer Class Codes",
		       "#FFCFFF",
		       "class codes",
		       500, 400, 5000);

    var d = w.document;
    chngformstart(d, "gspchanges");
    cc_bitsetup(d);
    tabrowleft(d, true);
    checkbox_item(d, "loco", "Local only");
    chngformend(d, plstring, "spccgi");
}

function gspchanges()  {
    var clist = listchanged(document.js_func_form);
    var arglist = "", errs = 0;
    for  (var el in clist)
	switch  (el)  {
	default:
	    alert("Forgotten about arg type " + el);
	    continue;
	case  "form":
	case  "dev":
	case  "descr":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "class":
	    arglist = argpush(arglist, el, clist[el]);
	    break;
	case  "network":
	case  "loco":
	    arglist = argpush(arglist, el, clist[el]? "y": "n");
	    break;
	}
    if  (arglist.length == 0  ||  errs > 0)  {
	alert("No changes made");
	return  false;
    }
    document.location.replace(get_chgurl() + arglist + get_chgitemlist());
    return  false;
}

function gsp_joblist()  {
    top.Butts.location = "/gsjobbutts.html";
    document.location = urldir(document.location) + remote_vn(document.location, "sqcgi");
}

function gsj_viewptrs()  {
    top.Butts.location = "/gsptrbutts.html";
    document.location = urldir(document.location) + remote_vn(document.location, "spcgi");
}

//========================================================================
//========================================================================
//
//	Batch - util fns
//
//------------------------------------------------------------------------
//------------------------------------------------------------------------

function vnameok(n, notrem)  {
    var s = n.value;	
    for (var i = 0;  i < s.length;  i++)  {
	var c = s.charAt(i);
	if  ((c < 'A' || c > 'Z')  &&
	     (c < 'a' || c > 'z')  &&
	     c != '_'  &&  (notrem  ||  c != ':')  &&
	     (i == 0 || c < '0' || c > '9'))
	    return  false;
    }
    return  true;
}

function batchlist(descr, permbits, notrun, oneonly)  {
    var  joblist = listclicked();
    if  (joblist.length == 0)
	return  joblist;
    var oklist = new Array(), noklist = new Array(), runlist = new Array(), notrunlist = new Array();
    for  (var elcnt = 0;  elcnt < joblist.length;  elcnt++)  {
	var el = joblist[elcnt];
	var val = el.value;
	var ind = val.indexOf(',');
	if  (ind <= 0)
	    continue;
	var jnum = val.substr(0, ind), perm = parseInt(val.substr(ind+1));
	if  ((perm & permbits) == permbits)  {
	    if  (perm & 0x2000)  {
		if  (notrun)
		    runlist = runlist.concat(jnum);
		else
		    oklist = oklist.concat(jnum);
	    }
	    else  {
		if  (notrun)
		    oklist = oklist.concat(jnum);
		else
		    notrunlist = notrunlist.concat(jnum);
	    }
	}
	else
	    noklist = noklist.concat(jnum);
    }
    if  (runlist.length > 0  ||  notrunlist.length > 0  ||  noklist.length > 0)  {
	if  (runlist.length > 0)
	    alert("The following jobs are still running so you cannot " + descr + ".\n" + runlist.join(", "));
	if  (notrunlist.length > 0)
	    alert("The following jobs are not running so you cannot " + descr + ".\n" + notrunlist.join(", "));
	if  (noklist.length > 0)
	    alert("The following jobs do not have the right permission, you cannot " + descr + ".\n" + noklist.join(", "));
	return  new Array();
    }
    if  (oneonly  &&  oklist.length > 1)  {
	alert("You can only select one job at a time for this operation");
	return  new Array();
    }
    return  oklist;
}
    
function jobops(code, descr, permbits, notrun)  {
    var  joblist = batchlist(descr, permbits, notrun, false);
    if  (joblist.length == 0)
	return  false;
    return  vreswin(makenewurl("btjdcgi") + "&" + code + esc_itemlist(joblist), 460, 230);
}

function  permbox(ucode, code, checked)  {
    nextcol(document);
    document.write("<input type=checkbox name=", ucode, code, " value=1");
    if  (checked)
	    document.write(" checked");
    document.writeln(">");
}

// Local Variables:
// mode: java
// cperl-indent-level: 4
// End:
