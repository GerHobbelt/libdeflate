#
# Helper script used by run_tests.sh, not intended to be run directly
#

set -eu

run_cmd() {
	echo "$WRAPPER $@"
	$WRAPPER "$@" > /dev/null
}

run_cmd ./test_checksums

for debug_args in '' '-G'; do
	for format in '' '-g' '-z'; do
		for ref_impl in '' '-Y' '-Z'; do
			run_cmd ./benchmark $format $debug_args $ref_impl \
				$SMOKEDATA
		done
	done
	for level in 1 3 7 9; do
		for ref_impl in '' '-Y'; do
			run_cmd ./benchmark -$level $debug_args $ref_impl \
				$SMOKEDATA
		done
	done
	for level in 1 3 7 9 12; do
		for ref_impl in '' '-Z'; do
			run_cmd ./benchmark -$level $debug_args $ref_impl \
				$SMOKEDATA
		done
	done
done

echo "exec_tests finished successfully" # Needed for 'adb shell'
