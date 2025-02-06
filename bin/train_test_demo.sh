#!/bin/bash

pushd "demo_data" || exit 1

# Create directories for both parties
mkdir -p batch_pw1
mkdir -p batch_pw2

for f in X y; do
    # Split first 150 lines into two equal parts for training (75 each)
    head -n75 "$f.txt" > "batch_pw1/${f}train"
    head -n150 "$f.txt" | tail -n75 > "batch_pw2/${f}train"

    # Write remaining lines into a test set
    tail -n$(($(wc -l < "$f.txt") - 150)) "$f.txt" | \
        tee "batch_pw1/${f}test" "batch_pw2/${f}test" > /dev/null
done

# Create suffix files for each party
for dir in batch_pw batch_pw1 batch_pw2; do
    for suffix in train test; do
        echo ${suffix} > "${dir}/${suffix}_suffixes.txt"
    done
done

popd || exit 1
