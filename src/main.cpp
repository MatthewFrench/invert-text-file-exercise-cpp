#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <semaphore>
#include <sstream>
#include "utf8.h"

using namespace std;

void retrievalThreadFunction(
        const string &inputFilePath,
        queue<vector<string> *> *linesAvailableToProcess,
        mutex *linesAvailableToProcessLock,
        atomic<bool> *readingFinished
) {
    ifstream readFromFile;
    readFromFile.open(inputFilePath, ios::in);

    istreambuf_iterator<char> readStartIterator(readFromFile);
    istreambuf_iterator<char> readEndIterator;
    // Note, if a single line takes up too much memory, we can change the solution
    // to limit the amount of memory being used. It will involve a tradeoff of memory vs speed.
    // One example would be to do a read scan for the next new-line, then read the file
    // binary data backwards from that newline to avoid storing the whole line in memory.
    // The trade-off there would be double the amount of reading to decrease amount of data stored in memory.
    vector<string> *currentLine = nullptr;
    while (readStartIterator != readEndIterator) {
        if (currentLine == nullptr) {
            currentLine = new vector<string>;
        }
        string characterPointAsString;
        // Handle unicode characters, just reversing bytes will make unicode text invalid
        utf8::utfchar32_t characterPoint = utf8::next(readStartIterator, readEndIterator);
        utf8::append(characterPoint, characterPointAsString);
        if (characterPointAsString[0] == '\n') {
            linesAvailableToProcessLock->lock();
            linesAvailableToProcess->push(currentLine);
            // Explicitly track the new lines, so we don't add an unnecessary newline to the output file
            linesAvailableToProcess->push(new vector<string>{"\n"});
            linesAvailableToProcessLock->unlock();
            currentLine = nullptr;
        } else {
            currentLine->push_back(characterPointAsString);
        }
    }
    if (currentLine != nullptr) {
        linesAvailableToProcessLock->lock();
        linesAvailableToProcess->push(currentLine);
        linesAvailableToProcessLock->unlock();
    }
    readingFinished->store(true);
    readFromFile.close();
}

void processThreadFunction(
        queue<vector<string> *> *linesAvailableToProcess,
        mutex *linesAvailableToProcessLock,
        queue<string *> *linesToWriteOut,
        mutex *linesToWriteOutLock,
        atomic<bool> *readingFinished,
        atomic<bool> *processingFinished
) {
    auto getNextItemForProcessing = [linesAvailableToProcess, linesAvailableToProcessLock]() -> vector<string> * {
        linesAvailableToProcessLock->lock();
        vector<string> *nextItem = nullptr;
        if (!linesAvailableToProcess->empty()) {
            nextItem = linesAvailableToProcess->front();
            linesAvailableToProcess->pop();
        }
        linesAvailableToProcessLock->unlock();
        return nextItem;
    };
    vector<string> *currentLine;
    while ((currentLine = getNextItemForProcessing()) != nullptr || !readingFinished->load()) {
        if (currentLine == nullptr) {
            // We're still reading lines but there is nothing to process, yield to allow
            // other threads to use the cpu
            std::this_thread::yield();
            continue;
        }
        std::stringstream stringBuilder;
        for (auto reverseIterator = currentLine->rbegin();
             reverseIterator != currentLine->rend(); ++reverseIterator) {
            stringBuilder << *reverseIterator;
        }
        auto *reversedString = new string(stringBuilder.str());
        linesToWriteOutLock->lock();
        linesToWriteOut->push(reversedString);
        linesToWriteOutLock->unlock();
        // This was created in the retrieval thread, delete here to avoid a memory leak
        delete currentLine;
    }
    processingFinished->store(true);
}

void writeThreadFunction(
        const string &outputFilePath,
        queue<string *> *linesToWriteOut,
        mutex *linesToWriteOutLock,
        atomic<bool> *processingFinished
) {
    auto getNextItemForWriting = [linesToWriteOut, linesToWriteOutLock]() -> string * {
        linesToWriteOutLock->lock();
        string *nextItem = nullptr;
        if (!linesToWriteOut->empty()) {
            nextItem = linesToWriteOut->front();
            linesToWriteOut->pop();
        }
        linesToWriteOutLock->unlock();
        return nextItem;
    };

    std::filesystem::remove(outputFilePath);
    ofstream writeToFile;
    writeToFile.open(outputFilePath);

    string *currentLine;
    while ((currentLine = getNextItemForWriting()) != nullptr || !processingFinished->load()) {
        if (currentLine == nullptr) {
            // We're still reading or processing lines but there is nothing to write,
            // yield to allow other threads to use the cpu
            std::this_thread::yield();
            continue;
        }
        writeToFile << *currentLine;
        // This was created in the processing thread, delete here to avoid a memory leak
        delete currentLine;
    }

    writeToFile.close();
}

int main(int argumentCount, char *argumentList[]) {
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    if (argumentCount < 2) {
        cout << "Please provide an input file." << endl;
    }
    if (argumentCount < 3) {
        cout << "Please provide an output file." << endl;
    }

    string inputFilePath = argumentList[1];
    string outputFilePath = argumentList[2];

    auto *linesAvailableToProcess = new queue<vector<string> *>();
    auto *linesAvailableToProcessLock = new mutex();
    auto *linesToWriteOut = new std::queue<string *>();
    auto *linesToWriteOutLock = new mutex();
    auto *readingFinished = new atomic<bool>(false);
    auto *processingFinished = new atomic<bool>(false);

    std::thread retrievalThread(retrievalThreadFunction,
                                inputFilePath,
                                linesAvailableToProcess,
                                linesAvailableToProcessLock,
                                readingFinished);
    std::thread processThread(processThreadFunction,
                              linesAvailableToProcess,
                              linesAvailableToProcessLock,
                              linesToWriteOut,
                              linesToWriteOutLock,
                              readingFinished,
                              processingFinished);
    std::thread writeThread(writeThreadFunction,
                            outputFilePath,
                            linesToWriteOut,
                            linesToWriteOutLock,
                            processingFinished);

    retrievalThread.join();
    std::chrono::steady_clock::time_point readingEndTime = std::chrono::steady_clock::now();
    processThread.join();
    std::chrono::steady_clock::time_point processingEndTime = std::chrono::steady_clock::now();
    writeThread.join();
    std::chrono::steady_clock::time_point writingEndTime = std::chrono::steady_clock::now();

    double readingDuration = std::chrono::duration<double>(readingEndTime - startTime).count();
    double processingDuration = std::chrono::duration<double>(processingEndTime - startTime).count();
    double writingDuration = std::chrono::duration<double>(writingEndTime - startTime).count();
    cout << "Reading finished in " << readingDuration << " seconds." << endl;
    cout << "Processing finished in " << processingDuration << " seconds." << endl;
    cout << "Writing finished in " << writingDuration << " seconds." << endl;

    delete linesAvailableToProcess;
    delete linesAvailableToProcessLock;
    delete linesToWriteOut;
    delete linesToWriteOutLock;
    delete readingFinished;
    delete processingFinished;
}