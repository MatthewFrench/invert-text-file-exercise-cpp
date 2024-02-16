# Invert Text File Exercise in C++

This project takes a text file input and target output, then saves each line of the input file reversed to the output file. Unicode-friendly.

Setup:
* `git clone https://github.com/microsoft/vcpkg`
* `./vcpkg/bootstrap-vcpkg.sh`
* `./vcpkg/vcpkg --feature-flags=manifests install`
* `cmake -B build -S .`

Run with:
* `./build/InvertTextFileExercise {input_file} {output_file}`

Potential future improvements:
* Detect file encoding and use the proper UTF-8/UTF-16/UTF-32
* Add an option to limit line memory usage by incorporating a new line find read forward then a read back to reverse and write the line out
* Add unit tests to ensure correct-ness
* Create a large input for benchmarking, add a naive solution to compare in benchmark