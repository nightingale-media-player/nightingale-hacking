<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<META NAME="GENERATOR" CONTENT="lxr-1998-06-10: http://lxr.linux.no">
<title>seamonkey mozilla/build/autoconf/altoptions.m4</title>
<link rel='stylesheet' title='' href='http://mxr.mozilla.org/seamonkey/style/style.css' type='text/css'>
</head>

<BODY   BGCOLOR="#FFFFFF" TEXT="#000000"
	LINK="#0000EE" VLINK="#551A8B" ALINK="#FF0000">

<TABLE class=banner BGCOLOR="#000000" WIDTH="100%" BORDER=0 CELLPADDING=0 CELLSPACING=0>
<TR><TD><A HREF="http://www.mozilla.org/"><IMG
 SRC="http://www.mozilla.org/images/mozilla-banner.gif" ALT=""
 BORDER=0 WIDTH=600 HEIGHT=58></A></TD></TR></TABLE>

<TABLE class=header BORDER=0 CELLPADDING=12 CELLSPACING=0 WIDTH="100%">
 <TR>
  <TD ALIGN=LEFT VALIGN=MIDDLE>
   <nobr><font size="+2"><b><a href="http://mxr.mozilla.org">Mozilla Cross Reference</a></b>
</nobr><i><a href="http://mxr.mozilla.org/seamonkey">seamonkey</a></i>
</font>
   <BR><B><a href="/seamonkey/source/">mozilla</a>/ <a href="/seamonkey/source/build/">build</a>/ <a href="/seamonkey/source/build/autoconf/">autoconf</a>/ <a href="/seamonkey/source/build/autoconf/altoptions.m4">altoptions.m4</a> </B>
  </TD>

  <TD ALIGN=RIGHT VALIGN=TOP WIDTH="1%">
   <TABLE BORDER CELLPADDING=12 CELLSPACING=0>
    <TR>
     <TD NOWRAP BGCOLOR="#FAFAFA">

      <A HREF="http://bonsai.mozilla.org/cvslog.cgi?file=/mozilla/build/autoconf/altoptions.m4&amp;rev=HEAD&amp;mark=1.15">CVS Log</A><BR>
      <A HREF="http://bonsai.mozilla.org/cvsblame.cgi?file=/mozilla/build/autoconf/altoptions.m4&amp;rev=1.15">CVS Blame</A><BR>
      <A HREF="http://bonsai.mozilla.org/cvsgraph.cgi?file=/mozilla/build/autoconf/altoptions.m4&amp;rev=1.15">CVS Graph</A><BR>


<!--

      <A HREF="http://error.trac-not-found.tld/ /log//altoptions.m4">SVN Log</A><BR>
      <A HREF="http://error.trac-not-found.tld/ /browser//altoptions.m4">SVN View</A><BR>

-->

      <A HREF="http://mxr.mozilla.org/seamonkey/source/build/autoconf/altoptions.m4?raw=1">Raw file</A><BR>
     </TD>
    </TR>
   </TABLE>
  </TD>


  <TD ALIGN=RIGHT VALIGN=TOP WIDTH="1%">
   <TABLE BORDER CELLPADDING=6 CELLSPACING=0>
    <TR>
     <TD BGCOLOR="#FAFAFA">
      <TABLE BORDER=0 CELLPADDING=6 CELLSPACING=0>
       <TR>
        <TD NOWRAP ALIGN=LEFT>
         changes to<BR>this file in<BR>the last:
        </TD>
        <TD NOWRAP>
         <A HREF="http://bonsai.mozilla.org/cvsquery.cgi?branch=HEAD&amp;file=/mozilla/build/autoconf/altoptions.m4&amp;date=day">day</A><BR>
         <A HREF="http://bonsai.mozilla.org/cvsquery.cgi?branch=HEAD&amp;file=/mozilla/build/autoconf/altoptions.m4&amp;date=week">week</A><BR>
         <A HREF="http://bonsai.mozilla.org/cvsquery.cgi?branch=HEAD&amp;file=/mozilla/build/autoconf/altoptions.m4&amp;date=month">month</A><BR>
        </TD>
       </TR>
      </TABLE>
     </TD>
    </TR>
   </TABLE>
  </TD>


 </TR>
</TABLE>
<pre lang='en'><span class=line>   <a name=1 href="/seamonkey/source/build/autoconf/altoptions.m4#1">1</a> </span>dnl ***** BEGIN LICENSE BLOCK *****
<span class=line>   <a name=2 href="/seamonkey/source/build/autoconf/altoptions.m4#2">2</a> </span>dnl Version: MPL 1.1/GPL 2.0/LGPL 2.1
<span class=line>   <a name=3 href="/seamonkey/source/build/autoconf/altoptions.m4#3">3</a> </span>dnl
<span class=line>   <a name=4 href="/seamonkey/source/build/autoconf/altoptions.m4#4">4</a> </span>dnl The contents of this file are subject to the Mozilla Public License Version
<span class=line>   <a name=5 href="/seamonkey/source/build/autoconf/altoptions.m4#5">5</a> </span>dnl 1.1 (the "License"); you may not use this file except in compliance with
<span class=line>   <a name=6 href="/seamonkey/source/build/autoconf/altoptions.m4#6">6</a> </span>dnl the License. You may obtain a copy of the License at
<span class=line>   <a name=7 href="/seamonkey/source/build/autoconf/altoptions.m4#7">7</a> </span>dnl <a href="http://www.mozilla.org/MPL/">http://www.mozilla.org/MPL/</a>
<span class=line>   <a name=8 href="/seamonkey/source/build/autoconf/altoptions.m4#8">8</a> </span>dnl
<span class=line>   <a name=9 href="/seamonkey/source/build/autoconf/altoptions.m4#9">9</a> </span>dnl Software distributed under the License is distributed on an "AS IS" basis,
<span class=line>  <a name=10 href="/seamonkey/source/build/autoconf/altoptions.m4#10">10</a> </span>dnl WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
<span class=line>  <a name=11 href="/seamonkey/source/build/autoconf/altoptions.m4#11">11</a> </span>dnl for the specific language governing rights and limitations under the
<span class=line>  <a name=12 href="/seamonkey/source/build/autoconf/altoptions.m4#12">12</a> </span>dnl License.
<span class=line>  <a name=13 href="/seamonkey/source/build/autoconf/altoptions.m4#13">13</a> </span>dnl
<span class=line>  <a name=14 href="/seamonkey/source/build/autoconf/altoptions.m4#14">14</a> </span>dnl The Original Code is mozilla.org code.
<span class=line>  <a name=15 href="/seamonkey/source/build/autoconf/altoptions.m4#15">15</a> </span>dnl
<span class=line>  <a name=16 href="/seamonkey/source/build/autoconf/altoptions.m4#16">16</a> </span>dnl The Initial Developer of the Original Code is
<span class=line>  <a name=17 href="/seamonkey/source/build/autoconf/altoptions.m4#17">17</a> </span>dnl Netscape Communications Corporation.
<span class=line>  <a name=18 href="/seamonkey/source/build/autoconf/altoptions.m4#18">18</a> </span>dnl Portions created by the Initial Developer are Copyright (C) 1999
<span class=line>  <a name=19 href="/seamonkey/source/build/autoconf/altoptions.m4#19">19</a> </span>dnl the Initial Developer. All Rights Reserved.
<span class=line>  <a name=20 href="/seamonkey/source/build/autoconf/altoptions.m4#20">20</a> </span>dnl
<span class=line>  <a name=21 href="/seamonkey/source/build/autoconf/altoptions.m4#21">21</a> </span>dnl Contributor(s):
<span class=line>  <a name=22 href="/seamonkey/source/build/autoconf/altoptions.m4#22">22</a> </span>dnl
<span class=line>  <a name=23 href="/seamonkey/source/build/autoconf/altoptions.m4#23">23</a> </span>dnl Alternatively, the contents of this file may be used under the terms of
<span class=line>  <a name=24 href="/seamonkey/source/build/autoconf/altoptions.m4#24">24</a> </span>dnl either of the GNU General Public License Version 2 or later (the "GPL"),
<span class=line>  <a name=25 href="/seamonkey/source/build/autoconf/altoptions.m4#25">25</a> </span>dnl or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
<span class=line>  <a name=26 href="/seamonkey/source/build/autoconf/altoptions.m4#26">26</a> </span>dnl in which case the provisions of the GPL or the LGPL are applicable instead
<span class=line>  <a name=27 href="/seamonkey/source/build/autoconf/altoptions.m4#27">27</a> </span>dnl of those above. If you wish to allow use of your version of this file only
<span class=line>  <a name=28 href="/seamonkey/source/build/autoconf/altoptions.m4#28">28</a> </span>dnl under the terms of either the GPL or the LGPL, and not to allow others to
<span class=line>  <a name=29 href="/seamonkey/source/build/autoconf/altoptions.m4#29">29</a> </span>dnl use your version of this file under the terms of the MPL, indicate your
<span class=line>  <a name=30 href="/seamonkey/source/build/autoconf/altoptions.m4#30">30</a> </span>dnl decision by deleting the provisions above and replace them with the notice
<span class=line>  <a name=31 href="/seamonkey/source/build/autoconf/altoptions.m4#31">31</a> </span>dnl and other provisions required by the GPL or the LGPL. If you do not delete
<span class=line>  <a name=32 href="/seamonkey/source/build/autoconf/altoptions.m4#32">32</a> </span>dnl the provisions above, a recipient may use your version of this file under
<span class=line>  <a name=33 href="/seamonkey/source/build/autoconf/altoptions.m4#33">33</a> </span>dnl the terms of any one of the MPL, the GPL or the LGPL.
<span class=line>  <a name=34 href="/seamonkey/source/build/autoconf/altoptions.m4#34">34</a> </span>dnl
<span class=line>  <a name=35 href="/seamonkey/source/build/autoconf/altoptions.m4#35">35</a> </span>dnl ***** END LICENSE BLOCK *****
<span class=line>  <a name=36 href="/seamonkey/source/build/autoconf/altoptions.m4#36">36</a> </span>
<span class=line>  <a name=37 href="/seamonkey/source/build/autoconf/altoptions.m4#37">37</a> </span>dnl altoptions.m4 - An alternative way of specifying command-line options.
<span class=line>  <a name=38 href="/seamonkey/source/build/autoconf/altoptions.m4#38">38</a> </span>dnl    These macros are needed to support a menu-based configurator.
<span class=line>  <a name=39 href="/seamonkey/source/build/autoconf/altoptions.m4#39">39</a> </span>dnl    This file also includes the macro, AM_READ_MYCONFIG, for reading
<span class=line>  <a name=40 href="/seamonkey/source/build/autoconf/altoptions.m4#40">40</a> </span>dnl    the 'myconfig.m4' file.
<span class=line>  <a name=41 href="/seamonkey/source/build/autoconf/altoptions.m4#41">41</a> </span>
<span class=line>  <a name=42 href="/seamonkey/source/build/autoconf/altoptions.m4#42">42</a> </span>dnl Send comments, improvements, bugs to Steve Lamm <a href="mailto:slamm@netscape.com">(slamm@netscape.com)</a>.
<span class=line>  <a name=43 href="/seamonkey/source/build/autoconf/altoptions.m4#43">43</a> </span>
<span class=line>  <a name=44 href="/seamonkey/source/build/autoconf/altoptions.m4#44">44</a> </span>
<span class=line>  <a name=45 href="/seamonkey/source/build/autoconf/altoptions.m4#45">45</a> </span>dnl MOZ_ARG_ENABLE_BOOL(           NAME, HELP, IF-YES [, IF-NO [, ELSE]])
<span class=line>  <a name=46 href="/seamonkey/source/build/autoconf/altoptions.m4#46">46</a> </span>dnl MOZ_ARG_DISABLE_BOOL(          NAME, HELP, IF-NO [, IF-YES [, ELSE]])
<span class=line>  <a name=47 href="/seamonkey/source/build/autoconf/altoptions.m4#47">47</a> </span>dnl MOZ_ARG_ENABLE_STRING(         NAME, HELP, IF-SET [, ELSE])
<span class=line>  <a name=48 href="/seamonkey/source/build/autoconf/altoptions.m4#48">48</a> </span>dnl MOZ_ARG_ENABLE_BOOL_OR_STRING( NAME, HELP, IF-YES, IF-NO, IF-SET[, ELSE]]])
<span class=line>  <a name=49 href="/seamonkey/source/build/autoconf/altoptions.m4#49">49</a> </span>dnl MOZ_ARG_WITH_BOOL(             NAME, HELP, IF-YES [, IF-NO [, ELSE])
<span class=line>  <a name=50 href="/seamonkey/source/build/autoconf/altoptions.m4#50">50</a> </span>dnl MOZ_ARG_WITHOUT_BOOL(          NAME, HELP, IF-NO [, IF-YES [, ELSE])
<span class=line>  <a name=51 href="/seamonkey/source/build/autoconf/altoptions.m4#51">51</a> </span>dnl MOZ_ARG_WITH_STRING(           NAME, HELP, IF-SET [, ELSE])
<span class=line>  <a name=52 href="/seamonkey/source/build/autoconf/altoptions.m4#52">52</a> </span>dnl MOZ_ARG_HEADER(Comment)
<span class=line>  <a name=53 href="/seamonkey/source/build/autoconf/altoptions.m4#53">53</a> </span>dnl MOZ_CHECK_PTHREADS(            NAME, IF-YES [, ELSE ])
<span class=line>  <a name=54 href="/seamonkey/source/build/autoconf/altoptions.m4#54">54</a> </span>dnl MOZ_READ_MYCONFIG() - Read in 'myconfig.sh' file
<span class=line>  <a name=55 href="/seamonkey/source/build/autoconf/altoptions.m4#55">55</a> </span>
<span class=line>  <a name=56 href="/seamonkey/source/build/autoconf/altoptions.m4#56">56</a> </span>
<span class=line>  <a name=57 href="/seamonkey/source/build/autoconf/altoptions.m4#57">57</a> </span>dnl MOZ_TWO_STRING_TEST(NAME, VAL, STR1, IF-STR1, STR2, IF-STR2 [, ELSE])
<span class=line>  <a name=58 href="/seamonkey/source/build/autoconf/altoptions.m4#58">58</a> </span>AC_DEFUN([MOZ_TWO_STRING_TEST],
<span class=line>  <a name=59 href="/seamonkey/source/build/autoconf/altoptions.m4#59">59</a> </span>[if test "[$2]" = "[$3]"; then
<span class=line>  <a name=60 href="/seamonkey/source/build/autoconf/altoptions.m4#60">60</a> </span>    ifelse([$4], , :, [$4])
<span class=line>  <a name=61 href="/seamonkey/source/build/autoconf/altoptions.m4#61">61</a> </span>  elif test "[$2]" = "[$5]"; then
<span class=line>  <a name=62 href="/seamonkey/source/build/autoconf/altoptions.m4#62">62</a> </span>    ifelse([$6], , :, [$6])
<span class=line>  <a name=63 href="/seamonkey/source/build/autoconf/altoptions.m4#63">63</a> </span>  else
<span class=line>  <a name=64 href="/seamonkey/source/build/autoconf/altoptions.m4#64">64</a> </span>    ifelse([$7], ,
<span class=line>  <a name=65 href="/seamonkey/source/build/autoconf/altoptions.m4#65">65</a> </span>      [AC_MSG_ERROR([Option, [$1], does not take an argument ([$2]).])],
<span class=line>  <a name=66 href="/seamonkey/source/build/autoconf/altoptions.m4#66">66</a> </span>      [$7])
<span class=line>  <a name=67 href="/seamonkey/source/build/autoconf/altoptions.m4#67">67</a> </span>  fi])
<span class=line>  <a name=68 href="/seamonkey/source/build/autoconf/altoptions.m4#68">68</a> </span>
<span class=line>  <a name=69 href="/seamonkey/source/build/autoconf/altoptions.m4#69">69</a> </span>dnl MOZ_ARG_ENABLE_BOOL(NAME, HELP, IF-YES [, IF-NO [, ELSE]])
<span class=line>  <a name=70 href="/seamonkey/source/build/autoconf/altoptions.m4#70">70</a> </span>AC_DEFUN([MOZ_ARG_ENABLE_BOOL],
<span class=line>  <a name=71 href="/seamonkey/source/build/autoconf/altoptions.m4#71">71</a> </span>[AC_ARG_ENABLE([$1], [$2], 
<span class=line>  <a name=72 href="/seamonkey/source/build/autoconf/altoptions.m4#72">72</a> </span> [MOZ_TWO_STRING_TEST([$1], [$enableval], yes, [$3], no, [$4])],
<span class=line>  <a name=73 href="/seamonkey/source/build/autoconf/altoptions.m4#73">73</a> </span> [$5])])
<span class=line>  <a name=74 href="/seamonkey/source/build/autoconf/altoptions.m4#74">74</a> </span>
<span class=line>  <a name=75 href="/seamonkey/source/build/autoconf/altoptions.m4#75">75</a> </span>dnl MOZ_ARG_DISABLE_BOOL(NAME, HELP, IF-NO [, IF-YES [, ELSE]])
<span class=line>  <a name=76 href="/seamonkey/source/build/autoconf/altoptions.m4#76">76</a> </span>AC_DEFUN([MOZ_ARG_DISABLE_BOOL],
<span class=line>  <a name=77 href="/seamonkey/source/build/autoconf/altoptions.m4#77">77</a> </span>[AC_ARG_ENABLE([$1], [$2],
<span class=line>  <a name=78 href="/seamonkey/source/build/autoconf/altoptions.m4#78">78</a> </span> [MOZ_TWO_STRING_TEST([$1], [$enableval], no, [$3], yes, [$4])],
<span class=line>  <a name=79 href="/seamonkey/source/build/autoconf/altoptions.m4#79">79</a> </span> [$5])])
<span class=line>  <a name=80 href="/seamonkey/source/build/autoconf/altoptions.m4#80">80</a> </span>
<span class=line>  <a name=81 href="/seamonkey/source/build/autoconf/altoptions.m4#81">81</a> </span>dnl MOZ_ARG_ENABLE_STRING(NAME, HELP, IF-SET [, ELSE])
<span class=line>  <a name=82 href="/seamonkey/source/build/autoconf/altoptions.m4#82">82</a> </span>AC_DEFUN([MOZ_ARG_ENABLE_STRING],
<span class=line>  <a name=83 href="/seamonkey/source/build/autoconf/altoptions.m4#83">83</a> </span>[AC_ARG_ENABLE([$1], [$2], [$3], [$4])])
<span class=line>  <a name=84 href="/seamonkey/source/build/autoconf/altoptions.m4#84">84</a> </span>
<span class=line>  <a name=85 href="/seamonkey/source/build/autoconf/altoptions.m4#85">85</a> </span>dnl MOZ_ARG_ENABLE_BOOL_OR_STRING(NAME, HELP, IF-YES, IF-NO, IF-SET[, ELSE]]])
<span class=line>  <a name=86 href="/seamonkey/source/build/autoconf/altoptions.m4#86">86</a> </span>AC_DEFUN([MOZ_ARG_ENABLE_BOOL_OR_STRING],
<span class=line>  <a name=87 href="/seamonkey/source/build/autoconf/altoptions.m4#87">87</a> </span>[ifelse([$5], , 
<span class=line>  <a name=88 href="/seamonkey/source/build/autoconf/altoptions.m4#88">88</a> </span> [errprint([Option, $1, needs an "IF-SET" argument.
<span class=line>  <a name=89 href="/seamonkey/source/build/autoconf/altoptions.m4#89">89</a> </span>])
<span class=line>  <a name=90 href="/seamonkey/source/build/autoconf/altoptions.m4#90">90</a> </span>  m4exit(1)],
<span class=line>  <a name=91 href="/seamonkey/source/build/autoconf/altoptions.m4#91">91</a> </span> [AC_ARG_ENABLE([$1], [$2],
<span class=line>  <a name=92 href="/seamonkey/source/build/autoconf/altoptions.m4#92">92</a> </span>  [MOZ_TWO_STRING_TEST([$1], [$enableval], yes, [$3], no, [$4], [$5])],
<span class=line>  <a name=93 href="/seamonkey/source/build/autoconf/altoptions.m4#93">93</a> </span>  [$6])])])
<span class=line>  <a name=94 href="/seamonkey/source/build/autoconf/altoptions.m4#94">94</a> </span>
<span class=line>  <a name=95 href="/seamonkey/source/build/autoconf/altoptions.m4#95">95</a> </span>dnl MOZ_ARG_WITH_BOOL(NAME, HELP, IF-YES [, IF-NO [, ELSE])
<span class=line>  <a name=96 href="/seamonkey/source/build/autoconf/altoptions.m4#96">96</a> </span>AC_DEFUN([MOZ_ARG_WITH_BOOL],
<span class=line>  <a name=97 href="/seamonkey/source/build/autoconf/altoptions.m4#97">97</a> </span>[AC_ARG_WITH([$1], [$2],
<span class=line>  <a name=98 href="/seamonkey/source/build/autoconf/altoptions.m4#98">98</a> </span> [MOZ_TWO_STRING_TEST([$1], [$withval], yes, [$3], no, [$4])],
<span class=line>  <a name=99 href="/seamonkey/source/build/autoconf/altoptions.m4#99">99</a> </span> [$5])])
<span class=line> <a name=100 href="/seamonkey/source/build/autoconf/altoptions.m4#100">100</a> </span>
<span class=line> <a name=101 href="/seamonkey/source/build/autoconf/altoptions.m4#101">101</a> </span>dnl MOZ_ARG_WITHOUT_BOOL(NAME, HELP, IF-NO [, IF-YES [, ELSE])
<span class=line> <a name=102 href="/seamonkey/source/build/autoconf/altoptions.m4#102">102</a> </span>AC_DEFUN([MOZ_ARG_WITHOUT_BOOL],
<span class=line> <a name=103 href="/seamonkey/source/build/autoconf/altoptions.m4#103">103</a> </span>[AC_ARG_WITH([$1], [$2],
<span class=line> <a name=104 href="/seamonkey/source/build/autoconf/altoptions.m4#104">104</a> </span> [MOZ_TWO_STRING_TEST([$1], [$withval], no, [$3], yes, [$4])],
<span class=line> <a name=105 href="/seamonkey/source/build/autoconf/altoptions.m4#105">105</a> </span> [$5])])
<span class=line> <a name=106 href="/seamonkey/source/build/autoconf/altoptions.m4#106">106</a> </span>
<span class=line> <a name=107 href="/seamonkey/source/build/autoconf/altoptions.m4#107">107</a> </span>dnl MOZ_ARG_WITH_STRING(NAME, HELP, IF-SET [, ELSE])
<span class=line> <a name=108 href="/seamonkey/source/build/autoconf/altoptions.m4#108">108</a> </span>AC_DEFUN([MOZ_ARG_WITH_STRING],
<span class=line> <a name=109 href="/seamonkey/source/build/autoconf/altoptions.m4#109">109</a> </span>[AC_ARG_WITH([$1], [$2], [$3], [$4])])
<span class=line> <a name=110 href="/seamonkey/source/build/autoconf/altoptions.m4#110">110</a> </span>
<span class=line> <a name=111 href="/seamonkey/source/build/autoconf/altoptions.m4#111">111</a> </span>dnl MOZ_ARG_HEADER(Comment)
<span class=line> <a name=112 href="/seamonkey/source/build/autoconf/altoptions.m4#112">112</a> </span>dnl This is used by webconfig to group options
<span class=line> <a name=113 href="/seamonkey/source/build/autoconf/altoptions.m4#113">113</a> </span>define(MOZ_ARG_HEADER, [# $1])
<span class=line> <a name=114 href="/seamonkey/source/build/autoconf/altoptions.m4#114">114</a> </span>
<span class=line> <a name=115 href="/seamonkey/source/build/autoconf/altoptions.m4#115">115</a> </span>dnl
<span class=line> <a name=116 href="/seamonkey/source/build/autoconf/altoptions.m4#116">116</a> </span>dnl Apparently, some systems cannot properly check for the pthread
<span class=line> <a name=117 href="/seamonkey/source/build/autoconf/altoptions.m4#117">117</a> </span>dnl library unless &lt;pthread.h&gt; is included so we need to test
<span class=line> <a name=118 href="/seamonkey/source/build/autoconf/altoptions.m4#118">118</a> </span>dnl using it
<span class=line> <a name=119 href="/seamonkey/source/build/autoconf/altoptions.m4#119">119</a> </span>dnl
<span class=line> <a name=120 href="/seamonkey/source/build/autoconf/altoptions.m4#120">120</a> </span>dnl MOZ_CHECK_PTHREADS(lib, success, failure)
<span class=line> <a name=121 href="/seamonkey/source/build/autoconf/altoptions.m4#121">121</a> </span>AC_DEFUN([MOZ_CHECK_PTHREADS],
<span class=line> <a name=122 href="/seamonkey/source/build/autoconf/altoptions.m4#122">122</a> </span>[
<span class=line> <a name=123 href="/seamonkey/source/build/autoconf/altoptions.m4#123">123</a> </span>AC_MSG_CHECKING([for pthread_create in -l$1])
<span class=line> <a name=124 href="/seamonkey/source/build/autoconf/altoptions.m4#124">124</a> </span>echo "
<span class=line> <a name=125 href="/seamonkey/source/build/autoconf/altoptions.m4#125">125</a> </span>    #include &lt;pthread.h&gt; 
<span class=line> <a name=126 href="/seamonkey/source/build/autoconf/altoptions.m4#126">126</a> </span>    void *foo(void *v) { int a = 1;  } 
<span class=line> <a name=127 href="/seamonkey/source/build/autoconf/altoptions.m4#127">127</a> </span>    int main() { 
<span class=line> <a name=128 href="/seamonkey/source/build/autoconf/altoptions.m4#128">128</a> </span>        pthread_t t;
<span class=line> <a name=129 href="/seamonkey/source/build/autoconf/altoptions.m4#129">129</a> </span>        if (!pthread_create(&amp;t, 0, &amp;foo, 0)) {
<span class=line> <a name=130 href="/seamonkey/source/build/autoconf/altoptions.m4#130">130</a> </span>            pthread_join(t, 0);
<span class=line> <a name=131 href="/seamonkey/source/build/autoconf/altoptions.m4#131">131</a> </span>        }
<span class=line> <a name=132 href="/seamonkey/source/build/autoconf/altoptions.m4#132">132</a> </span>        exit(0);
<span class=line> <a name=133 href="/seamonkey/source/build/autoconf/altoptions.m4#133">133</a> </span>    }" &gt; dummy.c ;
<span class=line> <a name=134 href="/seamonkey/source/build/autoconf/altoptions.m4#134">134</a> </span>    echo "${CC-cc} -o dummy${ac_exeext} dummy.c $CFLAGS $CPPFLAGS -l[$1] $LDFLAGS $LIBS" 1&gt;&amp;5;
<span class=line> <a name=135 href="/seamonkey/source/build/autoconf/altoptions.m4#135">135</a> </span>    ${CC-cc} -o dummy${ac_exeext} dummy.c $CFLAGS $CPPFLAGS -l[$1] $LDFLAGS $LIBS 2&gt;&amp;5;
<span class=line> <a name=136 href="/seamonkey/source/build/autoconf/altoptions.m4#136">136</a> </span>    _res=$? ;
<span class=line> <a name=137 href="/seamonkey/source/build/autoconf/altoptions.m4#137">137</a> </span>    rm -f dummy.c dummy${ac_exeext} ;
<span class=line> <a name=138 href="/seamonkey/source/build/autoconf/altoptions.m4#138">138</a> </span>    if test "$_res" = "0"; then
<span class=line> <a name=139 href="/seamonkey/source/build/autoconf/altoptions.m4#139">139</a> </span>        AC_MSG_RESULT([yes])
<span class=line> <a name=140 href="/seamonkey/source/build/autoconf/altoptions.m4#140">140</a> </span>        [$2]
<span class=line> <a name=141 href="/seamonkey/source/build/autoconf/altoptions.m4#141">141</a> </span>    else
<span class=line> <a name=142 href="/seamonkey/source/build/autoconf/altoptions.m4#142">142</a> </span>        AC_MSG_RESULT([no])
<span class=line> <a name=143 href="/seamonkey/source/build/autoconf/altoptions.m4#143">143</a> </span>        [$3]
<span class=line> <a name=144 href="/seamonkey/source/build/autoconf/altoptions.m4#144">144</a> </span>    fi
<span class=line> <a name=145 href="/seamonkey/source/build/autoconf/altoptions.m4#145">145</a> </span>])
<span class=line> <a name=146 href="/seamonkey/source/build/autoconf/altoptions.m4#146">146</a> </span>
<span class=line> <a name=147 href="/seamonkey/source/build/autoconf/altoptions.m4#147">147</a> </span>dnl MOZ_READ_MYCONFIG() - Read in 'myconfig.sh' file
<span class=line> <a name=148 href="/seamonkey/source/build/autoconf/altoptions.m4#148">148</a> </span>AC_DEFUN([MOZ_READ_MOZCONFIG],
<span class=line> <a name=149 href="/seamonkey/source/build/autoconf/altoptions.m4#149">149</a> </span>[AC_REQUIRE([AC_INIT_BINSH])dnl
<span class=line> <a name=150 href="/seamonkey/source/build/autoconf/altoptions.m4#150">150</a> </span># Read in '.mozconfig' script to set the initial options.
<span class=line> <a name=151 href="/seamonkey/source/build/autoconf/altoptions.m4#151">151</a> </span># See the mozconfig2configure script for more details.
<span class=line> <a name=152 href="/seamonkey/source/build/autoconf/altoptions.m4#152">152</a> </span>_AUTOCONF_TOOLS_DIR=`dirname [$]0`/[$1]/build/autoconf
<span class=line> <a name=153 href="/seamonkey/source/build/autoconf/altoptions.m4#153">153</a> </span>. $_AUTOCONF_TOOLS_DIR/mozconfig2configure])
<span class=line> <a name=154 href="/seamonkey/source/build/autoconf/altoptions.m4#154">154</a> </span>
<span class=line> <a name=155 href="/seamonkey/source/build/autoconf/altoptions.m4#155">155</a> </span>dnl This gets inserted at the top of the configure script
<span class=line> <a name=156 href="/seamonkey/source/build/autoconf/altoptions.m4#156">156</a> </span>MOZ_READ_MOZCONFIG(MOZ_TOPSRCDIR)
</pre><p class=footer>
   This page was automatically generated by 
   <a href="http://lxr.mozilla.org/mozilla/source/webtools/lxr/">MXR</a>.
</p>
</html>
