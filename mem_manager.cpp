#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iomanip>

// Struct for representing a process
struct Process {
    int id;
    int arrivalTime;
    int lifetime;
    int totalMemory;
    std::vector<int> memoryPieces;
    int startTime = -1;
    int completionTime = -1;
    bool admitted = false;
};

// Struct for representing memory blocks
struct MemoryBlock {
    int start;
    int size;
    bool free;

    MemoryBlock(int s, int sz, bool f) : start(s), size(sz), free(f) {}
};

// Memory Manager class
class MemoryManager {
private:
    int memorySize;
    int pageSize;
    std::vector<MemoryBlock> memoryMap;

public:
    MemoryManager(int memorySize, int pageSize) : memorySize(memorySize), pageSize(pageSize) {
        memoryMap.emplace_back(0, memorySize, true); // Initial memory is free
    }

    bool allocate(Process& process) {
        int numPages = (process.totalMemory + pageSize - 1) / pageSize;  // Round up to the nearest page
        for (auto& block : memoryMap) {
            if (block.free && block.size >= numPages * pageSize) {
                block.free = false;
                process.admitted = true;
                process.startTime = block.start;
                process.completionTime = process.startTime + process.lifetime;

                // Split the block if necessary
                if (block.size > numPages * pageSize) {
                    memoryMap.emplace_back(block.start + numPages * pageSize, block.size - numPages * pageSize, true);
                }
                block.size = numPages * pageSize; // Allocate the exact number of pages needed

                return true;
            }
        }
        return false; // No suitable memory block found
    }

    void release(const Process& process) {
        for (auto& block : memoryMap) {
            if (block.start == process.startTime) {
                block.free = true;
                mergeFreeBlocks();
                return;
            }
        }
    }

    void mergeFreeBlocks() {
        for (size_t i = 0; i < memoryMap.size() - 1; ++i) {
            if (memoryMap[i].free && memoryMap[i + 1].free) {
                memoryMap[i].size += memoryMap[i + 1].size;
                memoryMap.erase(memoryMap.begin() + i + 1);
                --i; // Recheck the merged block
            }
        }
    }

    void printMemoryMap(std::ofstream& logFile) {
        for (const auto& block : memoryMap) {
            if (block.free) {
                logFile << "[" << block.start << ", " << (block.start + block.size - 1) << "] - Free\n";
            } else {
                logFile << "[" << block.start << ", " << (block.start + block.size - 1) << "] - Allocated\n";
            }
        }
    }
};

// Simulation function
void simulate(const std::string& inputFileName, const std::string& outputFileName, int memorySize, int pageSize) {
    std::ifstream inputFile(inputFileName);
    std::ofstream outputFile(outputFileName);

    if (!inputFile) {
        std::cerr << "Error: Cannot open input file: " << inputFileName << "\n";
        return;
    }
    if (!outputFile) {
        std::cerr << "Error: Cannot create output file: " << outputFileName << "\n";
        return;
    }

    int numProcesses;
    inputFile >> numProcesses;
    std::queue<Process> inputQueue;
    std::vector<Process> completedProcesses;

    // Read input file
    for (int i = 0; i < numProcesses; ++i) {
        Process process;
        inputFile >> process.id >> process.arrivalTime >> process.lifetime;

        int numPieces;
        inputFile >> numPieces;
        process.totalMemory = 0;

        for (int j = 0; j < numPieces; ++j) {
            int size;
            inputFile >> size;
            process.memoryPieces.push_back(size);
            process.totalMemory += size;
        }

        inputQueue.push(process);
    }

    MemoryManager memoryManager(memorySize, pageSize);
    int clock = 0;
    double totalTurnaroundTime = 0;

    while (!inputQueue.empty()) {
        // Process arrivals
        while (!inputQueue.empty() && inputQueue.front().arrivalTime <= clock) {
            Process process = inputQueue.front();
            inputQueue.pop();
            outputFile << "t = " << clock << ": Process " << process.id << " arrives\n";

            if (memoryManager.allocate(process)) {
                outputFile << "t = " << clock << ": Process " << process.id << " admitted to memory\n";
                memoryManager.printMemoryMap(outputFile);
                completedProcesses.push_back(process);
            } else {
                outputFile << "t = " << clock << ": Process " << process.id << " waiting for memory\n";
            }
        }

        // Process completions
        for (auto it = completedProcesses.begin(); it != completedProcesses.end(); ) {
            if (it->completionTime == clock) {
                outputFile << "t = " << clock << ": Process " << it->id << " completes\n";
                memoryManager.release(*it);
                memoryManager.printMemoryMap(outputFile);
                totalTurnaroundTime += (it->completionTime - it->arrivalTime);
                it = completedProcesses.erase(it);
            } else {
                ++it;
            }
        }

        clock++;
        if (clock > 100000) break; // Safety condition to avoid infinite loops
    }

    // Calculate average turnaround time
    double avgTurnaroundTime = completedProcesses.empty() ? 0 : totalTurnaroundTime / numProcesses;
    outputFile << "Average Turnaround Time: " << std::fixed << std::setprecision(2) << avgTurnaroundTime << "\n";
    inputFile.close();
    outputFile.close();
}

// Main function
int main() {
    const std::string inputFileName = "in1.txt"; // Ensure this file exists
    const int memorySize = 2000; // Memory size to be used for all simulations

    // Simulate for 3 different page sizes
    simulate(inputFileName, "out1.txt", memorySize, 100); // Page size 100
    simulate(inputFileName, "out2.txt", memorySize, 200); // Page size 200
    simulate(inputFileName, "out3.txt", memorySize, 400); // Page size 400

    std::cout << "Simulations completed. Results saved in out1.txt, out2.txt, and out3.txt.\n";
    return 0;
}

