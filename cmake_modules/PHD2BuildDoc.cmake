# Copyright 2014-2015, Max Planck Society.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

# File created by Raffi Enficiaud
# This file mimics the old PERL script behaviour build_help_hhk


set(help_hhp_file ${PHD_PROJECT_ROOT_DIR}/PHD2GuideHelp.hhp)

function(get_phd_version)
  set(filename_to_extract_from ${PHD_PROJECT_ROOT_DIR}/phd.h)
  file(STRINGS ${filename_to_extract_from} file_content
       #REGEX "PHDVERSION[ _T\\(]+\"(.*)\""
  )

  foreach(SRC_LINE ${file_content})
    if("${SRC_LINE}" MATCHES "PHDVERSION[ _T\\(]+\"(([0-9]+)\\.([0-9]+).([0-9]+))\"")
        # message("Extracted/discovered version '${CMAKE_MATCH_1}'")
        set(VERSION_MAJOR ${CMAKE_MATCH_2} PARENT_SCOPE)
        set(VERSION_MINOR ${CMAKE_MATCH_3} PARENT_SCOPE)
        set(VERSION_PATCH ${CMAKE_MATCH_4} PARENT_SCOPE)
        return()
    endif()
  endforeach()

  message(FATAL_ERROR "Cannot extract version from file '${filename_to_extract_from}'")

endfunction()


# get_phd_version()

#use strict;

#my %a;

#print "reading PHD2GuideHelp.hhp\n";

#open HHP, "<PHD2GuideHelp.hhp";
#my $files;
#while (<HHP>) {
#    chomp;
#    s/^\s+//g;
#    s/\s+$//g;
#    next unless length $_;
#    if ($files) {
#        last if /^\[/;
#        my $f = $_;
#        print "  $f\n";
#        open F, "<$f" or die "could not open $f: $!\n";
#        while (<F>) {
#            foreach (m/<\s*a\s+name\s*=\s*"([^\"]+)"/ig,
#                     m/<\s*a\s+name\s*=\s*(\w+)\s*[^>]*>/ig) {
#                my $text = ucfirst $_;
#                $text =~ s/_/ /g;
#                $a{lc $text} = [ $_, $f, $text ];
#            }
#        }
#    }
#    elsif (/^\[FILES\]/) {
#        $files = 1;
#    }
#}



#[OPTIONS]
#Compatibility=1.1 or later
#Compiled file=PHD2_Help.chm
#Contents file=PHD2GuideHelp.hhc
#Default topic=Introduction.htm
#Display compile progress=No
#Index file=PHD2GuideHelp.hhk
#Language=0x409 English (United States)
#Title=PHD2 Guiding

#[FILES]
#MainScreen.htm
#Basic_use.htm
#Visualization.htm
#Advanced_settings.htm
#Guide_algorithms.htm
#Tools.htm
#Introduction.htm
#Trouble_shooting.htm
#Darks_BadPixel_Maps.htm
#KeyboardShortcuts.htm

#[INFOTYPES]





#print "writing PHD2GuideHelp.hhk\n";

#open OUT, ">PHD2GuideHelp.hhk";

#print OUT <<'EOF';
#<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
#<HTML><HEAD>
#<!-- Sitemap 1.0 -->
#</HEAD><BODY>
#<UL>
#EOF

#foreach (sort keys %a) {
#    my ($a, $f, $text) = @{$a{$_}};
#    print OUT <<EOF;
#\t<LI><OBJECT type="text/sitemap">
#\t\t<param name="Name" value="$text">
#\t\t<param name="Name" value="$a">
#\t\t<param name="Local" value="$f#$a">
#\t\t</OBJECT>
#EOF
#}

#print OUT <<'EOF';
#</UL>
#</BODY></HTML>
#EOF

#print "done.\n";
