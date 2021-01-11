for i in *
do
	n=$(basename $i)
	if test -f ../mybmm/$n; then
		diff $i ../mybmm/$n >/dev/null 2>&1
		if test $? -ne 0; then
			echo "$i and ../mybmm/$n differ"
		fi
	fi
done
