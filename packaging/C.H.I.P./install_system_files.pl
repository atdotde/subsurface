#!/usr/bin/perl -w

#
# This script iterates through the file FILENAMES and copies files referenced
# there to pwd in order to bring them under version control.
# Running with option -i restores them to their original place and restores ownership
# and permissions.
#
# This script should thus run with sudo. Handle with care.
#

use strict;

my $install = (@ARGV && $ARGV[0] eq '-i');

if ($<) {
  print "$0: This should be run with sudo\n";
  exit;
}

if ($install && not -e /CHIP_INSTALL_OK/) {
  die "$0: Run install only when /CHIP_INSTALL_OK exists. This overwrites system files!";
}

open(FL, "FILELIST") || die "$0: Cannot open FILELIST: $!";
open(OUT, ">FILELIST.new") || die "$0: Cannot open FILELIST.new: $!" unless $install;

while(<FL>) {
  my ($uid, $gid, $permissions, $path);
  chomp;
  s/#.*//;             # Remove comments
  s/^\s+//;            # Remove whitespace
  s/\s+$//;
  next unless /\S/;    # Skip empty lines
  if (/(\w*)\|(\w*)\|(\d*)\|(.*)/) {
    $uid = $1;
    $gid = $2;
    $permissions = oct($3);
    $path = $4;
  } else {
    printf "$_\ndoes not match pattern 'uid|gid|permissions|path'.\n";
    next;
  }
  $path =~ /.*\/(.+)/;
  my $filename = $1;
  if ($install) {
    print "Installing $path\n";
    if ($path =~ /\/$/) {
      mkdir($path) unless -d $path;
    } else {
      system ('cp', ,'-a', $filename, $path);
    }
    my $nuid = getpwnam($uid);
    my $ngid = getgrnam($gid);
    chown $nuid, $ngid, $path;
    chmod $permissions, $path;
  } else {
    print "Fetching $path\n";
    my ($nuid, $ngid, $mode) = (stat($path))[4,5,2];
    $uid = getpwuid($nuid);
    $gid = getgrgid($ngid);
    $permissions = $mode & 07777;
    if (-d $path && $path !~ /\/$/) {
      $path .= '/';
    }
    printf OUT "$uid|$gid|%o|$path\n", $permissions;
    if (not -d $path) {
      system ('cp', $path, $filename);
    }
  }
}
close FL;

if (not $install) {
  close OUT;
  rename "FILELIST.new", "FILELIST";
}
