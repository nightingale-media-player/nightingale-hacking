#!/usr/bin/perl
use strict;
use File::Path;
use Encode;

while(my $l = <>) {
  my @f = split("\t", $l);

  my @time = split(":", $f[5]);
  my $seconds = $time[0] * 60 + $time[1];
  my $path = "lib/$f[0]/$f[1]";
  my $filename = "$f[4] $f[3].mp3";

  print "$f[0] - $f[1] - $f[3], ${seconds}s to $path/$filename\n";

  generate($seconds);

  encode_mp3($f[0], $f[1], $f[3], $f[2], $f[4], $path, $filename);
}

sub generate {
  my ($seconds) = shift;

  my $csd = <<EOF;
<CsoundSynthesizer>;
 ; test.csd - a Csound structured data file
<CsOptions>
 -W -d -m 0 -o tone.wav
</CsOptions>
<CsInstruments>
 sr = 44100
 kr = 4410
 ksmps = 10
 nchnls = 1
 instr   1
     a1 oscil p4, p5, 1 ; simple oscillator
        out a1
   endin
</CsInstruments>
<CsScore>
 f1 0 8192 10 1
 i1 0 $seconds 20000 1000 ;play one second of one kHz tone
 e
</CsScore>
</CsoundSynthesizer>
EOF

  open(OUT, "> /tmp/libgen.csd");
  print OUT $csd;
  close(OUT);
  system("csound /tmp/libgen.csd");
}

sub encode_mp3 {
  my ($artist, $album, $track, $tracknum, $genre, $destdir, $filename) = @_;

  $artist = encode("iso-8859-1", decode("utf8", $artist));
  $album  = encode("iso-8859-1", decode("utf8", $album));
  $track  = encode("iso-8859-1", decode("utf8", $track));

  mkpath($destdir);

  my $cmd = <<EOF;
lame --ta "$artist" --tl "$album" --tt "$track" --tg "$genre" --tn "$tracknum" -b 64 --id3v2-only tone.wav "$destdir/$filename"
EOF

print "$cmd\n";
  system($cmd);
}

