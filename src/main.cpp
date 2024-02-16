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
    istreambuf_iterator<char> readStartIterator(readFromFile);
    istreambuf_iterator<char> readEndIterator;
    std::vector<string> currentLine;
    while (readStartIterator != readEndIterator) {
        string characterPointAsString;
        utf8::utfchar32_t characterPoint = utf8::next(readStartIterator, readEndIterator);
        utf8::append(characterPoint, characterPointAsString);
        if (characterPointAsString[0] == '\n') {
            for (int index = currentLine.size() - 1; index >= 0; index--) {
                writeToFile << currentLine[index];
            }
            currentLine.clear();
            writeToFile << '\n';
        } else {
            currentLine.push_back(characterPointAsString);
        }
    }
    for (int index = currentLine.size() - 1; index >= 0; index--) {
        writeToFile << currentLine[index];
    }
    readFromFile.close();
    writeToFile.close();
}