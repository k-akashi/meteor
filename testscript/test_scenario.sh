#!/bin/sh -x

text_file=$1
tmp_file=$1.tmp

awk 'BEGIN{
    FS = " ";
}
NR >= 3 {
    time = $1;
    delay = $18;
	if (delay > 30) {
		delay += 1000;
	}
    print time, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, "0.0000", delay, "0.0000";
}' ${text_file} > $tmp_file
