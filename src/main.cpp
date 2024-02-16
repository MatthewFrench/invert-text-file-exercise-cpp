#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "utf8.h"

using namespace std;

int main(int argumentCount, char *argumentList[]) {
    if (argumentCount < 2) {
        cout << "Please provide an input file." << endl;
    }
    if (argumentCount < 3) {
        cout << "Please provide an output file." << endl;
    }

    /*
     * Todo:
     * Can we multi-thread this to have one thread for loading and
     * inserting lines into a queue, one thread for processing lines
     * in that queue and submitting to a write-out queue, and one thread
     * for saving the write-out queue to output?
     */

    string inputFilePath = argumentList[1];
    string outputFilePath = argumentList[2];
    std::filesystem::remove(outputFilePath);
    ifstream readFromFile;
    ofstream writeToFile;
    readFromFile.open(inputFilePath, ios::in);
    writeToFile.open(outputFilePath);
    istreambuf_iterator<char> start(readFromFile);
    istreambuf_iterator<char> end;
    std::vector<string> line;
    while (start != end) {
        string s;
        utf8::utfchar32_t characterPoint = utf8::next(start, end);
        utf8::append(characterPoint, s);
        if (s[0] == '\n') {
            for (int i = line.size() - 1; i >= 0; i--) {
                writeToFile << line[i];
            }
            line.clear();
            writeToFile << '\n';
        } else {
            line.push_back(s);
        }
    }
    for (int i = line.size() - 1; i >= 0; i--) {
        writeToFile << line[i];
    }
    readFromFile.close();
    writeToFile.close();
}