#!/bin/sh
# 
# coremask.sh -  convert two number range from and to a hexadecimal bitmask value.
# Usage:     coremask from to 
# Example:   coremask 0 3
# Output:    0x0F
#
# Richard W
#

from=$1 to=$2

max=63
bitmask=0

if [ $to -gt $max -o  $from -gt $max ]
then
	echo "Maximum number is 63."
	exit
fi

if [ $from -gt $to ]
then
	echo "First number must be equal or less than second number."
	exit
fi

from=$(printf '%d' "$from") to=$(printf '%d' "$to")
while test "$from" -le "$to"; do
	bitpos=$((1 << $from))
    from=$((from+1))
	bitmask=$(($bitmask | $bitpos))
done

printf 'Coremask = 0x%llX' "$bitmask"
printf '\n'

