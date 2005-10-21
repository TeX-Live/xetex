#!/bin/sh
# Copyright (C) 2001 - 2003, International Business Machines Corporation.      
# All Rights Reserved.                                                       
#
# Authors:
# Ami Fixler
# Steven R. Loomis <srl@jtcsv.com>
# George Rhoten
#
# Shell script to unpax ICU and convert the files to an EBCDIC codepage.
# After extracting to EBCDIC, binary files are re-extracted without the
# EBCDIC conversion, thus restoring them to original codepage.
#
# Set the following variable to the list of binary file suffixes (extensions)

#binary_suffixes='ico ICO bmp BMP jpg JPG gif GIF brk BRK'
#ICU specific binary files
binary_suffixes='brk BRK bin BIN res RES cnv CNV dat DAT icu ICU spp SPP'

usage()
{
  echo "Enter archive filename as a parameter: $0 icu-archive.tar [strip]"
  echo "(strip is an option to remove hex '0D' carraige returns)"
}
# first make sure we at least one arg and it's a file we can read
if [ $# -eq 0 ]; then
  usage
  exit
fi
if [ ! -r $1 ]; then
  echo "$1 does not exist or cannot be read."
  usage
  exit
fi
# set up a few variables

echo ""
echo "Extracting from $1 ..."
echo ""
# extract files while converting them to EBCDIC
pax -rvf $1 -o to=IBM-1047,from=ISO8859-1

if [ $# -gt 1 ]; then 
  if [ $2 -eq strip ]; then
    echo ""
    echo "Stripping hex 0d characters ..."
    for i in $(pax -f $1 2>/dev/null)
    do
      case $i in
        */)
         # then this entry is a directory
         ;;
        *)
          # then this entry is NOT a directory
          tr -d  <$i >@@@icu@tmp
          chmod +w $i
          rm $i
          mv @@@icu@tmp $i
          ;;
       esac
    done
  fi
fi

echo ""
echo "Determining binary files ..."
echo ""

#for dir in `find ./icu -type d \( -name CVS -o -print \)`; do
#    if [ -f $dir/CVS/Entries ]; then
#        binary_files="$binary_files`cat $dir/CVS/Entries | fgrep -- -kb \
#                      | cut -d / -f2 | sed -e "s%^%$dir/%" \
#                      | sed -e "s%^\./%%" | tr '\n' ' '`"
#    fi
#done
#echo "Detecting Unicode files"
for file in `find icu \( -name \*.txt -print \)`; do
    bom8=`head -c 3 $file|\
          od -t x1|\
          head -n 1|\
          sed 's/  */ /g'|\
          cut -f2-4 -d ' '|\
          tr 'A-Z' 'a-z'`;
    #Find a converted UTF-8 BOM
    if test "$bom8" = "57 8b ab"
    then
        binary_files="$binary_files $file";
    fi
done

for i in $(pax -f $1 2>/dev/null)
do
  case $i in
    */)
#    then this entry is a directory
     ;;
    *.*)
#    then this entry has a dot in the filename
     for j in $binary_suffixes
     do
       suf=${i#*.*}
       if [ "$suf" = "$j" ]
       then
         binary_files="$binary_files $i"
         break
       fi
     done
     ;;
    *)
#    then this entry does not have a dot in it
     ;;
  esac
done

# now see if a re-extract of binary files is necessary
if [ ${#binary_files} -eq 0 ]; then
  echo ""
  echo "There are no binary files to restore."
else
  echo "Restoring binary files ..."
  echo ""
  rm $binary_files
  pax -rvf $1 $binary_files
fi
echo ""
echo "$0 has completed extracting ICU from $1."
