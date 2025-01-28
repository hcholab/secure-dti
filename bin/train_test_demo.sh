#!/bin/bash

pushd "demo_data" || exit 1

# Create directories for both parties
mkdir -p batch_pw1
mkdir -p batch_pw2

# Calculate lines for test data
total_lines=$(wc -l < X.txt)
test_lines=$((total_lines - 150))
half_test=$((test_lines / 2))

for f in X y; do
    # Split first 150 lines into two equal parts for training (75 each)
    head -n75 "$f.txt" > "batch_pw1/${f}train"
    head -n150 "$f.txt" | tail -n75 > "batch_pw2/${f}train"

    # Split remaining lines into two equal parts for testing
    head -n$((150 + half_test)) "$f.txt" | tail -n$half_test > "batch_pw1/${f}test"
    tail -n$((test_lines - half_test)) ${f}.txt > "batch_pw2/${f}test"
done

# Create suffix files for each party
for dir in batch_pw1 batch_pw2; do
    for suffix in train test; do
        echo ${suffix} > "${dir}/${suffix}_suffixes.txt"
    done
done

popd || exit 1
