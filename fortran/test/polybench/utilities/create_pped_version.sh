#!/bin/sh
## create_pped_version.sh for  in /Users/pouchet
##
## Made by Louis-Noel Pouchet
## Contact: <pouchet@cse.ohio-state.edu>
##
## Started on  Mon Oct 31 16:20:01 2011 Louis-Noel Pouchet
## Last update Mon Oct 31 20:42:35 2011 Louis-Noel Pouchet
##
set -eu

if [ $# -lt 1 ]; then
    echo "Usage: create_pped_version.sh <file.F90> [gcc -E flags]";
    exit 1;
fi;

poly_top_f="$(mktemp --suffix=.f)"
poly_bottom_F="$(mktemp --suffix=.F)"
poly_bottom_f="$(mktemp --suffix=.f)"
poly_f="$(mktemp --suffix=.f)"
trap 'rm -f "$poly_top_f" "$poly_bottom_F" "$poly_bottom_f" "$poly_f"' EXIT

file="$1";
shift 1
head -n 8 $file | sed 's/F90/f90/' > "$poly_top_f";
tail -n +9 $file > "$poly_bottom_F";

filename=`echo "$file" | sed -e "s/\(.*\).F90/\1/1"`;
filenameorig=`basename $file`;
benchdir=`dirname "$file"`;

gcc -E "$poly_bottom_F" -I$benchdir "$@" 2>/dev/null > "$poly_f"
#echo "gcc -E .__poly_bottom.f -I$benchdir -Iutilities $args 2>/tmp/moh > .__tmp_poly.f"

sed -e "/^[ ]*$/d" "$poly_f" | sed -e '/^#/d' | sed -e "s~$poly_bottom_f~$filenameorig~g" > "$poly_bottom_f";
cat "$poly_top_f" > "$filename.preproc.f90";
#echo "#include <polybench.h>\n" >> $filename.preproc.c;

cat "$poly_bottom_f" >> "$filename.preproc.f90";
