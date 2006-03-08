Hello!  Thank you for being interested in localizing Songbird!

This file should be found alongside three other files:

contents.rdf
songbird.dtd
songbird.properties

--------------------------------

You must edit the contents.rdf file to specify what language you are providing.

Everywhere it says en-US, you should replace it with your language-country code.

A language code is based on the iso639 specification:
http://www.oasis-open.org/cover/iso639a.html

A country code is based on the iso3166 specification:
http://www.oasis-open.org/cover/country3166.html

The W3C describes the language encoding as follows:
http://www.w3.org/TR/REC-html40/struct/dirlang.html#h-8.1.1

You should also change the chrome:displayName and chrome:author attributes
        chrome:displayName="English(US)"
        chrome:author="Pioneers of the Inevitable"

--------------------------------

You must translate the songbird.dtd file.  The dtd file is full of ENTITY statements:

<!ENTITY songbird           "Songbird" >

You must translate everything between the quotes.

--------------------------------

You must also translate the songbird.properties file.  It is full of paired statements separated by an '=' character.

faceplate.welcome=Welcome to Songbird

You must translate everything to the right of the '=' character.

--------------------------------

All files must be saved in UTF-8 format.  Notepad.exe works great.

When you are happy with your translation, you should zip it up and name the zipfile with your language code.

Email it to i18n@songbirdnest.com

Thanks!