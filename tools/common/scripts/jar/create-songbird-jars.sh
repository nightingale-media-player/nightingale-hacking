chromedir=../../../Staging/chrome
perlcmd="perl -I../ ../make-jars.pl -z zip -s ../$chromedir -j ../$chromedir"
mkdir TEMP
cd TEMP/
echo songbird.jar
echo
$perlcmd < ../$chromedir/songbird-jar.mn
echo
echo rubber-ducky.jar
echo
$perlcmd < ../$chromedir/rubber-ducky-jar.mn
echo
echo cardinal.jar
echo
$perlcmd < ../$chromedir/cardinal-jar.mn
echo
echo all-locales.jar
echo
$perlcmd < ../$chromedir/all-locales-jar.mn
cd ..
rm -rf TEMP/
